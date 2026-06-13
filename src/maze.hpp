#pragma once // Tells the compiler: only include this file once, even if multiple files #include it.

#include <vector>  // For std::vector (our dynamic 1D array)
#include <cstdint> // For fixed-width integer types (uint8_t)
#include "raylib.h" // For Vector2, Rectangle, Color, and drawing functions

// ============================================================================
// Maze Class
// ============================================================================
// Represents the entire maze as an implicit 2D grid graph stored in a
// contiguous 1D flat array for maximum cache locality.
//
// Key Concept: IMPLICIT GRAPH
// We don't store explicit Node objects with pointers to neighbors.
// Instead, for any cell at position (x, y), we KNOW its neighbors are at
// (x+1, y), (x-1, y), (x, y+1), (x, y-1) — the edges are implied by
// the grid geometry. This is much faster and uses far less memory.
// ============================================================================

class Maze {
public:
    // --- Cell Type Constants ---
    // These are the integer values stored in our 1D grid array.
    // Each cell in the maze is one of these types.
    static constexpr int CELL_WALL  = 0; // Solid wall — player cannot pass through
    static constexpr int CELL_FLOOR = 1; // Open floor — player can walk here
    static constexpr int CELL_ROOM  = 2; // Room floor — part of a BSP-carved office room

    // --- Constructor ---
    // Creates a maze of the given dimensions (in cells, not pixels).
    // All cells start as CELL_WALL (solid rock) before generation carves them out.
    Maze(int width, int height, int cellSize = 32);

    // --- Core Accessors ---

    // Convert 2D grid coordinates (x, y) to a 1D array index.
    // Formula: index = y * width + x
    // This is the heart of our 1D-to-2D mapping.
    int getIndex(int x, int y) const;

    // Get the cell type at grid position (x, y).
    // Returns CELL_WALL, CELL_FLOOR, or CELL_ROOM.
    int getCell(int x, int y) const;

    // Set the cell type at grid position (x, y).
    void setCell(int x, int y, int cellType);

    // Check if a grid coordinate is within the maze bounds.
    bool isInBounds(int x, int y) const;

    // --- Rendering ---

    // Draw the maze as colored rectangles (placeholder tiles).
    // Only draws cells visible within the camera viewport for performance.
    void render() const;

    // --- Public Dimension Getters ---
    int getWidth()    const { return m_width; }
    int getHeight()   const { return m_height; }
    int getCellSize() const { return m_cellSize; }

private:
    int m_width;    // Number of cells horizontally
    int m_height;   // Number of cells vertically
    int m_cellSize; // Size of each cell in pixels (default 32x32)

    // THE 1D FLAT ARRAY
    // This single contiguous block of memory holds every cell in the maze.
    // A 100x100 maze = 10,000 integers, all sitting next to each other in RAM.
    // We access cell (x, y) using: m_grid[y * m_width + x]
    std::vector<int> m_grid;
};
