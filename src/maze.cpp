#include "maze.hpp"

// ============================================================================
// Constructor
// ============================================================================
// Creates a width x height grid where every cell starts as a solid wall.
// Think of it like starting with a massive block of solid rock before we
// carve rooms and corridors out of it.
//
// The 1D array is sized to hold width * height integers.
// For a 100x100 maze, that's 10,000 integers = ~40 KB of contiguous memory.
Maze::Maze(int width, int height, int cellSize)
    : m_width(width)
    , m_height(height)
    , m_cellSize(cellSize)
    , m_grid(width * height, CELL_WALL)  // Fill every cell with CELL_WALL (0)
{
}

// ============================================================================
// getIndex — The Heart of 1D-to-2D Mapping
// ============================================================================
// Converts a 2D coordinate (x, y) into a 1D array index.
//
// Imagine our grid laid out like rows of a spreadsheet:
//   Row 0 (y=0): indices [0, 1, 2, 3, 4]       (if width = 5)
//   Row 1 (y=1): indices [5, 6, 7, 8, 9]
//   Row 2 (y=2): indices [10, 11, 12, 13, 14]
//
// To get to row y, we skip y full rows of 'width' cells each: y * width
// Then we move x cells to the right within that row: + x
//
// Example: getIndex(3, 2) on a width=5 grid
//   = 2 * 5 + 3
//   = 10 + 3
//   = 13  ← That's the correct index in our 1D array!
int Maze::getIndex(int x, int y) const {
    return y * m_width + x;
}

// ============================================================================
// getCell — Read a Cell
// ============================================================================
// Returns the cell type (CELL_WALL, CELL_FLOOR, or CELL_ROOM) at position (x, y).
// If the coordinates are out of bounds, we return CELL_WALL as a safety measure
// so anything outside the grid acts like a solid wall.
int Maze::getCell(int x, int y) const {
    if (!isInBounds(x, y)) {
        return CELL_WALL; // Out-of-bounds = treat as wall
    }
    return m_grid[getIndex(x, y)];
}

// ============================================================================
// setCell — Write a Cell
// ============================================================================
// Changes the cell type at position (x, y).
// This is how the generation algorithms will "carve" floor out of solid rock:
//   maze.setCell(5, 10, Maze::CELL_FLOOR);  // Turn cell (5,10) into walkable floor
void Maze::setCell(int x, int y, int cellType) {
    if (!isInBounds(x, y)) {
        return; // Silently ignore out-of-bounds writes
    }
    m_grid[getIndex(x, y)] = cellType;
}

// ============================================================================
// isInBounds — Boundary Check
// ============================================================================
// Returns true if (x, y) is inside the grid.
// x must be in range [0, width)  and y must be in range [0, height).
// The parenthesis ')' means exclusive — e.g., for width=100, valid x is 0 to 99.
bool Maze::isInBounds(int x, int y) const {
    return x >= 0 && x < m_width && y >= 0 && y < m_height;
}

// ============================================================================
// render — Draw the Maze
// ============================================================================
// Loops through every cell in the grid and draws a colored rectangle.
// 
// PERFORMANCE NOTE: Right now, this draws EVERY cell in the maze every frame.
// For a 100x100 grid that's 10,000 rectangles per frame. This is fine for now,
// but later we will optimize this to only draw cells visible in the camera
// viewport (called "frustum culling").
//
// Color coding (placeholder):
//   CELL_WALL  → Dark grey (solid rock)
//   CELL_FLOOR → Light yellow (the classic Backrooms carpet color)
//   CELL_ROOM  → Slightly different yellow (to distinguish office rooms)
void Maze::render() const {
    for (int y = 0; y < m_height; y++) {
        for (int x = 0; x < m_width; x++) {
            int cellType = m_grid[getIndex(x, y)];

            Color color;
            switch (cellType) {
                case CELL_FLOOR:
                    color = Color{ 194, 178, 128, 255 }; // Sandy yellow — corridor floor
                    break;
                case CELL_ROOM:
                    color = Color{ 210, 190, 140, 255 };  // Slightly lighter — office room
                    break;
                case CELL_WALL:
                default:
                    color = Color{ 40, 35, 30, 255 };     // Dark brown-grey — solid wall
                    break;
            }

            // Draw a filled rectangle at the correct pixel position.
            // Grid cell (x, y) maps to pixel position (x * cellSize, y * cellSize).
            DrawRectangle(
                x * m_cellSize,    // Pixel X position
                y * m_cellSize,    // Pixel Y position
                m_cellSize,        // Width in pixels
                m_cellSize,        // Height in pixels
                color              // Fill color
            );
        }
    }
}
