#include "prims_generator.hpp"
#include <queue>
#include <set>

// ============================================================================
// PRIM'S ALGORITHM IMPLEMENTATION (Corridor Generation)
// ============================================================================
void PrimsGenerator::generate(Maze &maze, std::mt19937 &rng,
                              int startRoomIndex) {
  int totalCells = maze.getWidth() * maze.getHeight();
  int triggerThreshold = static_cast<int>(totalCells * 0.05f); // 5% trigger
  int targetCoverage =
      static_cast<int>(totalCells * 0.25f); // 25% coverage target

  // Initial run from the center
  int carvedCount = carveFromRoom(maze, rng, startRoomIndex);

  // Fallback Check: If we got blocked early and covered less than 5% of the
  // map, we pick random rooms to seed from and run the algorithm continuously
  // until we reach at least 25% coverage.
  if (carvedCount < triggerThreshold && !maze.getRooms().empty()) {
    int maxAttempts = 50;
    int attempts = 0;
    while (carvedCount < targetCoverage && attempts < maxAttempts) {
      int randomRoomIndex = std::uniform_int_distribution<>(
          0, (int)maze.getRooms().size() - 1)(rng);
      carvedCount += carveFromRoom(maze, rng, randomRoomIndex);
      attempts++;
    }
  }

  const auto &rooms = maze.getRooms();

  // --- STEP 3: Punch Doorways ---
  // Prim's algorithm specifically avoids carving into existing carved areas to
  // prevent loops. Because of this, it naturally wraps *around* the BSP rooms
  // but never actually connects to them! We must do a final pass around the
  // perimeter of every room and forcefully punch a door to connect the room to
  // the corridor network.
  const int dx[] = {1, -1, 0, 0};
  const int dy[] = {0, 0, 1, -1};
  for (const auto &room : rooms) {
    // Collect all viable spots for a door (must have rock on the perimeter, and
    // floor right outside it)
    std::vector<std::pair<int, int>> doorCandidates;

    for (int ix = room.x; ix < room.x + room.width; ++ix) {
      int wy = room.y - 1;
      if (maze.getCell(ix, wy) == Maze::CELL_WALL) {
        int rCount = 0, cCount = 0;
        for (int d = 0; d < 4; ++d) {
          int nx = ix + dx[d];
          int ny = wy + dy[d];
          if (maze.getCell(nx, ny) == Maze::CELL_ROOM)
            rCount++;
          if (maze.getCell(nx, ny) == Maze::CELL_CORRIDOR)
            cCount++;
        }
        if (rCount == 1 && cCount == 1) {
          if (!maze.hasDiagonalLeak(ix, wy) &&
              maze.isValidDoorPlacement(ix, wy))
            doorCandidates.push_back({ix, wy});
        }
      }
      wy = room.y + room.height;
      if (maze.getCell(ix, wy) == Maze::CELL_WALL) {
        int rCount = 0, cCount = 0;
        for (int d = 0; d < 4; ++d) {
          int nx = ix + dx[d];
          int ny = wy + dy[d];
          if (maze.getCell(nx, ny) == Maze::CELL_ROOM)
            rCount++;
          if (maze.getCell(nx, ny) == Maze::CELL_CORRIDOR)
            cCount++;
        }
        if (rCount == 1 && cCount == 1) {
          if (!maze.hasDiagonalLeak(ix, wy) &&
              maze.isValidDoorPlacement(ix, wy))
            doorCandidates.push_back({ix, wy});
        }
      }
    }
    for (int iy = room.y; iy < room.y + room.height; ++iy) {
      int wx = room.x - 1;
      if (maze.getCell(wx, iy) == Maze::CELL_WALL) {
        int rCount = 0, cCount = 0;
        for (int d = 0; d < 4; ++d) {
          int nx = wx + dx[d];
          int ny = iy + dy[d];
          if (maze.getCell(nx, ny) == Maze::CELL_ROOM)
            rCount++;
          if (maze.getCell(nx, ny) == Maze::CELL_CORRIDOR)
            cCount++;
        }
        if (rCount == 1 && cCount == 1) {
          if (!maze.hasDiagonalLeak(wx, iy) &&
              maze.isValidDoorPlacement(wx, iy))
            doorCandidates.push_back({wx, iy});
        }
      }
      wx = room.x + room.width;
      if (maze.getCell(wx, iy) == Maze::CELL_WALL) {
        int rCount = 0, cCount = 0;
        for (int d = 0; d < 4; ++d) {
          int nx = wx + dx[d];
          int ny = iy + dy[d];
          if (maze.getCell(nx, ny) == Maze::CELL_ROOM)
            rCount++;
          if (maze.getCell(nx, ny) == Maze::CELL_CORRIDOR)
            cCount++;
        }
        if (rCount == 1 && cCount == 1) {
          if (!maze.hasDiagonalLeak(wx, iy) &&
              maze.isValidDoorPlacement(wx, iy))
            doorCandidates.push_back({wx, iy});
        }
      }
    }

    if (!doorCandidates.empty()) {
      int pick = std::uniform_int_distribution<>(0, (int)doorCandidates.size() -
                                                        1)(rng);
      maze.setCell(doorCandidates[pick].first, doorCandidates[pick].second,
                   Maze::CELL_CORRIDOR);
    }
  }
}

int PrimsGenerator::carveFromRoom(Maze &maze, std::mt19937 &rng,
                                  int startRoomIndex) {
  // 1. The Frontier List: Stores the indices of all rock/wall cells that are
  // adjacent to a carved floor. We will randomly pick from this list to grow
  // the maze.
  std::vector<int> frontier;
  int carvedCount = 0;
  int seedX = -1, seedY = -1;
  const auto &rooms = maze.getRooms();

  // --- STEP 1: Find a Seed Point ---
  // Prim's algorithm needs a starting point (a "seed"). Instead of picking a
  // random rock cell, we want the maze to grow *out* of the BSP rooms. We scan
  // the perimeter of the STARTING room to find a solid rock wall to start
  // carving.

  if (startRoomIndex >= 0 && startRoomIndex < rooms.size()) {
    const auto &room = rooms[startRoomIndex];

    for (int ix = room.x; ix < room.x + room.width; ++ix) {
      if (maze.getCell(ix, room.y - 1) == Maze::CELL_WALL &&
          !maze.hasDiagonalLeak(ix, room.y - 1)) {
        seedX = ix;
        seedY = room.y - 1;
        goto SEED_FOUND;
      }
      if (maze.getCell(ix, room.y + room.height) == Maze::CELL_WALL &&
          !maze.hasDiagonalLeak(ix, room.y + room.height)) {
        seedX = ix;
        seedY = room.y + room.height;
        goto SEED_FOUND;
      }
    }
    for (int iy = room.y; iy < room.y + room.height; ++iy) {
      if (maze.getCell(room.x - 1, iy) == Maze::CELL_WALL &&
          !maze.hasDiagonalLeak(room.x - 1, iy)) {
        seedX = room.x - 1;
        seedY = iy;
        goto SEED_FOUND;
      }
      if (maze.getCell(room.x + room.width, iy) == Maze::CELL_WALL &&
          !maze.hasDiagonalLeak(room.x + room.width, iy)) {
        seedX = room.x + room.width;
        seedY = iy;
        goto SEED_FOUND;
      }
    }
  SEED_FOUND:;
  }

  if (seedX == -1 || seedY == -1)
    return 0;

  // Carve the seed cell to start the maze
  maze.setCell(seedX, seedY, Maze::CELL_CORRIDOR);
  carvedCount++;

  // Cardinal directions: Right, Left, Down, Up
  const int dx[] = {1, -1, 0, 0};
  const int dy[] = {0, 0, 1, -1};

  // Add all solid walls immediately adjacent to the seed into the frontier
  // list. The frontier represents the "edge" of our growing mold.

  for (int d = 0; d < 4; ++d) {
    int nx = seedX + dx[d];
    int ny = seedY + dy[d];
    if (maze.getCell(nx, ny) == Maze::CELL_WALL) {
      frontier.push_back(maze.getIndex(nx, ny));
    }
  }

  return runPrims(maze, rng, frontier);
}

int PrimsGenerator::runPrims(Maze &maze, std::mt19937 &rng,
                             std::vector<int> &frontier) {
  int carvedCount = 0;
  const int dx[] = {1, -1, 0, 0};
  const int dy[] = {0, 0, 1, -1};

  // 2. Main Prim's Loop: Keep going until the frontier is empty.
  while (!frontier.empty()) {
    // Pick a random wall from the frontier. This randomness creates the
    // chaotic, organic mold-like growth characteristic of the Backrooms.
    int randIdx =
        std::uniform_int_distribution<>(0, (int)frontier.size() - 1)(rng);
    int wallIndex = frontier[randIdx];

    // Fast O(1) Removal: Instead of shifting the entire vector down (which is
    // O(N)), we swap the chosen element with the very last element, and then
    // pop the back.
    frontier[randIdx] = frontier.back();
    frontier.pop_back();

    // Convert the 1D index back into 2D grid coordinates
    int wx = wallIndex % maze.getWidth();
    int wy = wallIndex / maze.getWidth();

    // If this cell was somehow already carved (perhaps by another path), skip
    // it
    if (maze.getCell(wx, wy) != Maze::CELL_WALL)
      continue;

    int carvedNeighborCount = 0;
    for (int d = 0; d < 4; ++d) {
      int nx = wx + dx[d];
      int ny = wy + dy[d];
      int neighborType = maze.getCell(nx, ny);
      if (neighborType == Maze::CELL_CORRIDOR ||
          neighborType == Maze::CELL_ROOM) {
        ++carvedNeighborCount;
      }
    }

    // A valid Prim's wall MUST have exactly 1 carved neighbor.
    // If it has 2 or more, breaking it would create a loop, violating the
    // perfect maze tree structure.
    if (carvedNeighborCount != 1)
      continue;

    // --- Diagonal Leak Check ---
    // In grid mazes, carving cells diagonally adjacent can create visual
    // "leaks" where two rooms touch only at the corner, making the wall
    // geometry look broken in 3D.
    if (maze.hasDiagonalLeak(wx, wy))
      continue;

    // All checks passed! Smash the wall and turn it into a floor.
    maze.setCell(wx, wy, Maze::CELL_CORRIDOR);
    carvedCount++;

    // Add all of the newly exposed rock/wall neighbors to the frontier.

    for (int d = 0; d < 4; ++d) {
      int nx = wx + dx[d];
      int ny = wy + dy[d];
      if (maze.getCell(nx, ny) == Maze::CELL_WALL) {
        frontier.push_back(maze.getIndex(nx, ny));
      }
    }
  }

  return carvedCount;
}

// ============================================================================
// ZONE GENERATION — Targeted Corridor Growth & Border Stitching
// ============================================================================
void PrimsGenerator::generateZone(Maze &maze, std::mt19937 &rng, int startX,
                                  int startY, int width, int height) {
  const auto &rooms = maze.getRooms();
  std::vector<int> zoneRooms;

  std::vector<int> frontier;

  int endX = startX + width - 1;
  int endY = startY + height - 1;

  // 1. Identify all rooms that fall inside the new 2D zone
  for (int i = 0; i < rooms.size(); ++i) {
    if (rooms[i].x >= startX && rooms[i].x + rooms[i].width <= endX + 1 &&
        rooms[i].y >= startY && rooms[i].y + rooms[i].height <= endY + 1) {
      zoneRooms.push_back(i);
    }
  }

  // 2. Add the perimeter of EVERY new room into the shared frontier
  for (int roomIndex : zoneRooms) {
    const auto &room = rooms[roomIndex];
    int seedX = -1, seedY = -1;

    for (int ix = room.x; ix < room.x + room.width; ++ix) {
      if (maze.getCell(ix, room.y - 1) == Maze::CELL_WALL &&
          !maze.hasDiagonalLeak(ix, room.y - 1)) {
        seedX = ix;
        seedY = room.y - 1;
        goto ZONE_SEED_FOUND;
      }
      if (maze.getCell(ix, room.y + room.height) == Maze::CELL_WALL &&
          !maze.hasDiagonalLeak(ix, room.y + room.height)) {
        seedX = ix;
        seedY = room.y + room.height;
        goto ZONE_SEED_FOUND;
      }
    }
    for (int iy = room.y; iy < room.y + room.height; ++iy) {
      if (maze.getCell(room.x - 1, iy) == Maze::CELL_WALL &&
          !maze.hasDiagonalLeak(room.x - 1, iy)) {
        seedX = room.x - 1;
        seedY = iy;
        goto ZONE_SEED_FOUND;
      }
      if (maze.getCell(room.x + room.width, iy) == Maze::CELL_WALL &&
          !maze.hasDiagonalLeak(room.x + room.width, iy)) {
        seedX = room.x + room.width;
        seedY = iy;
        goto ZONE_SEED_FOUND;
      }
    }
  ZONE_SEED_FOUND:;

    if (seedX != -1 && seedY != -1) {
      maze.setCell(seedX, seedY, Maze::CELL_CORRIDOR);

      const int dx[] = {1, -1, 0, 0};
      const int dy[] = {0, 0, 1, -1};
      for (int d = 0; d < 4; ++d) {
        int nx = seedX + dx[d];
        int ny = seedY + dy[d];
        if (maze.getCell(nx, ny) == Maze::CELL_WALL && nx >= startX &&
            nx <= endX && ny >= startY && ny <= endY) {
          frontier.push_back(maze.getIndex(nx, ny));
        }
      }
    }
  }

  // 3. BORDER SEEDING: Add the Old Maze's exposed borders to the frontier!
  // This forces Prim's algorithm to natively weave into the old maze on all 4
  // sides!

  // Vertical Boundaries (Left and Right)
  for (int y = startY; y <= endY; ++y) {
    // Left Boundary
    if (maze.getCell(startX - 1, y) == Maze::CELL_CORRIDOR ||
        maze.getCell(startX - 1, y) == Maze::CELL_ROOM) {
      if (maze.getCell(startX, y) == Maze::CELL_WALL &&
          !maze.hasDiagonalLeak(startX, y)) {
        frontier.push_back(maze.getIndex(startX, y));
      }
    }
    // Right Boundary
    if (maze.getCell(endX + 1, y) == Maze::CELL_CORRIDOR ||
        maze.getCell(endX + 1, y) == Maze::CELL_ROOM) {
      if (maze.getCell(endX, y) == Maze::CELL_WALL &&
          !maze.hasDiagonalLeak(endX, y)) {
        frontier.push_back(maze.getIndex(endX, y));
      }
    }
  }

  // Horizontal Boundaries (Top and Bottom)
  for (int x = startX; x <= endX; ++x) {
    // Top Boundary
    if (maze.getCell(x, startY - 1) == Maze::CELL_CORRIDOR ||
        maze.getCell(x, startY - 1) == Maze::CELL_ROOM) {
      if (maze.getCell(x, startY) == Maze::CELL_WALL &&
          !maze.hasDiagonalLeak(x, startY)) {
        frontier.push_back(maze.getIndex(x, startY));
      }
    }
    // Bottom Boundary
    if (maze.getCell(x, endY + 1) == Maze::CELL_CORRIDOR ||
        maze.getCell(x, endY + 1) == Maze::CELL_ROOM) {
      if (maze.getCell(x, endY) == Maze::CELL_WALL &&
          !maze.hasDiagonalLeak(x, endY)) {
        frontier.push_back(maze.getIndex(x, endY));
      }
    }
  }

  // 4. Run the shared Prims loop to organically connect everything
  runPrims(maze, rng, frontier);

  // 5. Punch Doorways for just the zone rooms
  const int dx[] = {1, -1, 0, 0};
  const int dy[] = {0, 0, 1, -1};
  for (int roomIndex : zoneRooms) {
    const auto &room = rooms[roomIndex];
    std::vector<std::pair<int, int>> doorCandidates;

    for (int ix = room.x; ix < room.x + room.width; ++ix) {
      int wy = room.y - 1;
      if (maze.getCell(ix, wy) == Maze::CELL_WALL) {
        int rCount = 0, cCount = 0;
        for (int d = 0; d < 4; ++d) {
          int nx = ix + dx[d];
          int ny = wy + dy[d];
          if (maze.getCell(nx, ny) == Maze::CELL_ROOM)
            rCount++;
          if (maze.getCell(nx, ny) == Maze::CELL_CORRIDOR)
            cCount++;
        }
        if (rCount == 1 && cCount == 1) {
          if (!maze.hasDiagonalLeak(ix, wy) &&
              maze.isValidDoorPlacement(ix, wy))
            doorCandidates.push_back({ix, wy});
        }
      }
      wy = room.y + room.height;
      if (maze.getCell(ix, wy) == Maze::CELL_WALL) {
        int rCount = 0, cCount = 0;
        for (int d = 0; d < 4; ++d) {
          int nx = ix + dx[d];
          int ny = wy + dy[d];
          if (maze.getCell(nx, ny) == Maze::CELL_ROOM)
            rCount++;
          if (maze.getCell(nx, ny) == Maze::CELL_CORRIDOR)
            cCount++;
        }
        if (rCount == 1 && cCount == 1) {
          if (!maze.hasDiagonalLeak(ix, wy) &&
              maze.isValidDoorPlacement(ix, wy))
            doorCandidates.push_back({ix, wy});
        }
      }
    }
    for (int iy = room.y; iy < room.y + room.height; ++iy) {
      int wx = room.x - 1;
      if (maze.getCell(wx, iy) == Maze::CELL_WALL) {
        int rCount = 0, cCount = 0;
        for (int d = 0; d < 4; ++d) {
          int nx = wx + dx[d];
          int ny = iy + dy[d];
          if (maze.getCell(nx, ny) == Maze::CELL_ROOM)
            rCount++;
          if (maze.getCell(nx, ny) == Maze::CELL_CORRIDOR)
            cCount++;
        }
        if (rCount == 1 && cCount == 1) {
          if (!maze.hasDiagonalLeak(wx, iy) &&
              maze.isValidDoorPlacement(wx, iy))
            doorCandidates.push_back({wx, iy});
        }
      }
      wx = room.x + room.width;
      if (maze.getCell(wx, iy) == Maze::CELL_WALL) {
        int rCount = 0, cCount = 0;
        for (int d = 0; d < 4; ++d) {
          int nx = wx + dx[d];
          int ny = iy + dy[d];
          if (maze.getCell(nx, ny) == Maze::CELL_ROOM)
            rCount++;
          if (maze.getCell(nx, ny) == Maze::CELL_CORRIDOR)
            cCount++;
        }
        if (rCount == 1 && cCount == 1) {
          if (!maze.hasDiagonalLeak(wx, iy) &&
              maze.isValidDoorPlacement(wx, iy))
            doorCandidates.push_back({wx, iy});
        }
      }
    }

    if (!doorCandidates.empty()) {
      int pick = std::uniform_int_distribution<>(0, (int)doorCandidates.size() -
                                                        1)(rng);
      maze.setCell(doorCandidates[pick].first, doorCandidates[pick].second,
                   Maze::CELL_CORRIDOR);
    }
  }
}

// ============================================================================
// ALCOVE PRUNING — Connected Component Analysis
// ============================================================================
// After Prim's generates corridors, some tiny clusters of CELL_CORRIDOR tiles
// exist that only connect to a single room. These are useless alcoves that
// clutter the maze. We identify them via BFS and erase them.
//
// Algorithm:
//   1. Scan every cell in the grid for unvisited CELL_CORRIDOR tiles.
//   2. For each one found, BFS flood-fill through CELL_CORRIDOR tiles only.
//      - Track every tile index in the component.
//      - For each tile, check its 4 cardinal neighbors for CELL_ROOM.
//        If found, determine which Room it belongs to and add that room
//        index to a set of "adjacent rooms."
//   3. After BFS: if component size < minSize AND adjacentRooms <= 1,
//      erase the entire component (set all tiles back to CELL_WALL).
//
// Time Complexity:  O(V) — every cell visited at most once across all BFS runs.
// Space Complexity: O(V) — for the visited array.
// ============================================================================
void PrimsGenerator::pruneSmallAlcoves(Maze &maze, int minSize) {
  int width = maze.getWidth();
  int height = maze.getHeight();
  int totalCells = width * height;

  // Visited array — marks cells we've already assigned to a component.
  // This prevents us from re-scanning the same cell in multiple BFS runs.
  std::vector<bool> visited(totalCells, false);

  // Cardinal directions: Right, Left, Down, Up
  const int dx[] = {1, -1, 0, 0};
  const int dy[] = {0, 0, 1, -1};

  // Reference to the rooms list for adjacency lookups
  const auto &rooms = maze.getRooms();

  // Lambda helper: Given a grid coordinate (cx, cy), check if it falls
  // inside any Room and return that room's index. Returns -1 if not in a room.
  // This is O(R) per call where R is the number of rooms.
  auto findRoomIndex = [&](int cx, int cy) -> int {
    for (int i = 0; i < (int)rooms.size(); ++i) {
      if (cx >= rooms[i].x && cx < rooms[i].x + rooms[i].width &&
          cy >= rooms[i].y && cy < rooms[i].y + rooms[i].height) {
        return i;
      }
    }
    return -1;
  };

  // Main scan: iterate over every cell in the grid
  for (int y = 0; y < height; ++y) {
    for (int x = 0; x < width; ++x) {
      int idx = maze.getIndex(x, y);

      // Skip if already visited or not a corridor floor tile
      if (visited[idx] || maze.getCell(x, y) != Maze::CELL_CORRIDOR)
        continue;

      // --- BFS Flood Fill for this connected component ---
      // componentTiles: stores the 1D index of every CELL_CORRIDOR in this
      // cluster
      std::vector<int> componentTiles;
      // adjacentRooms: tracks which unique rooms this cluster touches
      std::set<int> adjacentRooms;

      std::queue<std::pair<int, int>> q;
      q.push({x, y});
      visited[idx] = true;

      while (!q.empty()) {
        auto [cx, cy] = q.front();
        q.pop();

        // Record this tile as part of the component
        componentTiles.push_back(maze.getIndex(cx, cy));

        // Check all 4 cardinal neighbors
        for (int d = 0; d < 4; ++d) {
          int nx = cx + dx[d];
          int ny = cy + dy[d];

          // Bounds check (don't wrap for pruning — only prune within bounds)
          if (nx < 0 || nx >= width || ny < 0 || ny >= height)
            continue;

          int nIdx = maze.getIndex(nx, ny);
          int neighborType = maze.getCell(nx, ny);

          if (neighborType == Maze::CELL_CORRIDOR && !visited[nIdx]) {
            // Neighbor is a corridor tile we haven't seen — add to BFS queue
            visited[nIdx] = true;
            q.push({nx, ny});
          } else if (neighborType == Maze::CELL_ROOM) {
            // Neighbor is a room tile — figure out which room it belongs to
            int roomIdx = findRoomIndex(nx, ny);
            if (roomIdx != -1) {
              adjacentRooms.insert(roomIdx);
            }
          }
        }
      }
      // --- BFS complete for this component ---

      // Decision: erase if the component is too small AND doesn't bridge rooms
      if ((int)componentTiles.size() < minSize && adjacentRooms.size() <= 1) {
        for (int tileIdx : componentTiles) {
          int tx = tileIdx % width;
          int ty = tileIdx / width;
          maze.setCell(tx, ty, Maze::CELL_WALL);
        }
      }
    }
  }
}
