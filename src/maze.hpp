#pragma once // Tells the compiler: only include this file once, even if
             // multiple files #include it.

#include <memory> // For std::shared_ptr (memory management for BSP tree)
#include <random> // For std::mt19937 (random number generator)
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

  // --- Maze Generation Algorithms ---
  // Executes the Binary Space Partitioning algorithm to carve rooms.
  void generateBSP();

  // Grows organic corridors between the BSP rooms using Randomized Prim's Algorithm.
  // This works like mold spreading through rock — it picks random walls on the
  // "frontier" (the boundary between carved space and solid rock) and smashes them
  // open, growing outward until every room is connected.
  void generateCorridors();

  // Breaks the "perfect maze" structure by randomly smashing walls to create
  // loops and cycles. This makes the maze confusing and prevents the player
  // from simply following one wall to find the exit.
  void generateLoops();

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
  int getWidth() const { return m_width; }
  int getHeight() const { return m_height; }
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

  // --- BSP Generation State ---
  // Our Random Number Generator. By storing it here, we guarantee
  // the maze generation is deterministic based on the initial seed.
  std::mt19937 m_rng;

  // A block of "cookie dough" in the BSP algorithm.
  struct BSPLeaf {
    int x, y, width, height; // The rectangular area this leaf owns
    std::shared_ptr<BSPLeaf> leftChild;
    std::shared_ptr<BSPLeaf> rightChild;

    // Coordinates of the actual carved room (if this leaf has one)
    int roomX, roomY, roomWidth, roomHeight;
    bool hasRoom = false;

    BSPLeaf(int _x, int _y, int _w, int _h)
        : x(_x), y(_y), width(_w), height(_h) {}

    // Slices the leaf in half. Returns true if successful.
    bool split(std::mt19937 &rng);

    // Recursively carves actual rooms into the Maze
    void createRooms(Maze &maze, std::mt19937 &rng);
  };

  // The root of our BSP tree
  std::shared_ptr<BSPLeaf> m_rootLeaf;

  // Keep track of all leaves so we can easily iterate over them later
  std::vector<std::shared_ptr<BSPLeaf>> m_leaves;

  // After BSP carves isolated rooms, this pass occasionally smashes the thin
  // walls between nearby rooms to create larger, combined L-shaped formations.
  void mergeAdjacentRooms();
};
