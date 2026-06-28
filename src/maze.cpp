#include "maze.hpp"
#include "raylib.h"
#include <algorithm>
#include <cmath>
#include <queue>

// ============================================================================
// Constructor
// ============================================================================
Maze::Maze(int width, int height, int cellSize, unsigned int seed)
    : m_width(width), m_height(height), m_cellSize(cellSize), m_nonWallCount(0),
      m_corridorCount(0), m_grid(width * height, CELL_WALL),
      m_visible(width * height, false) {
}

Maze::~Maze() {
}

// ============================================================================
// getIndex — The heart of 1D-to-2D Mapping
// ============================================================================
int Maze::getIndex(int x, int y) const {
  // --- TOROIDAL MATH (MODULO WRAPPING) ---
  // In C++, the '%' operator is a remainder, not a true modulo for negative
  // numbers. For example, -1 % 250 = -1. We want -1 to wrap to 249. Formula:
  // (value % MAX + MAX) % MAX guarantees a positive wrapped index.
  int wrappedX = (x % m_width + m_width) % m_width;
  int wrappedY = (y % m_height + m_height) % m_height;

  return wrappedY * m_width + wrappedX;
}

// ============================================================================
// getCell — Read a Cell
// ============================================================================
int Maze::getCell(int x, int y) const { return m_grid[getIndex(x, y)]; }

// ============================================================================
// setCell — Write a Cell
// ============================================================================
void Maze::setCell(int x, int y, int cellType) {
  int index = getIndex(x, y);
  int oldType = m_grid[index];

  if (oldType == CELL_WALL && cellType != CELL_WALL) {
    m_nonWallCount++;
  } else if (oldType != CELL_WALL && cellType == CELL_WALL) {
    m_nonWallCount--;
  }

  if (oldType == CELL_CORRIDOR && cellType != CELL_CORRIDOR) {
    m_corridorCount--;
  } else if (oldType != CELL_CORRIDOR && cellType == CELL_CORRIDOR) {
    m_corridorCount++;
  }

  m_grid[index] = cellType;
}

// ============================================================================
// FIELD OF VIEW (Flood Fill)
// ============================================================================
void Maze::updateFOV(Vector2 playerPos, AreaState state) {
  // Clear the visibility map every frame
  std::fill(m_visible.begin(), m_visible.end(), false);

  int playerGridX = (int)floor(playerPos.x / m_cellSize);
  int playerGridY = (int)floor(playerPos.y / m_cellSize);

  std::queue<std::pair<int, int>> q;
  q.push({playerGridX, playerGridY});
  m_visible[getIndex(playerGridX, playerGridY)] = true;

  // Max distance prevents flood fill from lighting up the whole map if
  // corridors connect
  int maxDistance = 25;

  // 8-way Flood Fill
  int dx[] = {0, 0, -1, 1, -1, 1, -1, 1};
  int dy[] = {-1, 1, 0, 0, -1, -1, 1, 1};

  while (!q.empty()) {
    auto [x, y] = q.front();
    q.pop();

    // Manhattan distance approx to stop unbounded filling
    if (std::abs(x - playerGridX) + std::abs(y - playerGridY) > maxDistance)
      continue;

    for (int i = 0; i < 8; ++i) {
      int nx = x + dx[i];
      int ny = y + dy[i];
      int nIndex = getIndex(nx, ny);

      if (!m_visible[nIndex]) {
        m_visible[nIndex] = true;

        // If it is a floor matching our current context, keep expanding!
        int cell = getCell(nx, ny);
        if (state == AreaState::CORRIDOR && cell == CELL_CORRIDOR) {
          q.push({nx, ny});
        } else if (state == AreaState::ROOM && cell == CELL_ROOM) {
          q.push({nx, ny});
        }
      }
    }
  }
}

bool Maze::isVisible(int x, int y) const {
  return m_visible[getIndex(x, y)];
}

// ============================================================================
// PHASE 2.3: RUBIK'S TORUS
// ============================================================================

void Maze::clearShiftingZones() { m_shiftingZones.clear(); }

void Maze::addShiftingZone(int x, int y, int w, int h) {
  m_shiftingZones.push_back({x, y, w, h});
}

bool Maze::isShiftingZone(int x, int y) const {
  int wrappedX = (x % m_width + m_width) % m_width;
  int wrappedY = (y % m_height + m_height) % m_height;
  for (const auto &zone : m_shiftingZones) {
    if (wrappedX >= zone.x && wrappedX < zone.x + zone.width &&
        wrappedY >= zone.y && wrappedY < zone.y + zone.height) {
      return true;
    }
  }
  return false;
}

void Maze::eraseZone(int startX, int startY, int width, int height) {
  // Overwrite cells with solid rock
  for (int y = startY; y < startY + height; ++y) {
    for (int x = startX; x < startX + width; ++x) {
      setCell(x, y, CELL_WALL);
    }
  }

  // Remove any rooms that intersect this 2D zone
  auto it = std::remove_if(
      m_rooms.begin(), m_rooms.end(),
      [startX, startY, width, height](const Room &r) {
        bool outsideX = (r.x + r.width <= startX) || (r.x >= startX + width);
        bool outsideY = (r.y + r.height <= startY) || (r.y >= startY + height);
        return !(outsideX ||
                 outsideY); // It intersects if it is NOT completely outside
      });
  m_rooms.erase(it, m_rooms.end());
}

bool Maze::hasDiagonalLeak(int x, int y) const {
  const int ddx[] = {-1, 1, -1, 1};
  const int ddy[] = {-1, -1, 1, 1};
  for (int i = 0; i < 4; ++i) {
    int nx = x + ddx[i];
    int ny = y + ddy[i];
    int tDiag = getCell(nx, ny);
    if (tDiag == Maze::CELL_CORRIDOR || tDiag == Maze::CELL_ROOM) {
      int t1 = getCell(nx, y);
      int t2 = getCell(x, ny);
      if (t1 == Maze::CELL_WALL && t2 == Maze::CELL_WALL) {
        return true;
      }
    }
  }
  return false;
}

// ============================================================================
// isValidDoorPlacement - 1-to-1 Door Isolation
// ============================================================================
bool Maze::isValidDoorPlacement(int x, int y) const {
  int roomNeighbors = 0;
  const int dx[] = {1, -1, 0, 0};
  const int dy[] = {0, 0, 1, -1};
  
  for (int d = 0; d < 4; ++d) {
    int nx = x + dx[d];
    int ny = y + dy[d];
    if (nx >= 0 && nx < m_width && ny >= 0 && ny < m_height) {
      if (getCell(nx, ny) == CELL_ROOM) {
        roomNeighbors++;
      }
    }
  }
  // Rule 1: A corridor cannot border > 1 room cell (prevents inner corner doors)
  if (roomNeighbors > 1) return false;

  // Rule 2: A room cell cannot border > 1 corridor cell (prevents adjacent doors)
  for (int d = 0; d < 4; ++d) {
    int nx = x + dx[d];
    int ny = y + dy[d];
    if (nx >= 0 && nx < m_width && ny >= 0 && ny < m_height) {
      if (getCell(nx, ny) == CELL_ROOM) {
        int currentCorridorNeighbors = 0;
        for (int dd = 0; dd < 4; ++dd) {
          int nnx = nx + dx[dd];
          int nny = ny + dy[dd];
          if (nnx >= 0 && nnx < m_width && nny >= 0 && nny < m_height) {
            if (getCell(nnx, nny) == CELL_CORRIDOR) {
              currentCorridorNeighbors++;
            }
          }
        }
        if (currentCorridorNeighbors >= 1) return false;
      }
    }
  }
  return true;
}
