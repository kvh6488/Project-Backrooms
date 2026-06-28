#include "tunnel_borer.hpp"
#include <queue>
#include <vector>

void fixDiagonalLeaksRecursive(Maze& maze, int cx, int cy) {
  const int ddx[] = {-1, 1, -1, 1};
  const int ddy[] = {-1, -1, 1, 1};
  for (int i = 0; i < 4; ++i) {
    int nx = cx + ddx[i];
    int ny = cy + ddy[i];
    int tDiag = maze.getCell(nx, ny);
    if (tDiag == Maze::CELL_CORRIDOR || tDiag == Maze::CELL_ROOM) {
      int t1 = maze.getCell(nx, cy);
      int t2 = maze.getCell(cx, ny);
      if (t1 == Maze::CELL_WALL && t2 == Maze::CELL_WALL) {
        maze.setCell(nx, cy, Maze::CELL_CORRIDOR);
        fixDiagonalLeaksRecursive(maze, nx, cy);
      }
    }
  }
}

void TunnelBorer::ensureConnectivity(Maze& maze) {
  int m_width = maze.getWidth();
  int m_height = maze.getHeight();

  // We need to keep looping because connecting one isolated room might still
  // leave others isolated. We stop when no isolated rooms remain.
  while (true) {
    // 1. Map the Main Network (Flood-Fill BFS)
    // We will find the very first valid room cell and flood outward, marking
    // every reachable floor/room cell as true.
    std::vector<bool> mainNetwork(m_width * m_height, false);
    std::queue<std::pair<int, int>> q;

    // Find a starting point (any non-wall cell)
    for (int y = 0; y < m_height; ++y) {
      for (int x = 0; x < m_width; ++x) {
        if (maze.getCell(x, y) != Maze::CELL_WALL) {
          q.push({x, y});
          mainNetwork[maze.getIndex(x, y)] = true;
          goto START_FLOOD; // Break out of nested loops
        }
      }
    }
  START_FLOOD:

    // Standard BFS: pop a cell, check its 4 neighbors
    const int dx[] = {0, 1, 0, -1};
    const int dy[] = {-1, 0, 1, 0};

    while (!q.empty()) {
      auto [cx, cy] = q.front();
      q.pop();

      for (int i = 0; i < 4; ++i) {
        int nx = cx + dx[i];
        int ny = cy + dy[i];

        int nIndex = maze.getIndex(nx, ny);
        // If neighbor is open space and hasn't been visited yet
        if (!mainNetwork[nIndex] && maze.getCell(nx, ny) != Maze::CELL_WALL) {
          mainNetwork[nIndex] = true;
          q.push({nx, ny});
        }
      }
    }

    int isolatedX = -1, isolatedY = -1;
    for (int y = 0; y < m_height; ++y) {
      for (int x = 0; x < m_width; ++x) {
        if (maze.getCell(x, y) != Maze::CELL_WALL && !mainNetwork[maze.getIndex(x, y)]) {
          isolatedX = x;
          isolatedY = y;
          goto FOUND_ISOLATED;
        }
      }
    }
  FOUND_ISOLATED:

    // If we didn't find any isolated rooms, we are 100% connected!
    if (isolatedX == -1) break; 

    // 3. Bore the Tunnel (Pathfinding BFS)
    // We start a new BFS from the isolated room.
    // This BFS CAN walk through solid rock (CELL_WALL).
    // It keeps track of "parents" so we can retrace the path.
    std::vector<int> parent(m_width * m_height, -1);
    std::vector<bool> visited(m_width * m_height, false);
    std::queue<std::pair<int, int>> borerQ;

    borerQ.push({isolatedX, isolatedY});
    visited[maze.getIndex(isolatedX, isolatedY)] = true;

    int intersectX = -1, intersectY = -1;

    while (!borerQ.empty()) {
      auto [cx, cy] = borerQ.front();
      borerQ.pop();

      // Did our borer just break into the main network?
      if (mainNetwork[maze.getIndex(cx, cy)]) {
        intersectX = cx;
        intersectY = cy;
        break; // Stop! We found the shortest path.
      }

      for (int i = 0; i < 4; ++i) {
        int nx = cx + dx[i];
        int ny = cy + dy[i];

        // We rely on getIndex to wrap coordinates naturally across the Torus!
        int nIndex = maze.getIndex(nx, ny);
        if (!visited[nIndex]) {
          visited[nIndex] = true;
          parent[nIndex] = maze.getIndex(cx, cy); // Remember where we came from
          borerQ.push({nx, ny});
        }
      }
    }

    // 4. Trace back and carve!
    // Start from the intersection point and walk backwards using the 'parent' array
    int currIdx = maze.getIndex(intersectX, intersectY);
    int startIdx = maze.getIndex(isolatedX, isolatedY);

    while (currIdx != startIdx && currIdx != -1) {
      int cx = currIdx % m_width;
      int cy = currIdx / m_width;

      // Smash the wall!
      if (maze.getCell(cx, cy) == Maze::CELL_WALL) {
        maze.setCell(cx, cy, Maze::CELL_CORRIDOR);
        fixDiagonalLeaksRecursive(maze, cx, cy);
      }

      currIdx = parent[currIdx]; // Move backwards one step
    }

    // Loop repeats to check if there are ANY OTHER isolated rooms left!
  }
}
