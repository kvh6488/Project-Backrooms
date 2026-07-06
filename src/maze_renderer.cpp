#include "maze_renderer.hpp"
#include "player.hpp"
#include <cmath>
#include <iostream>

MazeRenderer::MazeRenderer() {
  m_floorTileset = {0};
  m_wallTileset = {0};
  m_propTileset = {0};
  m_postApocWorkshopTextures = {0};
  m_doodadsTexture = {0};
  m_lightGradient = {0};
  m_lightMask = {0};
  m_lightMaskReady = false;
}

void MazeRenderer::loadTextures() {
  if (IsWindowReady()) {
    m_floorTileset = LoadTexture("assets/BCKRMlv1_Floor_set.png");
    m_wallTileset = LoadTexture("assets/BCKRMlv1_Wall_set.png");
    m_propTileset = LoadTexture("assets/BCKRMlv1_Prop_set.png");
    m_postApocWorkshopTextures =
        LoadTexture("assets/PostApoc_Workshop_WithShadow.png");
    m_doodadsTexture = LoadTexture("assets/doodads_spritesheet.png");
    m_mushroomTexture = LoadTexture("assets/mushrooms_pixel_asset.png");

    // Generate the soft radial gradient for the flashlight overlay.
    // Diameter of 512px gives us a smooth, high-res gradient circle
    // that scales well when drawn to screen.
    generateLightGradient(512);
  } else {
    std::cerr << "[ERROR] Window not ready. Cannot load textures!" << std::endl;
  }
}

MazeRenderer::~MazeRenderer() {
  if (IsWindowReady()) {
    UnloadTexture(m_floorTileset);
    UnloadTexture(m_wallTileset);
    UnloadTexture(m_propTileset);
    UnloadTexture(m_postApocWorkshopTextures);
    UnloadTexture(m_doodadsTexture);
    UnloadTexture(m_mushroomTexture);
    UnloadTexture(m_lightGradient);
    if (m_lightMaskReady)
      UnloadRenderTexture(m_lightMask);
  }
}

void MazeRenderer::render(const Maze &maze, const Camera2D &camera,
                          AreaState state, bool showGenerationZones) const {
  // --- FRUSTUM CULLING ---
  Vector2 topLeft = GetScreenToWorld2D({0.0f, 0.0f}, camera);
  Vector2 bottomRight = GetScreenToWorld2D(
      {(float)GetScreenWidth(), (float)GetScreenHeight()}, camera);

  int cellSize = maze.getCellSize();

  // We are back to purely 1x1 scaling!
  // We subtract 3 from startY to ensure walls projecting upwards are drawn even
  // if their base is off-screen.
  int startX = (int)std::floor(topLeft.x / cellSize) - 1;
  int endX = (int)std::ceil(bottomRight.x / cellSize) + 1;
  int startY = (int)std::floor(topLeft.y / cellSize) - 3;
  int endY = (int)std::ceil(bottomRight.y / cellSize) + 1;

  for (int y = startY; y <= endY; ++y) {
    for (int x = startX; x <= endX; ++x) {
      int cell = maze.getCell(x, y);

      // Determine if this cell should be drawn as a "void" (unseen or out of
      // context)
      bool isDoorInRoom =
          (state == AreaState::ROOM && cell == Maze::CELL_CORRIDOR);

      bool isDoorWithRoomBelow = false;
      if (isDoorInRoom) {
        if (maze.getCell(x, y + 1) == Maze::CELL_ROOM &&
            maze.isVisible(x, y + 1)) {
          isDoorWithRoomBelow = true;
        }
      }

      bool isRoomTouchingCorridor = false;
      if (state == AreaState::CORRIDOR && cell == Maze::CELL_ROOM) {
        if (maze.getCell(x + 1, y) == Maze::CELL_CORRIDOR ||
            maze.getCell(x - 1, y) == Maze::CELL_CORRIDOR ||
            maze.getCell(x, y + 1) == Maze::CELL_CORRIDOR ||
            maze.getCell(x, y - 1) == Maze::CELL_CORRIDOR) {
          isRoomTouchingCorridor = true;
        }
      }

      bool isVoid = false;

      if (state == AreaState::ROOM) {
        // In rooms, we MUST respect the FOV (BFS flood fill) so we only see the
        // current room.
        if (!maze.isVisible(x, y)) {
          isVoid = true;
        } else if (cell == Maze::CELL_CORRIDOR) {
          // Doors in rooms are rendered as walls, so they are not void!
          isVoid = false;
        }
      } else if (state == AreaState::CORRIDOR) {
        // In corridors, ignore FOV to let the flashlight mask do the work.
        // But we still don't want to render rooms that aren't touching the
        // corridor.
        if (cell == Maze::CELL_ROOM && !isRoomTouchingCorridor) {
          isVoid = true;
        }
      }

      if (isVoid) {
        // Draw the isolated wall tile {2, 1} to fill the void!
        Rectangle sourceRect = {2.0f * 16.0f, 1.0f * 16.0f, 16.0f, 16.0f};
        Rectangle destRect = {(float)(x * cellSize), (float)(y * cellSize),
                              (float)cellSize, (float)cellSize};
        DrawTexturePro(m_wallTileset, sourceRect, destRect, {0, 0}, 0.0f,
                       WHITE);
        continue;
      }

      if (cell == Maze::CELL_WALL || isRoomTouchingCorridor || isDoorInRoom) {
        // --- BITMASKING / AUTOTILING PATTERN ---
        // Map the mask (0-15) to spritesheet coordinates (x, y)
        static const Vector2 tileMap[16] = {
            {2.0f, 1.0f}, {2.0f, 0.0f}, {3.0f, 1.0f},
            {3.0f, 0.0f}, // Index 0 uses {2,1} for isolated walls
            {2.0f, 2.0f}, {2.0f, 3.0f}, {3.0f, 2.0f},
            {3.0f, 3.0f}, {1.0f, 1.0f}, {1.0f, 0.0f},
            {0.0f, 1.0f}, {0.0f, 0.0f}, {1.0f, 2.0f},
            {1.0f, 3.0f}, {0.0f, 2.0f}, {0.0f, 3.0f}};

        if (state == AreaState::CORRIDOR) {
          auto isCorridor = [&maze](int cx, int cy) {
            return (maze.getCell(cx, cy) == Maze::CELL_CORRIDOR);
          };

          int mask = 0;
          if (isCorridor(x, y - 1))
            mask |= 1; // N
          if (isCorridor(x + 1, y))
            mask |= 2; // E
          if (isCorridor(x, y + 1))
            mask |= 4; // S
          if (isCorridor(x - 1, y))
            mask |= 8; // W

          Vector2 tilePos = tileMap[mask];
          if (isRoomTouchingCorridor) {
            tilePos.x += 8.0f;
          }

          Rectangle sourceRect = {tilePos.x * 16.0f, tilePos.y * 16.0f, 16.0f,
                                  16.0f};
          Rectangle destRect = {(float)(x * cellSize), (float)(y * cellSize),
                                (float)cellSize, (float)cellSize};
          DrawTexturePro(m_wallTileset, sourceRect, destRect, {0, 0}, 0.0f,
                         WHITE);
        } else {
          // state == AreaState::ROOM
          auto isVisibleRoom = [&maze](int cx, int cy) {
            return (maze.getCell(cx, cy) == Maze::CELL_ROOM) &&
                   maze.isVisible(cx, cy);
          };

          int mask = 0;
          if (isVisibleRoom(x, y - 1))
            mask += 1; // Top
          if (isVisibleRoom(x + 1, y))
            mask += 2; // Right
          if (isVisibleRoom(x, y + 1))
            mask += 4; // Bottom
          if (isVisibleRoom(x - 1, y))
            mask += 8; // Left

          Vector2 tilePos = tileMap[mask];
          if (isDoorInRoom && !isDoorWithRoomBelow) {
            tilePos.x += 8.0f;
          }

          Rectangle sourceRectMask = {tilePos.x * 16.0f, tilePos.y * 16.0f,
                                      16.0f, 16.0f};

          int belowCell = maze.getCell(x, y + 1);
          // Zelda Top Walls ONLY apply in Rooms when the floor below is
          // visible!
          bool belowIsVisibleFloor =
              maze.isVisible(x, y + 1) && (belowCell == Maze::CELL_ROOM);

          if (belowIsVisibleFloor) {
            Color wallTint = WHITE;
            if (maze.getRadiationLevel(x, y + 1) > 0) {
              wallTint = (Color){200, 255, 200, 255};
            }

            // Zelda Top Wall: Project UPWARDS into the void!
            // Draw bitmasked roof edge at y-2 (which happens to be {2,2} for
            // mask 4)
            Rectangle destRectRoof = {(float)(x * cellSize),
                                      (float)((y - 2) * cellSize),
                                      (float)cellSize, (float)cellSize};
            DrawTexturePro(m_wallTileset, sourceRectMask, destRectRoof, {0, 0},
                           0.0f, wallTint);

            float wallX = 5.0f;
            if (isDoorInRoom && !isDoorWithRoomBelow) {
              wallX += 8.0f;
            }

            // Draw upper wallpaper at y-1
            Rectangle sourceRectTop = {wallX * 16.0f, 4.0f * 16.0f, 16.0f,
                                       16.0f};
            Rectangle destRectTop = {(float)(x * cellSize),
                                     (float)((y - 1) * cellSize),
                                     (float)cellSize, (float)cellSize};
            DrawTexturePro(m_wallTileset, sourceRectTop, destRectTop, {0, 0},
                           0.0f, wallTint);

            // Draw base wallpaper at y
            Rectangle sourceRectBot = {wallX * 16.0f, 6.0f * 16.0f, 16.0f,
                                       16.0f};
            Rectangle destRectBot = {(float)(x * cellSize),
                                     (float)(y * cellSize), (float)cellSize,
                                     (float)cellSize};
            DrawTexturePro(m_wallTileset, sourceRectBot, destRectBot, {0, 0},
                           0.0f, wallTint);

            if (isDoorWithRoomBelow) {
              float scale = cellSize / 16.0f;
              Rectangle sourceRectDoor = {16.0f, 2.0f, 16.0f, 29.0f};

              // If there is another door directly to the left on this same
              // wall, use the "right door" texture
              if (maze.getCell(x - 1, y) == Maze::CELL_CORRIDOR &&
                  maze.getCell(x - 1, y + 1) == Maze::CELL_ROOM) {
                sourceRectDoor = {80.0f, 19.0f, 16.0f, 29.0f};
              }

              Rectangle destRectDoor = {
                  (float)(x * cellSize),
                  (float)((y * cellSize) + cellSize - (29.0f * scale)),
                  (float)cellSize, 29.0f * scale};
              DrawTexturePro(m_propTileset, sourceRectDoor, destRectDoor,
                             {0, 0}, 0.0f, wallTint);
            }
          } else {
            // Bottom Wall / Inner Mass: Just draw the bitmasked roof tile at y
            Rectangle destRectTop = {(float)(x * cellSize),
                                     (float)(y * cellSize), (float)cellSize,
                                     (float)cellSize};
            DrawTexturePro(m_wallTileset, sourceRectMask, destRectTop, {0, 0},
                           0.0f, WHITE);
          }
        }
      } else {
        // Floor or Room
        bool isTexture = false;
        Rectangle sourceRect = {0};

        if (cell == Maze::CELL_CORRIDOR || cell == Maze::CELL_ROOM) {
          isTexture = true;
          sourceRect = {9.0f * 16.0f, 0.0f * 16.0f, 16.0f, 16.0f};
        }

        if (isTexture) {
          // The light mask overlay handles all darkness/shadows now.
          // We just draw the tiles at full brightness (WHITE).
          Color drawColor = WHITE;
          if (showGenerationZones && maze.isShiftingZone(x, y)) {
            // Highlight shifting zones in red
            drawColor = (Color){255, 100, 100, 255};
          }

          if (maze.getRadiationLevel(x, y) > 0) {
            // Green tint for radiated zones
            drawColor = (Color){200, 255, 200, 255};
          }

          Rectangle destRect = {(float)(x * cellSize), (float)(y * cellSize),
                                (float)cellSize, (float)cellSize};
          DrawTexturePro(m_floorTileset, sourceRect, destRect, {0, 0}, 0.0f,
                         drawColor);

          // --- Item Rendering (Grid-Parallel Switch) ---
          // O(1) lookup per cell. Adding a new item type means adding a
          // new case here — no need to touch any other rendering code.
          switch (maze.getItem(x, y)) {
            case ItemType::TOXIC_WASTE: {
              Rectangle sourceRectBarrel = {70.0f, 193.0f, 22.0f, 30.0f};
              Rectangle destRectBarrel = {
                  (float)(x * cellSize) + (cellSize / 2.0f) - 11.0f,
                  (float)(y * cellSize) + cellSize - 30.0f, 22.0f, 30.0f};
              DrawTexturePro(m_postApocWorkshopTextures, sourceRectBarrel,
                             destRectBarrel, {0, 0}, 0.0f, WHITE);

              // Draw radiation symbol centered over the barrel
              Rectangle sourceRectDoodad = {3.0f * 16.0f, 1.0f * 16.0f, 16.0f,
                                            16.0f};
              Rectangle destRectDoodad = {
                  (float)(x * cellSize) + (cellSize / 2.0f) - 8.0f,
                  (float)(y * cellSize) + cellSize - 30.0f + (30.0f / 2.0f) -
                      6.0f, // 2px down
                  16.0f, 16.0f};
              DrawTexturePro(m_doodadsTexture, sourceRectDoodad, destRectDoodad,
                             {0, 0}, 0.0f, WHITE);
              break;
            }
            case ItemType::MUSHROOM:
            case ItemType::MAGIC_MUSHROOM: {
              // Pseudo-random consistent hash to pick a tile from the top 8 (0-7)
              int tileIndex = (x * 73 + y * 37) % 8;
              int tx = tileIndex % 4;
              
              // Normal mushrooms use top half (rows 0-1), magic mushrooms use bottom half (rows 2-3)
              int ty = (maze.getItem(x, y) == ItemType::MUSHROOM) ? (tileIndex / 4) 
                                                                  : (tileIndex / 4) + 2;
              Rectangle sourceRect = {(float)tx * 16.0f, (float)ty * 16.0f, 16.0f, 16.0f};
              
              // 2 times smaller than the cell size (i.e. twice as large as before)
              float mSize = cellSize / 2.0f;
              Rectangle destRect = {
                  (float)(x * cellSize) + (cellSize / 2.0f) - (mSize / 2.0f),
                  (float)(y * cellSize) + (cellSize / 2.0f) - (mSize / 2.0f),
                  mSize, mSize};
                  
              DrawTexturePro(m_mushroomTexture, sourceRect, destRect, {0, 0}, 0.0f, WHITE);
              break;
            }
            case ItemType::NONE:
            default:
              break;
          }
        } else {
          DrawRectangle(x * cellSize, y * cellSize, cellSize, cellSize,
                        MAGENTA);
        }
      }
    }
  }
}

// ============================================================================
// generateLightGradient — Procedural Radial Gradient Texture
// ============================================================================
// Creates a semi-circular gradient image (facing UP/North): fully opaque
// white at the center, fading smoothly to fully transparent at the edges.
// Only the top half (dy <= 0) is generated to create a directional "torch"
// cone.
//
// This is drawn on top of a black screen-filling rectangle to create
// the "flashlight hole" effect.
// ============================================================================
void MazeRenderer::generateLightGradient(int diameter) {
  Image img = GenImageColor(diameter, diameter, BLACK);
  float center = diameter / 2.0f;
  float radius = center;

  for (int y = 0; y < diameter; ++y) {
    for (int x = 0; x < diameter; ++x) {
      float dx = x - center;
      float dy = y - center;
      float dist = std::sqrt(dx * dx + dy * dy);

      // Calculate angle from center. In screen space, UP is -y, so -90 degrees.
      float angle = std::atan2(dy, dx) * 180.0f / PI;

      // Calculate difference from UP (-90.0f)
      float diff = std::abs(angle - (-90.0f));
      if (diff > 180.0f)
        diff = 360.0f - diff;

      float halfConeAngle = m_lightConeAngle / 2.0f;
      if (dist < radius && diff <= halfConeAngle) {
        // Distance falloff: alpha starts at 1.0 at center and drops to 0 at
        // edge of circle.
        float normalizedDist = dist / radius;
        float distAlpha = 1.0f - (normalizedDist * normalizedDist);

        // Angular falloff: starts at center (0 degrees) and fades to the edge.
        // The fade strength controls the exponent of the curve.
        float fadeFactor = diff / halfConeAngle;
        float angularAlpha =
            1.0f - (std::pow(fadeFactor, m_lightFadeStrength) * 0.95f);

        float finalAlpha = distAlpha * angularAlpha;
        unsigned char a = static_cast<unsigned char>(finalAlpha * 255.0f);
        ImageDrawPixel(&img, x, y, (Color){255, 255, 255, a});
      }
      // Pixels outside the radius remain BLACK (fully opaque darkness)
    }
  }

  m_lightGradient = LoadTextureFromImage(img);
  UnloadImage(img);
}

// ============================================================================
// updateLightSettings — Update flashlight parameters and regenerate if needed
// ============================================================================
void MazeRenderer::updateLightSettings(float coneAngle, float fadeStrength,
                                       float sizeScale) {
  bool needsRegen =
      (m_lightConeAngle != coneAngle || m_lightFadeStrength != fadeStrength);

  m_lightConeAngle = coneAngle;
  m_lightFadeStrength = fadeStrength;
  m_lightSizeScale = sizeScale;

  if (needsRegen) {
    if (m_lightGradient.id != 0) {
      UnloadTexture(m_lightGradient);
    }
    generateLightGradient(250);
  }
}

// ============================================================================
// initLightMask — Create/Resize the Light Mask RenderTexture
// ============================================================================
void MazeRenderer::initLightMask() {
  int screenW = GetScreenWidth();
  int screenH = GetScreenHeight();

  // If already allocated at the right size, skip
  if (m_lightMaskReady && m_lightMask.texture.width == screenW &&
      m_lightMask.texture.height == screenH) {
    return;
  }

  if (m_lightMaskReady)
    UnloadRenderTexture(m_lightMask);
  m_lightMask = LoadRenderTexture(screenW, screenH);
  m_lightMaskReady = true;
}

// ============================================================================
// renderDarknessOverlay — Screen-Space Flashlight Effect (Fixed)
// ============================================================================
// Uses a RenderTexture "light mask":
//   1. Render the light mask off-screen: clear to BLACK, draw directional
//   gradient
//   2. Draw the light mask onto the main framebuffer using BLEND_MULTIPLIED
//   3. Where the mask is white (1.0): tiles show through unchanged
//      Where the mask is black (0.0): tiles are multiplied to darkness
// ============================================================================
void MazeRenderer::renderDarknessOverlay(Vector2 playerWorldPos,
                                         const Camera2D &camera,
                                         AreaState state, FacingDirection dir) {
  // Only draw darkness in corridors; rooms are always fully lit
  if (state != AreaState::CORRIDOR)
    return;

  // Ensure the light mask RenderTexture exists and matches screen size
  initLightMask();
  if (!m_lightMaskReady)
    return;

  int screenW = GetScreenWidth();
  int screenH = GetScreenHeight();

  // Convert player's world position to screen position
  Vector2 playerScreen = GetWorldToScreen2D(playerWorldPos, camera);

  // Scale the gradient to match the FOV radius.
  float overlaySize = 3.0f * 32.0f * camera.zoom * m_lightSizeScale;

  // Calculate rotation based on facing direction
  // The texture is generated facing UP (0 degrees rotation).
  // Raylib rotations are clockwise.
  float rotation = 0.0f;
  switch (dir) {
  case FacingDirection::UP:
    rotation = 0.0f;
    break;
  case FacingDirection::RIGHT:
    rotation = 90.0f;
    break;
  case FacingDirection::DOWN:
    rotation = 180.0f;
    break;
  case FacingDirection::LEFT:
    rotation = 270.0f;
    break;
  }

  // --- Step 1: Build the light mask in the off-screen RenderTexture ---
  BeginTextureMode(m_lightMask);
  ClearBackground(BLACK); // Start with total darkness

  // Draw the directional gradient at the player's screen position.
  Rectangle srcRect = {0, 0, (float)m_lightGradient.width,
                       (float)m_lightGradient.height};
  // The destRect specifies the location of the origin (which we set to center)
  Rectangle destRect = {playerScreen.x, playerScreen.y, overlaySize,
                        overlaySize};

  // Shift the origin backward (down in local texture space) so the tip
  // of the cone drops slightly behind the player. This ensures the player's
  // full sprite is visible.
  float backwardOffset = 28.0f * camera.zoom;
  Vector2 origin = {overlaySize / 2.0f, (overlaySize / 2.0f) - backwardOffset};

  DrawTexturePro(m_lightGradient, srcRect, destRect, origin, rotation, WHITE);

  EndTextureMode();

  // --- Step 2: Composite the light mask onto the main framebuffer ---
  // BLEND_MULTIPLIED: finalPixel = framebufferPixel × maskPixel
  //   mask white (1,1,1) → tiles show through
  //   mask black (0,0,0) → tiles become black
  BeginBlendMode(BLEND_MULTIPLIED);

  // RenderTextures in Raylib are vertically flipped, so we negate the height
  DrawTexturePro(m_lightMask.texture, {0, 0, (float)screenW, -(float)screenH},
                 {0, 0, (float)screenW, (float)screenH}, {0, 0}, 0.0f, WHITE);

  EndBlendMode();
}

void MazeRenderer::renderItemUI(ItemType type, Rectangle destRect, Color tint) const {
  switch (type) {
    case ItemType::MUSHROOM:
    case ItemType::MAGIC_MUSHROOM: {
      Rectangle sourceRect = {
          (type == ItemType::MUSHROOM) ? 16.0f : 0.0f,
          (type == ItemType::MUSHROOM) ? 0.0f : 32.0f,
          16.0f, 16.0f
      };
      DrawTexturePro(m_mushroomTexture, sourceRect, destRect, {0, 0}, 0.0f, tint);
      break;
    }
    case ItemType::TOXIC_WASTE: {
      DrawRectangleRec(destRect, GREEN);
      break;
    }
    default:
      DrawRectangleRec(destRect, tint);
      break;
  }
}
