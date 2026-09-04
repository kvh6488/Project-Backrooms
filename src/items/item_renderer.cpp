#include "items/item_renderer.hpp"
#include "items/item_database.hpp"
#include "core/asset_load.hpp"
#include "world/view_bounds.hpp"
#include <cmath>
#include <iostream>

ItemRenderer::ItemRenderer() {
  m_postApocWorkshopTextures = {0};
  m_doodadsTexture = {0};
  m_mushroomTexture = {0};
  m_postApocIconsTexture = {0};
}

void ItemRenderer::loadTextures() {
  if (IsWindowReady()) {
    m_postApocWorkshopTextures = assets::loadTexture(
        "assets/PostApoc_Workshop_WithShadow.png", "ItemRenderer");
    m_doodadsTexture =
        assets::loadTexture("assets/doodads_spritesheet.png", "ItemRenderer");
    m_mushroomTexture = assets::loadTexture(
        "assets/mushrooms_pixel_asset.png", "ItemRenderer");
    m_postApocIconsTexture = assets::loadTexture(
        "assets/PostApoc_Workshop_Icons.png", "ItemRenderer");
    m_ritualTexture = assets::loadTexture(
        "assets/Spritesheet_TheDarkRitual_BigWander.png", "ItemRenderer");
  } else {
    std::cerr << "[ERROR] Window not ready. Cannot load item textures!"
              << std::endl;
  }
}

ItemRenderer::~ItemRenderer() {
  if (IsWindowReady()) {
    UnloadTexture(m_postApocWorkshopTextures);
    UnloadTexture(m_doodadsTexture);
    UnloadTexture(m_mushroomTexture);
    UnloadTexture(m_postApocIconsTexture);
    UnloadTexture(m_ritualTexture);
  }
}

// ============================================================================
// render — World-Space Item Pass
// ============================================================================
// This is called AFTER the player renderer inside BeginMode2D, so items
// draw on top of the player sprite. This gives the visual effect of the
// player walking "behind" tall furniture like cupboards.
//
// Uses the same frustum culling approach as MazeRenderer — only items
// within the visible screen region are drawn.
//
// Time Complexity: O(V) where V = number of visible cells on screen.
// Each cell is an O(1) lookup into the grid-parallel item array.
// ============================================================================
void ItemRenderer::render(const Maze &maze, const Camera2D &camera,
                          AreaState state) const {
  // --- FRUSTUM CULLING (shared with MazeRenderer) ---
  ViewBounds view = ViewBounds::fromCamera(maze, camera);
  int cellSize = maze.getCellSize();

  for (int y = view.startY; y <= view.endY; ++y) {
    for (int x = view.startX; x <= view.endX; ++x) {
      // Floor check plus the room/corridor visibility rule, shared with the
      // magic book pass so the two cannot drift apart.
      if (!isCellRenderable(maze, x, y, state))
        continue;

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
        Rectangle sourceRectDoodad = {3.0f * 16.0f, 1.0f * 16.0f, 16.0f, 16.0f};
        Rectangle destRectDoodad = {(float)(x * cellSize) + (cellSize / 2.0f) -
                                        8.0f,
                                    (float)(y * cellSize) + cellSize - 30.0f +
                                        (30.0f / 2.0f) - 6.0f, // 2px down
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

        // Normal mushrooms use top half (rows 0-1), magic mushrooms use bottom
        // half (rows 2-3)
        int ty = (maze.getItem(x, y) == ItemType::MUSHROOM)
                     ? (tileIndex / 4)
                     : (tileIndex / 4) + 2;
        Rectangle sourceRect = {(float)tx * 16.0f, (float)ty * 16.0f, 16.0f,
                                16.0f};

        // 2 times smaller than the cell size (i.e. twice as large as before)
        float mSize = cellSize / 2.0f;
        Rectangle destRect = {
            (float)(x * cellSize) + (cellSize / 2.0f) - (mSize / 2.0f),
            (float)(y * cellSize) + (cellSize / 2.0f) - (mSize / 2.0f), mSize,
            mSize};

        DrawTexturePro(m_mushroomTexture, sourceRect, destRect, {0, 0}, 0.0f,
                       WHITE);
        break;
      }
      case ItemType::CUPBOARD: {
        // --- Context-Aware Texture Selection ---
        // The cupboard's appearance depends on which wall it leans against.
        // Priority: wall above > wall right > wall left.
        // This is a simplified version of the bitmasking autotile pattern
        // used for walls, but only checking 3 directions.
        bool wallAbove = maze.getCell(x, y - 1) == Maze::CELL_WALL;
        bool wallRight = maze.getCell(x + 1, y) == Maze::CELL_WALL;
        bool wallLeft = maze.getCell(x - 1, y) == Maze::CELL_WALL;

        Rectangle cupSrc = {0};
        float cupW = 0, cupH = 0;
        bool flipH = false;

        // Vertical offset: cupboards against the top wall are nudged 3px
        // upward so they visually "sit flush" against the rendered wall
        // face (which projects downward via the Zelda-style wall system).
        float yOffset = 0.0f;

        if (wallAbove) {
          // Front-facing cupboard (viewed from the south)
          // Sprite rect: top-left (449, 198) to bottom-right (479, 255)
          cupSrc = {449.0f, 197.0f, 32.0f, 60.0f};
          cupW = 30.0f;
          cupH = 57.0f;
          yOffset = -14.0f; // Nudge upward to sit flush against wall face
        } else if (wallRight) {
          // Side-facing cupboard (leaning against right wall)
          // Sprite rect: top-left (481, 256) to bottom-right (512, 320)
          cupSrc = {480.0f, 256.0f, 32.0f, 64.0f};
          cupW = 31.0f;
          cupH = 64.0f;
        } else if (wallLeft) {
          // Side-facing cupboard (leaning against left wall) — horizontally
          // flipped Same sprite as wallRight, but we negate the source width
          // to flip. Raylib convention: negative source width = horizontal
          // mirror.
          cupSrc = {480.0f, 256.0f, 32.0f, 64.0f};
          cupW = 31.0f;
          cupH = 64.0f;
          flipH = true;
        } else {
          // Fallback: shouldn't happen given spawn rules, but default to front
          cupSrc = {449.0f, 198.0f, 30.0f, 57.0f};
          cupW = 30.0f;
          cupH = 57.0f;
        }

        // --- Color Variant Logic ---
        // Deterministically pick red or blue based on coordinates
        bool isRed = false;
        unsigned int hash = (unsigned int)(x * 73856093 ^ y * 19349663);
        if (hash % 2 == 0) {
          isRed = true;
          cupSrc.y += 128.0f; // Red variant is 128px below blue
        }

        // --- Open Cupboard Logic ---
        if (maze.getItemState(x, y) == 1) { // 1 = open
          bool isSideways = !wallAbove && (wallRight || wallLeft);
          if (isSideways) {
            // Sideways cupboards render the same open texture regardless of
            // contents
            cupSrc.x += 32.0f;
          } else {
            if (maze.isCupboardEmpty(x, y)) {
              cupSrc.x += 63.0f; // Open, no items,
            } else {
              cupSrc.x += 32.0f; // Open, with items
              if (isRed) {
                cupSrc.x -= 1.0f; // Shift red variant 1px left
              }
            }
          }
        }

        // Scale the sprite to fit within the cell while preserving aspect
        // ratio. The sprite anchors to the bottom of the cell (furniture sits
        // on the floor).
        float scale = (float)cellSize / cupW;
        float drawW = cupW * scale;
        float drawH = cupH * scale;

        // If the scaled height exceeds the cell, cap it
        if (drawH > cellSize * 2.0f) {
          scale = (cellSize * 2.0f) / cupH;
          drawW = cupW * scale;
          drawH = cupH * scale;
        }

        // Negate source width for horizontal flip (Raylib convention)
        Rectangle srcFinal = {cupSrc.x, cupSrc.y,
                              flipH ? -cupSrc.width : cupSrc.width,
                              cupSrc.height};

        // Anchor bottom-center of the cell, applying vertical offset
        Rectangle destRectCup = {
            (float)(x * cellSize) + (cellSize / 2.0f) - (drawW / 2.0f),
            (float)(y * cellSize) + cellSize - drawH + yOffset, drawW, drawH};

        DrawTexturePro(m_postApocWorkshopTextures, srcFinal, destRectCup,
                       {0, 0}, 0.0f, WHITE);
        break;
      }
      case ItemType::TABLE: {
        TableSprite table = computeTableSprite(maze, x, y);
        if (!table.valid) {
          break; // Non-root tile (state 0 / 2) - the root draws the whole sprite
        }
        DrawTexturePro(m_postApocWorkshopTextures, table.src, table.dest,
                       {0, 0}, 0.0f, WHITE);
        // NOTE: the magic book is deliberately NOT drawn here - see
        // renderMagicBookOverlay(), which runs in a later, shader-exempt pass.
        break;
      }
      case ItemType::NONE:
      default:
        break;
      }
    }
  }
}

// ============================================================================
// computeTableSprite - Resolve a table root tile to its atlas + world rect
// ============================================================================
// The grey/non-grey variant is picked by a spatial hash of the coordinates:
// a deterministic, storage-free way to give each tile a stable pseudo-random
// look. The same (x, y) always hashes to the same variant, so the table does
// not flicker between frames and nothing needs to be persisted in the maze.
// The two variants have DIFFERENT pixel dimensions, which is exactly why this
// must be computed in one place only.
// ============================================================================
ItemRenderer::TableSprite
ItemRenderer::computeTableSprite(const Maze &maze, int x, int y) const {
  TableSprite out = {};
  out.valid = false;

  int state = maze.getItemState(x, y);
  // Only the "root" tiles draw (1 = horizontal right, 3 = vertical bottom).
  if (state != 1 && state != 3) {
    return out;
  }

  bool isGrey = ((unsigned int)(x * 73856093 ^ y * 19349663) % 2 == 0);

  float tableW = 0.0f, tableH = 0.0f;
  if (state == 1) { // Horizontal Right
    if (isGrey) {
      out.src = {352.0f, 19.0f, 63.0f, 45.0f};
      tableW = 63.0f;
      tableH = 45.0f;
    } else {
      out.src = {353.0f, 406.0f, 61.0f, 41.0f};
      tableW = 61.0f;
      tableH = 41.0f;
    }
  } else { // state == 3, Vertical Bottom
    if (isGrey) {
      out.src = {416.0f, 0.0f, 31.0f, 64.0f};
      tableW = 31.0f;
      tableH = 64.0f;
    } else {
      out.src = {321.0f, 385.0f, 29.0f, 62.0f};
      tableW = 29.0f;
      tableH = 62.0f;
    }
  }

  int cellSize = maze.getCellSize();
  if (state == 1) {
    // Scale to fit width = 2 * cellSize, anchored to the bottom of the cell
    // and centered across x and x-1.
    float scale = (cellSize * 2.0f) / tableW;
    float drawW = tableW * scale;
    float drawH = tableH * scale;
    out.dest = {(float)(x * cellSize) - (drawW / 2.0f),
                (float)(y * cellSize) + cellSize - drawH, drawW, drawH};
  } else {
    // Scale to fit width = 1 * cellSize, anchored bottom, centered
    // horizontally.
    float scale = (float)cellSize / tableW;
    float drawW = tableW * scale;
    float drawH = tableH * scale;
    out.dest = {(float)(x * cellSize) + (cellSize / 2.0f) - (drawW / 2.0f),
                (float)(y * cellSize) + cellSize - drawH, drawW, drawH};
  }

  out.valid = true;
  return out;
}

// ============================================================================
// renderMagicBookOverlay - Shader-exempt pass for the magic book
// ============================================================================
// Runs AFTER EndShaderMode() so the book does not warp with the trip shader.
// Because it is outside render()'s per-cell scan it still has to apply the two
// things that loop provides for free — the visibility rule and the frustum
// cull — but both now come from view_bounds.hpp rather than being restated
// here, so they cannot drift from the item pass. Table geometry likewise comes
// from computeTableSprite().
// ============================================================================
void ItemRenderer::renderMagicBookOverlay(const Maze &maze,
                                          const Camera2D &camera,
                                          AreaState state, Vector2 tripOffset,
                                          float glowScale) const {
  if (!maze.isMagicBookSpawned()) {
    return;
  }

  int x = maze.getMagicBookX();
  int y = maze.getMagicBookY();

  if (!isCellRenderable(maze, x, y, state)) {
    return;
  }
  if (!ViewBounds::fromCamera(maze, camera).contains(x, y)) {
    return; // Off screen
  }

  int cellSize = maze.getCellSize();
  TableSprite table = computeTableSprite(maze, x, y);
  if (!table.valid) {
    return; // The book is only ever placed on a table root tile.
  }

  Rectangle bookSrc = {48.0f, 97.0f, 16.0f, 16.0f};
  const float bookW = 16.0f;
  const float bookH = 16.0f;

  // Sit the book on the table surface rather than its bounding-box center,
  // then ride along with whatever apparent motion the trip shader is giving
  // the table this frame.
  float centerX = table.dest.x + (table.dest.width / 2.0f) + tripOffset.x;
  float centerY = table.dest.y + (table.dest.height / 2.0f) - 8.0f +
                  tripOffset.y;

  float bookScale = (cellSize / 32.0f) * 1.5f;
  float drawBookW = bookW * bookScale;
  float drawBookH = bookH * bookScale;

  // --- Pulsing glow ---
  // Map sin from [-1, 1] into [0, 1] so it can drive a lerp directly. Driving
  // it off wall-clock time (not accumulated delta) keeps it frame-rate
  // independent.
  float pulse = (sinf((float)GetTime() * 5.0f) + 1.0f) * 0.5f;

  BeginBlendMode(BLEND_ADDITIVE);

  // Soft radial halo. A scaled copy of the SPRITE would inherit the book's
  // silhouette and its hard pixel-art edges; real light scatter is radially
  // symmetric with a smooth falloff, so a gradient reads far better and costs
  // one draw call instead of two.
  float haloRadius = drawBookW * (0.95f + 0.15f * pulse) * glowScale;
  Color haloInner = {255, 80, 140, (unsigned char)(70 + 55 * pulse)};
  Color haloOuter = {255, 40, 90, 0};
  DrawCircleGradient((int)centerX, (int)centerY, haloRadius, haloInner,
                     haloOuter);

  // One tight sprite-shaped rim so the book's outline still reads as lit.
  float rimScale = bookScale * (1.04f + 0.03f * pulse);
  Rectangle destRim = {centerX, centerY, bookW * rimScale, bookH * rimScale};
  Vector2 originRim = {destRim.width / 2.0f, destRim.height / 2.0f};
  Color rimColor = {255, 105, 180, (unsigned char)(110 + 70 * pulse)};
  DrawTexturePro(m_ritualTexture, bookSrc, destRim, originRim, 0.0f, rimColor);

  EndBlendMode();

  // --- The book itself ---
  Rectangle destRectBook = {centerX, centerY, drawBookW, drawBookH};
  Vector2 origin = {drawBookW / 2.0f, drawBookH / 2.0f};
  DrawTexturePro(m_ritualTexture, bookSrc, destRectBook, origin, 0.0f, WHITE);
}

// ============================================================================
// atlasFor - resolve a UiTexture name to the loaded handle
// ============================================================================
// This is still a switch, but note what it switches ON. It used to be a switch
// over ItemType, which grows with every item added; this one is over the atlas
// set, which the renderer already owns and which changes only when an artist
// hands over a new sheet. Adding an item is now one edit in ItemDatabase.
// ============================================================================
Texture2D ItemRenderer::atlasFor(UiTexture which) const {
  switch (which) {
  case UiTexture::WORKSHOP:
    return m_postApocWorkshopTextures;
  case UiTexture::WORKSHOP_ICONS:
    return m_postApocIconsTexture;
  case UiTexture::RITUAL:
    return m_ritualTexture;
  case UiTexture::MUSHROOMS:
  default:
    return m_mushroomTexture;
  }
}

void ItemRenderer::renderItemUI(ItemType type, Rectangle destRect,
                                Color tint) const {
  const auto &def = ItemDatabase::getDef(type);

  // The barrel has no inventory icon: it is drawn as a flat green swatch, so
  // it deliberately never reaches the atlas path.
  if (type == ItemType::TOXIC_WASTE) {
    DrawRectangleRec(destRect, GREEN);
    return;
  }

  if (def.uiSpriteRect.width > 0 && def.uiSpriteRect.height > 0) {
    DrawTexturePro(atlasFor(def.uiTexture), def.uiSpriteRect, destRect, {0, 0},
                   0.0f, tint);
  } else {
    DrawRectangleRec(destRect, tint);
  }
}
