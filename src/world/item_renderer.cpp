#include "world/item_renderer.hpp"
#include <cmath>
#include <iostream>

ItemRenderer::ItemRenderer() {
  m_postApocWorkshopTextures = {0};
  m_doodadsTexture = {0};
  m_mushroomTexture = {0};
}

void ItemRenderer::loadTextures() {
  if (IsWindowReady()) {
    m_postApocWorkshopTextures =
        LoadTexture("assets/PostApoc_Workshop_WithShadow.png");
    m_doodadsTexture = LoadTexture("assets/doodads_spritesheet.png");
    m_mushroomTexture = LoadTexture("assets/mushrooms_pixel_asset.png");
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
  // --- FRUSTUM CULLING (same approach as MazeRenderer) ---
  Vector2 topLeft = GetScreenToWorld2D({0.0f, 0.0f}, camera);
  Vector2 bottomRight = GetScreenToWorld2D(
      {(float)GetScreenWidth(), (float)GetScreenHeight()}, camera);

  int cellSize = maze.getCellSize();

  int startX = (int)std::floor(topLeft.x / cellSize) - 1;
  int endX = (int)std::ceil(bottomRight.x / cellSize) + 1;
  int startY = (int)std::floor(topLeft.y / cellSize) - 3;
  int endY = (int)std::ceil(bottomRight.y / cellSize) + 1;

  for (int y = startY; y <= endY; ++y) {
    for (int x = startX; x <= endX; ++x) {
      int cell = maze.getCell(x, y);

      // Only render items on visible floor/room cells
      if (cell != Maze::CELL_ROOM && cell != Maze::CELL_CORRIDOR)
        continue;

      // Visibility check: in rooms, respect FOV; in corridors, let the
      // flashlight mask handle it (same logic as MazeRenderer)
      if (state == AreaState::ROOM && !maze.isVisible(x, y))
        continue;

      if (state == AreaState::CORRIDOR && cell == Maze::CELL_ROOM) {
        // Don't render items in rooms that aren't touching the corridor
        bool touchesCorridor = maze.getCell(x + 1, y) == Maze::CELL_CORRIDOR ||
                               maze.getCell(x - 1, y) == Maze::CELL_CORRIDOR ||
                               maze.getCell(x, y + 1) == Maze::CELL_CORRIDOR ||
                               maze.getCell(x, y - 1) == Maze::CELL_CORRIDOR;
        if (!touchesCorridor)
          continue;
      }

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
        if (maze.getTestSwapCupboardColors())
          hash += 1; // Toggle color for testing
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
      case ItemType::NONE:
      default:
        break;
      }
    }
  }
}

// ============================================================================
// renderItemUI — Screen-Space Inventory Icon Drawing
// ============================================================================
void ItemRenderer::renderItemUI(ItemType type, Rectangle destRect,
                                Color tint) const {
  switch (type) {
  case ItemType::MUSHROOM:
  case ItemType::MAGIC_MUSHROOM: {
    Rectangle sourceRect = {(type == ItemType::MUSHROOM) ? 16.0f : 0.0f,
                            (type == ItemType::MUSHROOM) ? 0.0f : 32.0f, 16.0f,
                            16.0f};
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
