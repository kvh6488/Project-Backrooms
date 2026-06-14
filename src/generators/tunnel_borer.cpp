#include "tunnel_borer.hpp"
#include <queue>
#include <vector>

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
          mainNetwork[y * m_width + x] = true;
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

        if (maze.isInBounds(nx, ny)) {
          int nIndex = ny * m_width + nx;
          // If neighbor is open space and hasn't been visited yet
          if (!mainNetwork[nIndex] && maze.getCell(nx, ny) != Maze::CELL_WALL) {
            mainNetwork[nIndex] = true;
            q.push({nx, ny});
          }
        }
      }
    }

    // 2. Detect Isolation
    // Scan the maze. Is there a room cell that mainNetwork didn't reach?
    int isolatedX = -1, isolatedY = -1;
    for (int y = 0; y < m_height; ++y) {
      for (int x = 0; x < m_width; ++x) {
        if (maze.getCell(x, y) == Maze::CELL_ROOM && !mainNetwork[y * m_width + x]) {
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
    visited[isolatedY * m_width + isolatedX] = true;

    int intersectX = -1, intersectY = -1;

    while (!borerQ.empty()) {
      auto [cx, cy] = borerQ.front();
      borerQ.pop();

      // Did our borer just break into the main network?
      if (mainNetwork[cy * m_width + cx]) {
        intersectX = cx;
        intersectY = cy;
        break; // Stop! We found the shortest path.
      }

      for (int i = 0; i < 4; ++i) {
        int nx = cx + dx[i];
        int ny = cy + dy[i];

        // Skip the outer border so we don't bore tunnels outside the map
        if (nx > 0 && nx < m_width - 1 && ny > 0 && ny < m_height - 1) {
          int nIndex = ny * m_width + nx;
          if (!visited[nIndex]) {
            visited[nIndex] = true;
            parent[nIndex] = cy * m_width + cx; // Remember where we came from
            borerQ.push({nx, ny});
          }
        }
      }
    }

    // 4. Trace back and carve!
    // Start from the intersection point and walk backwards using the 'parent' array
    int currIdx = intersectY * m_width + intersectX;
    int startIdx = isolatedY * m_width + isolatedX;

    while (currIdx != startIdx && currIdx != -1) {
      int cx = currIdx % m_width;
      int cy = currIdx / m_width;

      // Smash the wall!
      if (maze.getCell(cx, cy) == Maze::CELL_WALL) {
        maze.setCell(cx, cy, Maze::CELL_FLOOR);
      }

      currIdx = parent[currIdx]; // Move backwards one step
    }

    // Loop repeats to check if there are ANY OTHER isolated rooms left!
  }
}
