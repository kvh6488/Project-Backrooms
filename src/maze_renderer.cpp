#include "maze_renderer.hpp"
#include <cmath>
#include <iostream>

MazeRenderer::MazeRenderer() {
  m_floorTileset = {0};
  m_wallTileset = {0};
}

void MazeRenderer::loadTextures() {
  if (IsWindowReady()) {
    m_floorTileset = LoadTexture("assets/BCKRMlv1_Floor_set.png");
    m_wallTileset = LoadTexture("assets/BCKRMlv1_Wall_set.png");
  } else {
    std::cerr << "[ERROR] Window not ready. Cannot load textures!" << std::endl;
  }
}

MazeRenderer::~MazeRenderer() {
  if (IsWindowReady()) {
    UnloadTexture(m_floorTileset);
    UnloadTexture(m_wallTileset);
  }
}

void MazeRenderer::render(const Maze &maze, const Camera2D &camera, AreaState state) const {
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

      // Determine if this cell should be drawn as a "void" (unseen or out of context)
      bool isVoid = false;
      if (!maze.isVisible(x, y)) {
        isVoid = true;
      } else if (state == AreaState::CORRIDOR && cell == Maze::CELL_ROOM) {
        isVoid = true;
      } else if (state == AreaState::ROOM && cell == Maze::CELL_CORRIDOR) {
        isVoid = true;
      }

      if (isVoid) {
        // Draw the isolated wall tile {2, 1} to fill the void!
        Rectangle sourceRect = {2.0f * 16.0f, 1.0f * 16.0f, 16.0f, 16.0f};
        Rectangle destRect = {(float)(x * cellSize), (float)(y * cellSize),
                              (float)cellSize, (float)cellSize};
        DrawTexturePro(m_wallTileset, sourceRect, destRect, {0, 0}, 0.0f, WHITE);
        continue;
      }

      if (cell == Maze::CELL_WALL) {
        // --- BITMASKING / AUTOTILING PATTERN ---
        // Map the mask (0-15) to spritesheet coordinates (x, y)
        static const Vector2 tileMap[16] = {
            {2.0f, 1.0f}, {2.0f, 0.0f}, {3.0f, 1.0f}, {3.0f, 0.0f}, // Index 0 uses {2,1} for isolated walls
            {2.0f, 2.0f}, {2.0f, 3.0f}, {3.0f, 2.0f}, {3.0f, 3.0f},
            {1.0f, 1.0f}, {1.0f, 0.0f}, {0.0f, 1.0f}, {0.0f, 0.0f},
            {1.0f, 2.0f}, {1.0f, 3.0f}, {0.0f, 2.0f}, {0.0f, 3.0f}
        };

        if (state == AreaState::CORRIDOR) {
          auto isVisibleCorridor = [&maze](int cx, int cy) {
            return (maze.getCell(cx, cy) == Maze::CELL_CORRIDOR) && maze.isVisible(cx, cy);
          };
          
          int mask = 0;
          if (isVisibleCorridor(x, y - 1)) mask += 1; // Top
          if (isVisibleCorridor(x + 1, y)) mask += 2; // Right
          if (isVisibleCorridor(x, y + 1)) mask += 4; // Bottom
          if (isVisibleCorridor(x - 1, y)) mask += 8; // Left

          Vector2 tilePos = tileMap[mask];
          Rectangle sourceRect = {tilePos.x * 16.0f, tilePos.y * 16.0f, 16.0f, 16.0f};
          Rectangle destRect = {(float)(x * cellSize),
                                (float)(y * cellSize), (float)cellSize,
                                (float)cellSize};
          DrawTexturePro(m_wallTileset, sourceRect, destRect, {0, 0}, 0.0f, WHITE);
        } else {
          // state == AreaState::ROOM
          auto isVisibleRoom = [&maze](int cx, int cy) {
            return (maze.getCell(cx, cy) == Maze::CELL_ROOM) && maze.isVisible(cx, cy);
          };

          int mask = 0;
          if (isVisibleRoom(x, y - 1)) mask += 1; // Top
          if (isVisibleRoom(x + 1, y)) mask += 2; // Right
          if (isVisibleRoom(x, y + 1)) mask += 4; // Bottom
          if (isVisibleRoom(x - 1, y)) mask += 8; // Left

          Vector2 tilePos = tileMap[mask];
          Rectangle sourceRectMask = {tilePos.x * 16.0f, tilePos.y * 16.0f, 16.0f, 16.0f};

          int belowCell = maze.getCell(x, y + 1);
          // Zelda Top Walls ONLY apply in Rooms when the floor below is visible!
          bool belowIsVisibleFloor =
              maze.isVisible(x, y + 1) && (belowCell == Maze::CELL_ROOM);

          if (belowIsVisibleFloor) {
            // Zelda Top Wall: Project UPWARDS into the void!
            // Draw bitmasked roof edge at y-2 (which happens to be {2,2} for mask 4)
            Rectangle destRectRoof = {(float)(x * cellSize),
                                      (float)((y - 2) * cellSize),
                                      (float)cellSize, (float)cellSize};
            DrawTexturePro(m_wallTileset, sourceRectMask, destRectRoof, {0, 0},
                           0.0f, WHITE);

            // Draw upper wallpaper at y-1
            Rectangle sourceRectTop = {5.0f * 16.0f, 4.0f * 16.0f, 16.0f, 16.0f};
            Rectangle destRectTop = {(float)(x * cellSize),
                                     (float)((y - 1) * cellSize),
                                     (float)cellSize, (float)cellSize};
            DrawTexturePro(m_wallTileset, sourceRectTop, destRectTop, {0, 0},
                           0.0f, WHITE);

            // Draw base wallpaper at y
            Rectangle sourceRectBot = {5.0f * 16.0f, 6.0f * 16.0f, 16.0f, 16.0f};
            Rectangle destRectBot = {(float)(x * cellSize),
                                     (float)(y * cellSize), (float)cellSize,
                                     (float)cellSize};
            DrawTexturePro(m_wallTileset, sourceRectBot, destRectBot, {0, 0},
                           0.0f, WHITE);
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
        Color drawColor;
        bool isTexture = false;
        Rectangle sourceRect = {0};

        if (cell == Maze::CELL_CORRIDOR || cell == Maze::CELL_ROOM) {
          drawColor = maze.isShiftingZone(x, y) ? (Color){255, 100, 100, 255} : WHITE;
          isTexture = true;
          sourceRect = {9.0f * 16.0f, 0.0f * 16.0f, 16.0f, 16.0f};
        } else {
          drawColor = MAGENTA;
        }

        if (isTexture) {
          Rectangle destRect = {(float)(x * cellSize),
                                (float)(y * cellSize), (float)cellSize,
                                (float)cellSize};
          DrawTexturePro(m_floorTileset, sourceRect, destRect, {0, 0}, 0.0f,
                         drawColor);
        } else {
          DrawRectangle(x * cellSize, y * cellSize, cellSize, cellSize,
                        drawColor);
        }
      }
    }
  }
}
