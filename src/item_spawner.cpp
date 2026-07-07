#include "item_spawner.hpp"

// ============================================================================
// Constructor
// ============================================================================
ItemSpawner::ItemSpawner(std::mt19937 &rng) : m_rng(rng) {}

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
bool ItemSpawner::isNearCorridor(const Maze &maze, int x, int y) const {
  const int dx[] = {1, 1, 1, 0, -1, -1, -1, 0};
  const int dy[] = {1, 0, -1, -1, -1, 0, 1, 1};
  for (int d = 0; d < 8; ++d) {
    if (maze.getCell(x + dx[d], y + dy[d]) == Maze::CELL_CORRIDOR) {
      return true;
    }
  }
  return false;
}

// ============================================================================
// isRoomCorner — Corner Proximity Check
// ============================================================================
// Returns true if the cell has walls adjacent to it on two adjacent sides
// (e.g. Left + Top, Right + Top, Left + Bottom, Right + Bottom).
// ============================================================================
bool ItemSpawner::isRoomCorner(const Maze &maze, int x, int y) const {
  bool leftWall = maze.getCell(x - 1, y) == Maze::CELL_WALL;
  bool rightWall = maze.getCell(x + 1, y) == Maze::CELL_WALL;
  bool topWall = maze.getCell(x, y - 1) == Maze::CELL_WALL;
  bool bottomWall = maze.getCell(x, y + 1) == Maze::CELL_WALL;

  return (leftWall && topWall) || (rightWall && topWall) ||
         (leftWall && bottomWall) || (rightWall && bottomWall);
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
void ItemSpawner::spawnBarrels(Maze &maze, int count, int boundsX, int boundsY,
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
      if (maze.getCell(x, y) != Maze::CELL_ROOM)
        continue;

      // Rule 2: Must not be adjacent to a corridor (doorway check)
      if (isNearCorridor(maze, x, y))
        continue;

      // Rule 3: Must not already contain an item
      if (maze.getItem(x, y) != ItemType::NONE)
        continue;

      // All rules passed — place the barrel
      maze.setItem(x, y, ItemType::TOXIC_WASTE);
      break;
    }
  }
}

// ============================================================================
// spawnMushroomClump — Spreads a clump of mushrooms from a starting cell
// ============================================================================
int ItemSpawner::spawnMushroomClump(Maze &maze, int startX, int startY,
                                    ItemType type) {
  std::uniform_int_distribution<int> sizeDist(2, 5);
  int targetSize = sizeDist(m_rng);
  int placed = 0;

  // We use a candidate list to organically grow the clump
  std::vector<std::pair<int, int>> candidates;
  candidates.push_back({startX, startY});

  while (placed < targetSize && !candidates.empty()) {
    // Pick a random candidate from the list
    std::uniform_int_distribution<int> candDist(0, candidates.size() - 1);
    int idx = candDist(m_rng);
    auto [cx, cy] = candidates[idx];

    // Remove the chosen candidate by swapping it with the last element
    candidates[idx] = candidates.back();
    candidates.pop_back();

    // Verify it is still valid: must be a radiated room cell with no item,
    // and must not be right in front of a door (corridor).
    bool isValidRad = (type == ItemType::MAGIC_MUSHROOM)
                          ? (maze.getRadiationLevel(cx, cy) > 0)
                          : (maze.getRadiationLevel(cx, cy) == 0);
    if (maze.getCell(cx, cy) == Maze::CELL_ROOM &&
        maze.getItem(cx, cy) == ItemType::NONE && isValidRad &&
        !isNearCorridor(maze, cx, cy)) {

      // Place the mushroom
      maze.setItem(cx, cy, type);
      placed++;

      // Add neighbors to candidates list
      const int dx[] = {1, -1, 0, 0};
      const int dy[] = {0, 0, 1, -1};
      for (int i = 0; i < 4; ++i) {
        // Toroidal wrap
        int nx =
            (cx + dx[i] % maze.getWidth() + maze.getWidth()) % maze.getWidth();
        int ny = (cy + dy[i] % maze.getHeight() + maze.getHeight()) %
                 maze.getHeight();
        candidates.push_back({nx, ny});
      }
    }
  }
  return placed;
}

// ============================================================================
// spawnMushrooms — Places mushroom clumps in room corners
// ============================================================================
void ItemSpawner::spawnMushrooms(Maze &maze, ItemType type, int target,
                                 int boundsX, int boundsY, int boundsW,
                                 int boundsH) {
  if (boundsW == -1)
    boundsW = maze.getWidth();
  if (boundsH == -1)
    boundsH = maze.getHeight();

  std::uniform_real_distribution<float> chance(0.0f, 1.0f);
  int totalPlaced = 0;

  // If we have a target count (e.g. respawning), we randomly pick corners
  // inside the bounds and try to spawn clumps until we reach the target.
  if (target > 0) {
    // Collect all valid corners in the zone
    std::vector<std::pair<int, int>> validCorners;
    for (int y = boundsY; y < boundsY + boundsH; ++y) {
      for (int x = boundsX; x < boundsX + boundsW; ++x) {
        bool isValidRad = (type == ItemType::MAGIC_MUSHROOM)
                              ? (maze.getRadiationLevel(x, y) > 0)
                              : (maze.getRadiationLevel(x, y) == 0);
        if (maze.getCell(x, y) == Maze::CELL_ROOM &&
            maze.getItem(x, y) == ItemType::NONE && isValidRad &&
            isRoomCorner(maze, x, y) && !isNearCorridor(maze, x, y)) {
          validCorners.push_back({x, y});
        }
      }
    }

    if (validCorners.empty())
      return;

    std::uniform_int_distribution<int> cornerDist(0, validCorners.size() - 1);
    int attempts = 0;
    while (totalPlaced < target && attempts < 100) { // Safety break
      auto [cx, cy] = validCorners[cornerDist(m_rng)];
      if (maze.getItem(cx, cy) == ItemType::NONE) {
        totalPlaced += spawnMushroomClump(maze, cx, cy, type);
      }
      attempts++;
    }
  }
  // Initial generation: scan all bounds and apply 6% chance to each corner
  else {
    for (int y = boundsY; y < boundsY + boundsH; ++y) {
      for (int x = boundsX; x < boundsX + boundsW; ++x) {
        bool isValidRad = (type == ItemType::MAGIC_MUSHROOM)
                              ? (maze.getRadiationLevel(x, y) > 0)
                              : (maze.getRadiationLevel(x, y) == 0);
        if (maze.getCell(x, y) == Maze::CELL_ROOM &&
            maze.getItem(x, y) == ItemType::NONE && isValidRad &&
            isRoomCorner(maze, x, y) && !isNearCorridor(maze, x, y)) {

          if (chance(m_rng) < 0.3f) {
            totalPlaced += spawnMushroomClump(maze, x, y, type);
          }
        }
      }
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
void ItemSpawner::spawnInitialItems(Maze &maze) {
  // Barrel count: uniformly random from [3, 5]
  std::uniform_int_distribution<int> barrelCountDist(3, 5);
  int barrelCount = barrelCountDist(m_rng);

  // Spawn barrels across the entire maze
  spawnBarrels(maze, barrelCount, 0, 0, maze.getWidth(), maze.getHeight());

  // Trigger radiation zone calculation now that barrels are placed
  maze.calculateRadiationZones();

  // Mushrooms require radiation info
  spawnMushrooms(maze, ItemType::MAGIC_MUSHROOM);
  spawnMushrooms(maze, ItemType::MUSHROOM);
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
void ItemSpawner::respawnItems(Maze &maze,
                               const std::map<ItemType, int> &itemsToSpawn,
                               int zoneX, int zoneY, int zoneW, int zoneH) {
  // First, spawn barrels to establish radiation zones
  auto itBarrel = itemsToSpawn.find(ItemType::TOXIC_WASTE);
  if (itBarrel != itemsToSpawn.end()) {
    spawnBarrels(maze, itBarrel->second, zoneX, zoneY, zoneW, zoneH);
  }

  // Recalculate radiation zones before spawning mushrooms
  maze.calculateRadiationZones();

  // Then spawn other items
  for (const auto &[type, count] : itemsToSpawn) {
    if (type == ItemType::TOXIC_WASTE) continue;
    
    switch (type) {
    case ItemType::MUSHROOM:
      spawnMushrooms(maze, ItemType::MUSHROOM, count, zoneX, zoneY, zoneW,
                     zoneH);
      break;
    case ItemType::MAGIC_MUSHROOM:
      spawnMushrooms(maze, ItemType::MAGIC_MUSHROOM, count, zoneX, zoneY, zoneW,
                     zoneH);
      break;
    default:
      break;
    }
  }
}
