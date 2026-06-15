#pragma once // Tells the compiler: only include this file once, even if
             // multiple files #include it.

#include <vector> // For std::vector (our dynamic 1D array)

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
  static constexpr int CELL_WALL = 0; // Solid wall — player cannot pass through
  static constexpr int CELL_FLOOR = 1; // Open floor — player can walk here
  static constexpr int CELL_ROOM =
      2; // Room floor — part of a BSP-carved office room

  // --- Constructor ---
  // Creates a maze of the given dimensions.
  // seed determines the random layout. Same seed = same maze.
  Maze(int width, int height, int cellSize = 32, unsigned int seed = 12345);



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

  // --- Rendering ---

  // Draw the maze as colored rectangles (placeholder tiles).
  // Only draws cells visible within the camera viewport for performance.
  void render() const;

  // --- Public Dimension Getters ---
  int getWidth() const { return m_width; }
  int getHeight() const { return m_height; }
  int getCellSize() const { return m_cellSize; }

  // --- Real-time Statistics ---
  // Returns the total number of cells that are NOT walls (O(1) time).
  int getNonWallCount() const { return m_nonWallCount; }
  // Returns the total number of corridor floor cells (O(1) time).
  int getCorridorCount() const { return m_corridorCount; }

  struct Room {
    int x, y, width, height;
  };

  void addRoom(const Room& room) { m_rooms.push_back(room); }
  const std::vector<Room>& getRooms() const { return m_rooms; }

private:
  int m_width;    // Number of cells horizontally
  int m_height;   // Number of cells vertically
  int m_cellSize; // Size of each cell in pixels (default 32x32)

  // Real-time stat tracking
  int m_nonWallCount;
  int m_corridorCount;

  // THE 1D FLAT ARRAY
  std::vector<int> m_grid;

  // List of all carved rooms
  std::vector<Room> m_rooms;


};
