#include "prims_generator.hpp"

// ============================================================================
// PRIM'S ALGORITHM IMPLEMENTATION (Corridor Generation)
// ============================================================================
void PrimsGenerator::generate(Maze& maze, std::mt19937& rng) {
  std::vector<int> frontier;

  int seedX = -1, seedY = -1;
  const auto& rooms = maze.getRooms();

  for (const auto &room : rooms) {
    seedX = room.x - 1;
    seedY = room.y;
    if (maze.isInBounds(seedX, seedY) && maze.getCell(seedX, seedY) == Maze::CELL_WALL) break;

    seedX = room.x + room.width;
    if (maze.isInBounds(seedX, seedY) && maze.getCell(seedX, seedY) == Maze::CELL_WALL) break;

    seedX = room.x;
    seedY = room.y + room.height;
    if (maze.isInBounds(seedX, seedY) && maze.getCell(seedX, seedY) == Maze::CELL_WALL) break;

    seedX = -1;
    seedY = -1;
  }

  if (seedX == -1 || seedY == -1) return;

  maze.setCell(seedX, seedY, Maze::CELL_FLOOR);

  const int dx[] = {1, -1, 0, 0};
  const int dy[] = {0, 0, 1, -1};

  for (int d = 0; d < 4; ++d) {
    int nx = seedX + dx[d];
    int ny = seedY + dy[d];
    if (maze.isInBounds(nx, ny) && maze.getCell(nx, ny) == Maze::CELL_WALL) {
      frontier.push_back(ny * maze.getWidth() + nx);
    }
  }

  while (!frontier.empty()) {
    int randIdx = std::uniform_int_distribution<>(0, (int)frontier.size() - 1)(rng);
    int wallIndex = frontier[randIdx];

    frontier[randIdx] = frontier.back();
    frontier.pop_back();

    int wx = wallIndex % maze.getWidth();
    int wy = wallIndex / maze.getWidth();

    if (maze.getCell(wx, wy) != Maze::CELL_WALL) continue;

    int carvedNeighborCount = 0;
    for (int d = 0; d < 4; ++d) {
      int nx = wx + dx[d];
      int ny = wy + dy[d];
      if (maze.isInBounds(nx, ny)) {
        int neighborType = maze.getCell(nx, ny);
        if (neighborType == Maze::CELL_FLOOR || neighborType == Maze::CELL_ROOM) {
          ++carvedNeighborCount;
        }
      }
    }

    if (carvedNeighborCount != 1) continue;

    bool hasDiagonalLeak = false;
    const int diagX[] = {1, 1, -1, -1};
    const int diagY[] = {1, -1, 1, -1};
    for (int d = 0; d < 4; ++d) {
      int dnx = wx + diagX[d];
      int dny = wy + diagY[d];
      if (maze.isInBounds(dnx, dny)) {
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
    }
    if (hasDiagonalLeak) continue;

    maze.setCell(wx, wy, Maze::CELL_FLOOR);

    for (int d = 0; d < 4; ++d) {
      int nx = wx + dx[d];
      int ny = wy + dy[d];
      if (maze.isInBounds(nx, ny) && maze.getCell(nx, ny) == Maze::CELL_WALL) {
        frontier.push_back(ny * maze.getWidth() + nx);
      }
    }
  }

  // --- STEP 6: Punch Doorways ---
  for (const auto &room : rooms) {
    std::vector<std::pair<int, int>> doorCandidates;

    for (int ix = room.x; ix < room.x + room.width; ++ix) {
      int wy = room.y - 1;
      if (maze.isInBounds(ix, wy) && maze.getCell(ix, wy) == Maze::CELL_WALL) {
        if (maze.isInBounds(ix, wy - 1) && maze.getCell(ix, wy - 1) == Maze::CELL_FLOOR) {
          doorCandidates.push_back({ix, wy});
        }
      }
      wy = room.y + room.height;
      if (maze.isInBounds(ix, wy) && maze.getCell(ix, wy) == Maze::CELL_WALL) {
        if (maze.isInBounds(ix, wy + 1) && maze.getCell(ix, wy + 1) == Maze::CELL_FLOOR) {
          doorCandidates.push_back({ix, wy});
        }
      }
    }
    for (int iy = room.y; iy < room.y + room.height; ++iy) {
      int wx = room.x - 1;
      if (maze.isInBounds(wx, iy) && maze.getCell(wx, iy) == Maze::CELL_WALL) {
        if (maze.isInBounds(wx - 1, iy) && maze.getCell(wx - 1, iy) == Maze::CELL_FLOOR) {
          doorCandidates.push_back({wx, iy});
        }
      }
      wx = room.x + room.width;
      if (maze.isInBounds(wx, iy) && maze.getCell(wx, iy) == Maze::CELL_WALL) {
        if (maze.isInBounds(wx + 1, iy) && maze.getCell(wx + 1, iy) == Maze::CELL_FLOOR) {
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
