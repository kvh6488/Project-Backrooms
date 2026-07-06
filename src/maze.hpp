#pragma once
#include <map>
#include <raylib.h>
#include <vector>


enum class AreaState { CORRIDOR, ROOM };

// ============================================================================
// ItemType Enum — Grid-Parallel Item Layer
// ============================================================================
// Each cell in the maze can hold at most one item. This enum encodes the
// item type using integer codes stored in a flat 1D array (m_items) that
// mirrors m_grid. This gives us O(1) lookup for rendering and collision:
//   maze.getItem(x, y)  →  instant answer, no list scanning.
//
// Alternative considered: Separate std::vector per item type (m_barrels,
// m_mushrooms, etc.). Rejected because rendering would require iterating
// every list for every visible cell — O(items) per cell vs O(1).
// ============================================================================
enum class ItemType : int {
  NONE = 0,          // Empty cell — no item present
  BARREL = 1,        // Radiation barrel — emits BFS radiation zones
  MUSHROOM = 2,      // Mushroom — spawns only in normal rooms
  MAGIC_MUSHROOM = 3 // Magic Mushroom — grants special ability, spawns only in
                     // radiated rooms
};

// Forward declarations
class Player;
enum class FacingDirection;

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
  static constexpr int CELL_CORRIDOR = 1; // Open floor — player can walk here
  static constexpr int CELL_ROOM =
      2; // Room floor — part of a BSP-carved office room

  // --- Constructor ---
  // Creates a maze of the given dimensions.
  // seed determines the random layout. Same seed = same maze.
  Maze(int width, int height, int cellSize = 32, unsigned int seed = 12345);
  ~Maze();

  // --- Core Accessors ---

  // Convert 2D grid coordinates (x, y) to a 1D array index.
  // Formula: index = y * width + x
  // This is the heart of our 1D-to-2D mapping.
  int getIndex(int x, int y) const;

  // Get the cell type at grid position (x, y).
  // Returns CELL_WALL, CELL_CORRIDOR, or CELL_ROOM.
  int getCell(int x, int y) const;

  // Set the cell type at grid position (x, y).
  void setCell(int x, int y, int cellType);

  // ============================================================================
  // Recursive Shadowcasting FOV
  // ============================================================================
  // Computes line-of-sight visibility from the player's grid position.
  // In CORRIDOR mode: restricts to a forward-facing cone of `radius` tiles,
  //   using recursive shadowcasting to produce true LOS with wall shadows.
  // In ROOM mode: marks the entire room as fully visible (always lit).
  //
  // This is the 2D roguelike gold standard for FOV. It divides the circle
  // around the player into 8 octants and recursively scans each row outward.
  // When it encounters a wall, it splits the visible arc and recurses on the
  // remaining visible portion — producing perfect shadow geometry with no
  // artifacts. See:
  // http://www.roguebasin.com/index.php/FOV_using_recursive_shadowcasting
  // ============================================================================
  void updateVisibility(int playerX, int playerY, AreaState state);

  // Check if cell is visible (used by MazeRenderer)
  bool isVisible(int x, int y) const;

  // Get the light level at a cell (0.0 = pitch black, 1.0 = fully lit)
  // Used by the renderer to apply smooth distance-based darkening.
  float getLightLevel(int x, int y) const;

  // --- Phase 2.3: Rubik's Torus ---
  // Check if a coordinate is within the designated shifting slice
  bool isShiftingZone(int x, int y) const;

  // Phase 2.3: Zone Regeneration
  void clearShiftingZones();
  void addShiftingZone(int x, int y, int w, int h);
  void eraseZone(int startX, int startY, int width, int height);

  // --- Public Dimension Getters ---
  int getWidth() const { return m_width; }
  int getHeight() const { return m_height; }
  int getCellSize() const { return m_cellSize; }

  // --- Real-time Statistics ---
  // Returns the total number of cells that are NOT walls (O(1) time).
  int getNonWallCount() const { return m_nonWallCount; }
  // Returns the total number of corridor cells (O(1) time).
  int getCorridorCount() const { return m_corridorCount; }

  bool hasDiagonalLeak(int x, int y) const;

  // Enforces 1-to-1 Door Isolation Constraint
  bool isValidDoorPlacement(int x, int y) const;

  struct Room {
    int x, y, width, height;
    bool isRadiated = false;
  };

  void addRoom(const Room &room) { m_rooms.push_back(room); }
  const std::vector<Room> &getRooms() const { return m_rooms; }

  // --- Item Layer (Grid-Parallel) ---
  // Each cell can hold one item. O(1) read/write via the same index math
  // as m_grid. This replaces per-item-type lists (m_barrels, etc.).
  ItemType getItem(int x, int y) const;
  void setItem(int x, int y, ItemType type);

  // Wipe all items in a rectangular zone and return a count of each type
  // that was removed. Used during Tic-Tac-Toe zone regeneration so the
  // ItemSpawner knows exactly what to replenish.
  std::map<ItemType, int> clearItemsInZone(int startX, int startY, int width,
                                           int height);

  // --- Phase 3: Radiation Mechanics ---
  void calculateRadiationZones();
  void destroyBarrelNear(int x, int y, int radius = 1);
  int getRadiationLevel(int x, int y) const;
  bool hasBarrel(int x, int y) const;
  bool isBarrelNear(int x, int y, int radius = 1) const;

private:
  int m_width;    // Number of cells horizontally
  int m_height;   // Number of cells vertically
  int m_cellSize; // Size of each cell in pixels (default 32x32)

  // Real-time stat tracking
  int m_nonWallCount;
  int m_corridorCount;

  // THE 1D FLAT ARRAY
  std::vector<int> m_grid;
  std::vector<bool> m_visible;
  std::vector<float> m_lightLevel; // Per-cell brightness (0.0 to 1.0)
  std::vector<int> m_radiationMap; // 0 for safe, >0 for radiated

  // Item layer — parallel to m_grid. Each index stores the ItemType
  // occupying that cell (NONE if empty). This is the single source of
  // truth for all placed items.
  std::vector<ItemType> m_items;

  // Helper to place walls and floors
  void setRoomTiles(int startX, int startY, int width, int height);

  // List of all carved rooms
  std::vector<Room> m_rooms;

  // List of shifting zones for Minimap rendering
  std::vector<Room> m_shiftingZones;
};
