#include "render/maze_renderer.hpp"
#include "entities/player.hpp"
#include "core/asset_load.hpp"
#include "core/grid.hpp"
#include "render/view_bounds.hpp"
#include <cmath>
#include <iostream>

MazeRenderer::MazeRenderer() {
  m_floorTileset = {0};
  m_wallTileset = {0};
  m_propTileset = {0};
  m_lightGradient = {0};
  m_lightMask = {0};
  m_lightMaskReady = false;
}

void MazeRenderer::loadTextures() {
  if (IsWindowReady()) {
    m_floorTileset =
        assets::loadTexture("assets/BCKRMlv1_Floor_set.png", "MazeRenderer");
    m_wallTileset =
        assets::loadTexture("assets/BCKRMlv1_Wall_set.png", "MazeRenderer");
    m_propTileset =
        assets::loadTexture("assets/BCKRMlv1_Prop_set.png", "MazeRenderer");

    generateLightGradient();
  } else {
    std::cerr << "[ERROR] Window not ready. Cannot load textures!" << std::endl;
  }
}

MazeRenderer::~MazeRenderer() {
  if (IsWindowReady()) {
    UnloadTexture(m_floorTileset);
    UnloadTexture(m_wallTileset);
    UnloadTexture(m_propTileset);
    UnloadTexture(m_lightGradient);
    if (m_lightMaskReady)
      UnloadRenderTexture(m_lightMask);
  }
}

void MazeRenderer::render(const Maze &maze, const Camera2D &camera,
                          const Viewport &canvas, AreaState state,
                          bool showGenerationZones) const {
  // --- FRUSTUM CULLING ---
  // Shared with the item passes; see view_bounds.hpp for why startY reaches
  // three cells further than the other edges.
  ViewBounds view = ViewBounds::fromCamera(maze, camera, canvas);

  for (int y = view.startY; y <= view.endY; ++y) {
    for (int x = view.startX; x <= view.endX; ++x) {
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
        DrawTexturePro(m_wallTileset, grid::srcTile(2, 1), grid::cellRect(x, y),
                       {0, 0}, 0.0f, WHITE);
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

          DrawTexturePro(m_wallTileset,
                         grid::srcTile((int)tilePos.x, (int)tilePos.y),
                         grid::cellRect(x, y), {0, 0}, 0.0f, WHITE);
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

          Rectangle sourceRectMask =
              grid::srcTile((int)tilePos.x, (int)tilePos.y);

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
            DrawTexturePro(m_wallTileset, sourceRectMask,
                           grid::cellRect(x, y - 2), {0, 0}, 0.0f, wallTint);

            // Wallpaper: upper course at y-1, base course at y - column 5,
            // rows 4 and 6 of the wall sheet. A door cell reaching this
            // branch always has visible room floor below it, i.e. it is
            // isDoorWithRoomBelow, so there is no door-wallpaper variant.
            DrawTexturePro(m_wallTileset, grid::srcTile(5, 4),
                           grid::cellRect(x, y - 1), {0, 0}, 0.0f, wallTint);
            DrawTexturePro(m_wallTileset, grid::srcTile(5, 6),
                           grid::cellRect(x, y), {0, 0}, 0.0f, wallTint);

            if (isDoorWithRoomBelow) {
              // One tile wide, 29 art px tall, standing on the base course.
              // The frame sits off the 16px grid on the prop sheet, so it is
              // addressed in art pixels.
              Rectangle sourceRectDoor = {16.0f, 2.0f, 16.0f, 29.0f};

              // If there is another door directly to the left on this same
              // wall, use the "right door" texture
              if (maze.getCell(x - 1, y) == Maze::CELL_CORRIDOR &&
                  maze.getCell(x - 1, y + 1) == Maze::CELL_ROOM) {
                sourceRectDoor = {80.0f, 19.0f, 16.0f, 29.0f};
              }

              DrawTexturePro(m_propTileset, sourceRectDoor,
                             grid::standingOn(sourceRectDoor, x, y), {0, 0},
                             0.0f, wallTint);
            }
          } else {
            // Bottom Wall / Inner Mass: Just draw the bitmasked roof tile at y
            DrawTexturePro(m_wallTileset, sourceRectMask, grid::cellRect(x, y),
                           {0, 0}, 0.0f, WHITE);
          }
        }
      } else {
        // Floor or Room
        if (cell == Maze::CELL_CORRIDOR || cell == Maze::CELL_ROOM) {
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

          DrawTexturePro(m_floorTileset, grid::srcTile(9, 0),
                         grid::cellRect(x, y), {0, 0}, 0.0f, drawColor);
        } else {
          DrawRectangleRec(grid::cellRect(x, y), MAGENTA);
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
void MazeRenderer::generateLightGradient() {
  const int diameter = kLightGradientDiameter;
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
// Cone angle and edge fade are baked into the gradient texture, so changing
// either rebuilds it; the radius is only a draw-time scale.
void MazeRenderer::updateLightSettings(float coneAngle, float fadeStrength,
                                       float sizeScale) {
  bool needsRegen =
      (m_lightConeAngle != coneAngle || m_lightFadeStrength != fadeStrength);

  m_lightConeAngle = coneAngle;
  m_lightFadeStrength = fadeStrength;
  m_lightSizeScale = sizeScale;

  if (needsRegen && IsWindowReady()) {
    if (m_lightGradient.id != 0) {
      UnloadTexture(m_lightGradient);
    }
    generateLightGradient();
  }
}

// ============================================================================
// initLightMask — Create/Resize the Light Mask RenderTexture
// ============================================================================
void MazeRenderer::initLightMask(const Viewport &canvas) {
  int screenW = canvas.width;
  int screenH = canvas.height;

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
// buildLightMask — Pre-calculate screen-space flashlight effect
// ============================================================================
void MazeRenderer::buildLightMask(Vector2 playerWorldPos,
                                  const Camera2D &camera,
                                  const Viewport &canvas, AreaState state,
                                  FacingDirection dir) {
  // Only draw darkness in corridors; rooms are always fully lit
  if (state != AreaState::CORRIDOR)
    return;

  // Ensure the light mask RenderTexture exists and matches the canvas
  initLightMask(canvas);
  if (!m_lightMaskReady)
    return;

  // Convert player's world position to screen position
  Vector2 playerScreen = GetWorldToScreen2D(playerWorldPos, camera);

  // Gradient diameter on the canvas: three cells, times the radius slider.
  float overlaySize = 3.0f * grid::CELL * m_lightSizeScale;

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
  // full sprite is visible. 14 art px, in canvas pixels.
  float backwardOffset = 14.0f * grid::WORLD_SCALE;
  Vector2 origin = {overlaySize / 2.0f, (overlaySize / 2.0f) - backwardOffset};

  DrawTexturePro(m_lightGradient, srcRect, destRect, origin, rotation, WHITE);

  EndTextureMode();
}

void MazeRenderer::drawLightMask(const Viewport &canvas) {
  if (!m_lightMaskReady) return;
  int screenW = canvas.width;
  int screenH = canvas.height;

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
