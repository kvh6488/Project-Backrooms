#include "maze.hpp"
#include "raylib.h"

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
