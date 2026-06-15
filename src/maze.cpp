#include "maze.hpp"
#include "raylib.h"

// ============================================================================
// Constructor
// ============================================================================
Maze::Maze(int width, int height, int cellSize, unsigned int seed)
    : m_width(width), m_height(height), m_cellSize(cellSize),
      m_nonWallCount(0), m_grid(width * height, CELL_WALL)
{}

// ============================================================================
// getIndex — The heart of 1D-to-2D Mapping
// ============================================================================
int Maze::getIndex(int x, int y) const { return y * m_width + x; }

// ============================================================================
// getCell — Read a Cell
// ============================================================================
int Maze::getCell(int x, int y) const {
  if (!isInBounds(x, y)) {
    return CELL_WALL; // Out-of-bounds = treat as wall
  }
  return m_grid[getIndex(x, y)];
}

// ============================================================================
// setCell — Write a Cell
// ============================================================================
void Maze::setCell(int x, int y, int cellType) {
  if (!isInBounds(x, y)) {
    return; // Silently ignore out-of-bounds writes
  }
  
  int index = getIndex(x, y);
  int oldType = m_grid[index];
  
  if (oldType == CELL_WALL && cellType != CELL_WALL) {
    m_nonWallCount++;
  } else if (oldType != CELL_WALL && cellType == CELL_WALL) {
    m_nonWallCount--;
  }

  m_grid[index] = cellType;
}

// ============================================================================
// isInBounds — Boundary Check
// ============================================================================
bool Maze::isInBounds(int x, int y) const {
  return x >= 0 && x < m_width && y >= 0 && y < m_height;
}

// ============================================================================
// RENDERING
// ============================================================================
void Maze::render() const {
  // We use Raylib's colors
  Color colorWall = Color{40, 40, 45, 255};      // Dark grey rock
  Color colorRoom = Color{220, 210, 180, 255};   // Creamy office carpet
  Color colorCorridor = Color{180, 170, 140, 255}; // Slightly darker floor

  // Render the grid
  for (int y = 0; y < m_height; ++y) {
    for (int x = 0; x < m_width; ++x) {
      int cell = getCell(x, y);

      Color drawColor = colorWall; // Default to wall
      if (cell == CELL_ROOM)
        drawColor = colorRoom;
      else if (cell == CELL_FLOOR)
        drawColor = colorCorridor;

      // Draw the rectangle using Raylib
      DrawRectangle(x * m_cellSize, y * m_cellSize, m_cellSize, m_cellSize,
                    drawColor);
    }
  }
}
