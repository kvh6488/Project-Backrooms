#include "world/generators/generator_utils.hpp"

namespace GeneratorUtils {

int countCarvedNeighbors(const Maze& maze, int x, int y) {
  int count = 0;
  for (int d = 0; d < 4; ++d) {
    int nx = x + DX[d];
    int ny = y + DY[d];
    int t = maze.getCell(nx, ny);
    if (t == Maze::CELL_CORRIDOR || t == Maze::CELL_ROOM) {
      ++count;
    }
  }
  return count;
}

void punchDoorways(Maze& maze, std::mt19937& rng, const std::vector<Maze::Room>& roomsToProcess) {
  for (const auto& room : roomsToProcess) {
    std::vector<std::pair<int, int>> doorCandidates;

    // Top and Bottom walls
    for (int ix = room.x; ix < room.x + room.width; ++ix) {
      int yTop = room.y - 1;
      if (maze.getCell(ix, yTop) == Maze::CELL_WALL) {
        int rCount = 0, cCount = 0;
        for (int d = 0; d < 4; ++d) {
          int nx = ix + DX[d];
          int ny = yTop + DY[d];
          if (maze.getCell(nx, ny) == Maze::CELL_ROOM) rCount++;
          if (maze.getCell(nx, ny) == Maze::CELL_CORRIDOR) cCount++;
        }
        if (rCount == 1 && cCount == 1) {
          if (!maze.hasDiagonalLeak(ix, yTop) && maze.isValidDoorPlacement(ix, yTop))
            doorCandidates.push_back({ix, yTop});
        }
      }

      int yBottom = room.y + room.height;
      if (maze.getCell(ix, yBottom) == Maze::CELL_WALL) {
        int rCount = 0, cCount = 0;
        for (int d = 0; d < 4; ++d) {
          int nx = ix + DX[d];
          int ny = yBottom + DY[d];
          if (maze.getCell(nx, ny) == Maze::CELL_ROOM) rCount++;
          if (maze.getCell(nx, ny) == Maze::CELL_CORRIDOR) cCount++;
        }
        if (rCount == 1 && cCount == 1) {
          if (!maze.hasDiagonalLeak(ix, yBottom) && maze.isValidDoorPlacement(ix, yBottom))
            doorCandidates.push_back({ix, yBottom});
        }
      }
    }

    // Left and Right walls
    for (int iy = room.y; iy < room.y + room.height; ++iy) {
      int xLeft = room.x - 1;
      if (maze.getCell(xLeft, iy) == Maze::CELL_WALL) {
        int rCount = 0, cCount = 0;
        for (int d = 0; d < 4; ++d) {
          int nx = xLeft + DX[d];
          int ny = iy + DY[d];
          if (maze.getCell(nx, ny) == Maze::CELL_ROOM) rCount++;
          if (maze.getCell(nx, ny) == Maze::CELL_CORRIDOR) cCount++;
        }
        if (rCount == 1 && cCount == 1) {
          if (!maze.hasDiagonalLeak(xLeft, iy) && maze.isValidDoorPlacement(xLeft, iy))
            doorCandidates.push_back({xLeft, iy});
        }
      }

      int xRight = room.x + room.width;
      if (maze.getCell(xRight, iy) == Maze::CELL_WALL) {
        int rCount = 0, cCount = 0;
        for (int d = 0; d < 4; ++d) {
          int nx = xRight + DX[d];
          int ny = iy + DY[d];
          if (maze.getCell(nx, ny) == Maze::CELL_ROOM) rCount++;
          if (maze.getCell(nx, ny) == Maze::CELL_CORRIDOR) cCount++;
        }
        if (rCount == 1 && cCount == 1) {
          if (!maze.hasDiagonalLeak(xRight, iy) && maze.isValidDoorPlacement(xRight, iy))
            doorCandidates.push_back({xRight, iy});
        }
      }
    }

    if (!doorCandidates.empty()) {
      int pick = std::uniform_int_distribution<>(0, (int)doorCandidates.size() - 1)(rng);
      maze.setCell(doorCandidates[pick].first, doorCandidates[pick].second, Maze::CELL_CORRIDOR);
    }
  }
}

} // namespace GeneratorUtils
