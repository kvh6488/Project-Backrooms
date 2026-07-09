#include "items/item_renderer.hpp"
#include "items/item_database.hpp"
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
    m_postApocWorkshopTextures =
        LoadTexture("assets/PostApoc_Workshop_WithShadow.png");
    m_doodadsTexture = LoadTexture("assets/doodads_spritesheet.png");
    m_mushroomTexture = LoadTexture("assets/mushrooms_pixel_asset.png");
    m_postApocIconsTexture = LoadTexture("assets/PostApoc_Workshop_Icons.png");
    m_ritualTexture =
        LoadTexture("assets/Spritesheet_TheDarkRitual_BigWander.png");
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
        // Strictly no room items should render while the player is in the
        // corridors.
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
        int state = maze.getItemState(x, y);
        // We only draw on the "root" tiles (1 for horizontal right, 3 for
        // vertical bottom)
        if (state == 0 || state == 2) {
          break;
        }

        bool isGrey = false;
        unsigned int hash = (unsigned int)(x * 73856093 ^ y * 19349663);
        if (hash % 2 == 0) {
          isGrey = true;
        }

        Rectangle tableSrc = {0};
        float tableW = 0, tableH = 0;

        if (state == 1) { // Horizontal Right
          if (isGrey) {
            tableSrc = {352.0f, 19.0f, 63.0f, 45.0f};
            tableW = 63.0f;
            tableH = 45.0f;
          } else {
            tableSrc = {353.0f, 406.0f, 61.0f, 41.0f};
            tableW = 61.0f;
            tableH = 41.0f;
          }
        } else if (state == 3) { // Vertical Bottom
          if (isGrey) {
            tableSrc = {416.0f, 0.0f, 31.0f, 64.0f};
            tableW = 31.0f;
            tableH = 64.0f;
          } else {
            tableSrc = {321.0f, 385.0f, 29.0f, 62.0f};
            tableW = 29.0f;
            tableH = 62.0f;
          }
        }

        Rectangle destRectTable = {0};
        if (state == 1) {
          // Scale to fit width = 2 * cellSize
          float scale = (cellSize * 2.0f) / tableW;
          float drawW = tableW * scale;
          float drawH = tableH * scale;
          // Anchor bottom of the cell, centered over x and x-1
          destRectTable = {(float)(x * cellSize) - (drawW / 2.0f),
                           (float)(y * cellSize) + cellSize - drawH, drawW,
                           drawH};
        } else if (state == 3) {
          // Scale to fit width = 1 * cellSize
          float scale = (float)cellSize / tableW;
          float drawW = tableW * scale;
          float drawH = tableH * scale;
          // Anchor bottom of the cell, centered horizontally
          destRectTable = {
              (float)(x * cellSize) + (cellSize / 2.0f) - (drawW / 2.0f),
              (float)(y * cellSize) + cellSize - drawH, drawW, drawH};
        }

        DrawTexturePro(m_postApocWorkshopTextures, tableSrc, destRectTable,
                       {0, 0}, 0.0f, WHITE);

        // --- Render Magic Book of Maps Overlay ---
        if (maze.isMagicBookSpawned() && maze.getMagicBookX() == x &&
            maze.getMagicBookY() == y) {
          Rectangle bookSrc = {48.0f, 97.0f, 16.0f, 16.0f};
          float bookW = 16.0f;
          float bookH = 16.0f;

          float centerX, centerY;
          if (state == 1) { // Horizontal Right
            centerX = destRectTable.x + (destRectTable.width / 2.0f);
            centerY = destRectTable.y + (destRectTable.height / 2.0f) - 8.0f;
          } else { // Vertical Bottom
            centerX = destRectTable.x + (destRectTable.width / 2.0f);
            centerY = destRectTable.y + (destRectTable.height / 2.0f) - 8.0f;
          }

          // Adjust scale to match table's pixel-art feel
          float bookScale =
              (cellSize / 32.0f) * 1.5f; // Slight enlargement for visibility
          float drawBookW = bookW * bookScale;
          float drawBookH = bookH * bookScale;

          Rectangle destRectBook = {centerX, centerY, drawBookW, drawBookH};
          Vector2 origin = {drawBookW / 2.0f, drawBookH / 2.0f};

          // --- Glow Effect ---
          float t = (float)GetTime();
          float pulse = (sinf(t * 5.0f) + 1.0f) * 0.5f; // Oscillates 0.0 to 1.0
          
          BeginBlendMode(BLEND_ADDITIVE);
          
          // Inner glow
          float glowScale1 = bookScale * (1.05f + 0.05f * pulse);
          Color glowColor1 = {255, 105, 180, (unsigned char)(120 + 80 * pulse)}; // Hot pink/red
          Rectangle destGlow1 = {centerX, centerY, bookW * glowScale1, bookH * glowScale1};
          Vector2 originGlow1 = {destGlow1.width / 2.0f, destGlow1.height / 2.0f};
          DrawTexturePro(m_ritualTexture, bookSrc, destGlow1, originGlow1, 0.0f, glowColor1);
          
          // Outer glow
          float glowScale2 = bookScale * (1.2f + 0.1f * pulse);
          Color glowColor2 = {255, 50, 100, (unsigned char)(60 + 40 * pulse)}; // Deeper red/pink
          Rectangle destGlow2 = {centerX, centerY, bookW * glowScale2, bookH * glowScale2};
          Vector2 originGlow2 = {destGlow2.width / 2.0f, destGlow2.height / 2.0f};
          DrawTexturePro(m_ritualTexture, bookSrc, destGlow2, originGlow2, 0.0f, glowColor2);
          
          EndBlendMode();

          // Normal solid book on top
          DrawTexturePro(m_ritualTexture, bookSrc, destRectBook, origin, 0.0f,
                         WHITE);
        }
        break;
      }
      case ItemType::NONE:
      default:
        break;
      }
    }
  }
}

void ItemRenderer::renderItemUI(ItemType type, Rectangle destRect,
                                Color tint) const {
  const auto &def = ItemDatabase::getDef(type);

  if (type == ItemType::TOXIC_WASTE) {
    DrawRectangleRec(destRect, GREEN);
    return;
  }

  if (def.uiSpriteRect.width > 0 && def.uiSpriteRect.height > 0) {
    Texture2D texToUse = m_mushroomTexture; // Fallback

    switch (type) {
    case ItemType::PAPER:
    case ItemType::PENCIL:
      texToUse = m_postApocWorkshopTextures;
      break;
    case ItemType::MAP:
      texToUse = m_postApocIconsTexture;
      break;
    case ItemType::MUSHROOM:
    case ItemType::MAGIC_MUSHROOM:
      texToUse = m_mushroomTexture;
      break;
    case ItemType::MAGIC_BOOK_OF_MAPS:
      texToUse = m_ritualTexture;
      break;
    default:
      break;
    }

    DrawTexturePro(texToUse, def.uiSpriteRect, destRect, {0, 0}, 0.0f, tint);
  } else {
    DrawRectangleRec(destRect, tint);
  }
}
