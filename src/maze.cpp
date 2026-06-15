#include "maze.hpp"
#include "raylib.h"
#include <cmath>
#include <algorithm>

// ============================================================================
// Constructor
// ============================================================================
Maze::Maze(int width, int height, int cellSize, unsigned int seed)
    : m_width(width), m_height(height), m_cellSize(cellSize),
      m_nonWallCount(0), m_corridorCount(0), m_grid(width * height, CELL_WALL)
{}

// ============================================================================
// getIndex — The heart of 1D-to-2D Mapping
// ============================================================================
int Maze::getIndex(int x, int y) const {
  // --- TOROIDAL MATH (MODULO WRAPPING) ---
  // In C++, the '%' operator is a remainder, not a true modulo for negative numbers.
  // For example, -1 % 250 = -1. We want -1 to wrap to 249.
  // Formula: (value % MAX + MAX) % MAX guarantees a positive wrapped index.
  int wrappedX = (x % m_width + m_width) % m_width;
  int wrappedY = (y % m_height + m_height) % m_height;
  
  return wrappedY * m_width + wrappedX;
}

// ============================================================================
// getCell — Read a Cell
// ============================================================================
int Maze::getCell(int x, int y) const {
  return m_grid[getIndex(x, y)];
}

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

  if (oldType == CELL_FLOOR && cellType != CELL_FLOOR) {
    m_corridorCount--;
  } else if (oldType != CELL_FLOOR && cellType == CELL_FLOOR) {
    m_corridorCount++;
  }

  m_grid[index] = cellType;
}

// ============================================================================
// RENDERING
// ============================================================================
void Maze::render(const Camera2D& camera) const {
  // --- FRUSTUM CULLING ---
  // 1. Calculate the screen bounds in world space
  Vector2 topLeft = GetScreenToWorld2D({0.0f, 0.0f}, camera);
  Vector2 bottomRight = GetScreenToWorld2D({(float)GetScreenWidth(), (float)GetScreenHeight()}, camera);

  // 2. Convert world coordinates to grid tiles
  // We subtract/add 1 tile to the edges to act as a buffer so we don't see tiles popping in
  int startX = (int)floor(topLeft.x / m_cellSize) - 1;
  int endX   = (int)ceil(bottomRight.x / m_cellSize) + 1;
  
  int startY = (int)floor(topLeft.y / m_cellSize) - 1;
  int endY   = (int)ceil(bottomRight.y / m_cellSize) + 1;

  // 3. Render ONLY the tiles inside the camera frustum!
  for (int y = startY; y <= endY; ++y) {
    for (int x = startX; x <= endX; ++x) {
      // Modulo Wrapping perfectly fetches the right data even if x or y is negative or > width
      int cell = getCell(x, y);

      Color drawColor;
      switch (cell) {
        case CELL_WALL:  
          drawColor = Color{40, 40, 45, 255}; 
          break;      
        case CELL_FLOOR: 
          drawColor = isShiftingZone(x, y) ? Color{255, 100, 100, 255} : Color{180, 170, 140, 255}; 
          break;   
        case CELL_ROOM:  
          drawColor = isShiftingZone(x, y) ? Color{200, 50, 50, 255} : Color{220, 210, 180, 255}; 
          break;   
        default:         
          drawColor = MAGENTA; 
          break; 
      }

      // We draw the tile visually at the absolute world coordinate (x * m_cellSize),
      // even if x is 50000. It seamlessly creates an infinite plane!
      DrawRectangle(x * m_cellSize, y * m_cellSize, m_cellSize, m_cellSize, drawColor);
    }
  }
}

// ============================================================================
// PHASE 2.3: RUBIK'S TORUS
// ============================================================================

void Maze::clearShiftingZones() {
  m_shiftingZones.clear();
}

void Maze::addShiftingZone(int x, int y, int w, int h) {
  m_shiftingZones.push_back({x, y, w, h});
}

bool Maze::isShiftingZone(int x, int y) const {
  int wrappedX = (x % m_width + m_width) % m_width;
  int wrappedY = (y % m_height + m_height) % m_height;
  for (const auto& zone : m_shiftingZones) {
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
  auto it = std::remove_if(m_rooms.begin(), m_rooms.end(), [startX, startY, width, height](const Room& r) {
    bool outsideX = (r.x + r.width <= startX) || (r.x >= startX + width);
    bool outsideY = (r.y + r.height <= startY) || (r.y >= startY + height);
    return !(outsideX || outsideY); // It intersects if it is NOT completely outside
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
    if (tDiag == Maze::CELL_FLOOR || tDiag == Maze::CELL_ROOM) {
      int t1 = getCell(nx, y);
      int t2 = getCell(x, ny);
      if (t1 == Maze::CELL_WALL && t2 == Maze::CELL_WALL) {
        return true;
      }
    }
  }
  return false;
}
