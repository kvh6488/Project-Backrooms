#include "item_spawner.hpp"

// ============================================================================
// Constructor
// ============================================================================
ItemSpawner::ItemSpawner(std::mt19937& rng) : m_rng(rng) {}

// ============================================================================
// isNearCorridor — Doorway Proximity Check
// ============================================================================
// Returns true if any of the 4 cardinal neighbours of (x, y) is a corridor
// cell. Items should never be placed on these cells because they act as
// doorway transitions between rooms and corridors — placing a barrel here
// would block the only entrance.
//
// Time Complexity: O(1) — always exactly 4 neighbour checks.
// ============================================================================
bool ItemSpawner::isNearCorridor(const Maze& maze, int x, int y) const {
  const int dx[] = {1, -1, 0, 0};
  const int dy[] = {0, 0, 1, -1};
  for (int d = 0; d < 4; ++d) {
    if (maze.getCell(x + dx[d], y + dy[d]) == Maze::CELL_CORRIDOR) {
      return true;
    }
  }
  return false;
}

// ============================================================================
// spawnBarrels — Place Barrels with Placement Rules
// ============================================================================
// Placement rules for radiation barrels:
//   1. Cell must be CELL_ROOM (barrels only appear inside rooms).
//   2. Cell must NOT be adjacent to a CELL_CORRIDOR in any cardinal
//      direction (avoids blocking doorways).
//   3. Cell must currently hold no item (ItemType::NONE).
//
// The method picks random cells within the given bounding box and retries
// up to maxAttempts times per barrel. This is a rejection sampling approach
// — simple, correct, and fast given the density of room cells.
//
// Alternative considered: Pre-collecting all valid cells into a list and
// sampling from it. This would guarantee O(1) placement but costs O(W*H)
// upfront. Rejection sampling is typically faster when valid cells are
// plentiful (which they are in our maze).
// ============================================================================
void ItemSpawner::spawnBarrels(Maze& maze, int count,
                               int boundsX, int boundsY,
                               int boundsW, int boundsH) {
  // Distributions for random cell selection within the bounding box
  std::uniform_int_distribution<int> distX(boundsX, boundsX + boundsW - 1);
  std::uniform_int_distribution<int> distY(boundsY, boundsY + boundsH - 1);

  const int maxAttempts = 100; // Safety cap to avoid infinite loops

  for (int i = 0; i < count; ++i) {
    for (int attempt = 0; attempt < maxAttempts; ++attempt) {
      int x = distX(m_rng);
      int y = distY(m_rng);

      // Rule 1: Must be a room cell
      if (maze.getCell(x, y) != Maze::CELL_ROOM) continue;

      // Rule 2: Must not be adjacent to a corridor (doorway check)
      if (isNearCorridor(maze, x, y)) continue;

      // Rule 3: Must not already contain an item
      if (maze.getItem(x, y) != ItemType::NONE) continue;

      // All rules passed — place the barrel
      maze.setItem(x, y, ItemType::BARREL);
      break;
    }
  }
}

// ============================================================================
// spawnInitialItems — One-Shot World Population
// ============================================================================
// Called once after maze generation. Determines item counts internally
// using the shared RNG, then delegates to per-item-type spawn methods.
//
// This keeps Application completely decoupled from item spawn logic —
// it just calls spawnInitialItems() and the spawner handles everything.
// ============================================================================
void ItemSpawner::spawnInitialItems(Maze& maze) {
  // Barrel count: uniformly random from [3, 5]
  std::uniform_int_distribution<int> barrelCountDist(3, 5);
  int barrelCount = barrelCountDist(m_rng);

  // Spawn barrels across the entire maze
  spawnBarrels(maze, barrelCount, 0, 0, maze.getWidth(), maze.getHeight());

  // Trigger radiation zone calculation now that barrels are placed
  maze.calculateRadiationZones();
}

// ============================================================================
// respawnItems — Zone Regeneration Replenishment
// ============================================================================
// After a shifting zone regenerates, this method replaces the items that
// were destroyed. The itemsToSpawn map (from Maze::clearItemsInZone)
// tells us exactly what types and counts to replenish.
//
// Items are spawned only within the zone boundaries to ensure they land
// in the freshly carved rooms.
// ============================================================================
void ItemSpawner::respawnItems(Maze& maze,
                                const std::map<ItemType, int>& itemsToSpawn,
                                int zoneX, int zoneY,
                                int zoneW, int zoneH) {
  for (const auto& [type, count] : itemsToSpawn) {
    switch (type) {
      case ItemType::BARREL:
        spawnBarrels(maze, count, zoneX, zoneY, zoneW, zoneH);
        break;
      case ItemType::MUSHROOM:
        // Future: spawnMushrooms(maze, count, zoneX, zoneY, zoneW, zoneH);
        break;
      default:
        break;
    }
  }
}
