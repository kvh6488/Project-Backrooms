#include "prims_generator.hpp"

// ============================================================================
// PRIM'S ALGORITHM IMPLEMENTATION (Corridor Generation)
// ============================================================================
void PrimsGenerator::generate(Maze& maze, std::mt19937& rng, int startRoomIndex) {
  int totalCells = maze.getWidth() * maze.getHeight();
  int triggerThreshold = static_cast<int>(totalCells * 0.05f); // 5% trigger
  int targetCoverage = static_cast<int>(totalCells * 0.25f); // 25% coverage target

  // Initial run from the center
  int carvedCount = carveFromRoom(maze, rng, startRoomIndex);

  // Fallback Check: If we got blocked early and covered less than 5% of the map,
  // we pick random rooms to seed from and run the algorithm continuously until
  // we reach at least 25% coverage.
  if (carvedCount < triggerThreshold && !maze.getRooms().empty()) {
    int maxAttempts = 50;
    int attempts = 0;
    while (carvedCount < targetCoverage && attempts < maxAttempts) {
      int randomRoomIndex = std::uniform_int_distribution<>(0, (int)maze.getRooms().size() - 1)(rng);
      carvedCount += carveFromRoom(maze, rng, randomRoomIndex);
      attempts++;
    }
  }

  const auto& rooms = maze.getRooms();

  // --- STEP 3: Punch Doorways ---
  // Prim's algorithm specifically avoids carving into existing carved areas to prevent loops.
  // Because of this, it naturally wraps *around* the BSP rooms but never actually connects to them!
  // We must do a final pass around the perimeter of every room and forcefully punch a door
  // to connect the room to the corridor network.
  for (const auto &room : rooms) {
    // Collect all viable spots for a door (must have rock on the perimeter, and floor right outside it)
    std::vector<std::pair<int, int>> doorCandidates;

    for (int ix = room.x; ix < room.x + room.width; ++ix) {
      int wy = room.y - 1;
      if (maze.getCell(ix, wy) == Maze::CELL_WALL) {
        if (maze.getCell(ix, wy - 1) == Maze::CELL_FLOOR) {
          doorCandidates.push_back({ix, wy});
        }
      }
      wy = room.y + room.height;
      if (maze.getCell(ix, wy) == Maze::CELL_WALL) {
        if (maze.getCell(ix, wy + 1) == Maze::CELL_FLOOR) {
          doorCandidates.push_back({ix, wy});
        }
      }
    }
    for (int iy = room.y; iy < room.y + room.height; ++iy) {
      int wx = room.x - 1;
      if (maze.getCell(wx, iy) == Maze::CELL_WALL) {
        if (maze.getCell(wx - 1, iy) == Maze::CELL_FLOOR) {
          doorCandidates.push_back({wx, iy});
        }
      }
      wx = room.x + room.width;
      if (maze.getCell(wx, iy) == Maze::CELL_WALL) {
        if (maze.getCell(wx + 1, iy) == Maze::CELL_FLOOR) {
          doorCandidates.push_back({wx, iy});
        }
      }
    }

    if (!doorCandidates.empty()) {
      int pick = std::uniform_int_distribution<>(0, (int)doorCandidates.size() - 1)(rng);
      maze.setCell(doorCandidates[pick].first, doorCandidates[pick].second, Maze::CELL_FLOOR);
    }
  }
}

int PrimsGenerator::carveFromRoom(Maze& maze, std::mt19937& rng, int startRoomIndex) {
  // 1. The Frontier List: Stores the indices of all rock/wall cells that are 
  // adjacent to a carved floor. We will randomly pick from this list to grow the maze.
  std::vector<int> frontier;
  int carvedCount = 0;
  int seedX = -1, seedY = -1;
  const auto& rooms = maze.getRooms();

  // --- STEP 1: Find a Seed Point ---
  // Prim's algorithm needs a starting point (a "seed"). Instead of picking a random
  // rock cell, we want the maze to grow *out* of the BSP rooms. 
  // We scan the perimeter of the STARTING room to find a solid rock wall to start carving.

  if (startRoomIndex >= 0 && startRoomIndex < rooms.size()) {
    const auto &room = rooms[startRoomIndex];
    
    seedX = room.x - 1;
    seedY = room.y;
    if (!(maze.getCell(seedX, seedY) == Maze::CELL_WALL)) {
      seedX = room.x + room.width;
      if (!(maze.getCell(seedX, seedY) == Maze::CELL_WALL)) {
        seedX = room.x;
        seedY = room.y + room.height;
        if (!(maze.getCell(seedX, seedY) == Maze::CELL_WALL)) {
          seedX = -1;
          seedY = -1;
        }
      }
    }
  }

  if (seedX == -1 || seedY == -1) return 0;

  // Carve the seed cell to start the maze
  maze.setCell(seedX, seedY, Maze::CELL_FLOOR);
  carvedCount++;

  // Cardinal directions: Right, Left, Down, Up
  const int dx[] = {1, -1, 0, 0};
  const int dy[] = {0, 0, 1, -1};

  // Add all solid walls immediately adjacent to the seed into the frontier list.
  // The frontier represents the "edge" of our growing mold.

  for (int d = 0; d < 4; ++d) {
    int nx = seedX + dx[d];
    int ny = seedY + dy[d];
    if (maze.getCell(nx, ny) == Maze::CELL_WALL) {
      frontier.push_back(maze.getIndex(nx, ny));
    }
  }

  // 2. Main Prim's Loop: Keep going until the frontier is empty.
  while (!frontier.empty()) {
    // Pick a random wall from the frontier. This randomness creates the chaotic,
    // organic mold-like growth characteristic of the Backrooms.
    int randIdx = std::uniform_int_distribution<>(0, (int)frontier.size() - 1)(rng);
    int wallIndex = frontier[randIdx];

    // Fast O(1) Removal: Instead of shifting the entire vector down (which is O(N)),
    // we swap the chosen element with the very last element, and then pop the back.
    frontier[randIdx] = frontier.back();
    frontier.pop_back();

    // Convert the 1D index back into 2D grid coordinates
    int wx = wallIndex % maze.getWidth();
    int wy = wallIndex / maze.getWidth();

    // If this cell was somehow already carved (perhaps by another path), skip it
    if (maze.getCell(wx, wy) != Maze::CELL_WALL) continue;

    int carvedNeighborCount = 0;
    for (int d = 0; d < 4; ++d) {
      int nx = wx + dx[d];
      int ny = wy + dy[d];
      int neighborType = maze.getCell(nx, ny);
      if (neighborType == Maze::CELL_FLOOR || neighborType == Maze::CELL_ROOM) {
        ++carvedNeighborCount;
      }
    }

    // A valid Prim's wall MUST have exactly 1 carved neighbor. 
    // If it has 2 or more, breaking it would create a loop, violating the perfect maze tree structure.
    if (carvedNeighborCount != 1) continue;

    // --- Diagonal Leak Check ---
    // In grid mazes, carving cells diagonally adjacent can create visual "leaks" where two rooms 
    // touch only at the corner, making the wall geometry look broken in 3D.
    bool hasDiagonalLeak = false;
    const int diagX[] = {1, 1, -1, -1};
    const int diagY[] = {1, -1, 1, -1};
    for (int d = 0; d < 4; ++d) {
      int dnx = wx + diagX[d];
      int dny = wy + diagY[d];
      int diagType = maze.getCell(dnx, dny);
      if (diagType == Maze::CELL_FLOOR || diagType == Maze::CELL_ROOM) {
        int sharedA = maze.getCell(wx + diagX[d], wy);
        int sharedB = maze.getCell(wx, wy + diagY[d]);
        if (sharedA == Maze::CELL_WALL && sharedB == Maze::CELL_WALL) {
          hasDiagonalLeak = true;
          break;
        }
      }
    }
    if (hasDiagonalLeak) continue;

    // All checks passed! Smash the wall and turn it into a floor.
    maze.setCell(wx, wy, Maze::CELL_FLOOR);
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
