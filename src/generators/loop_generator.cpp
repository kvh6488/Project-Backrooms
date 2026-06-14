#include "loop_generator.hpp"

// ============================================================================
// LOOP GENERATION — Breaking the Perfect Maze
// ============================================================================
void LoopGenerator::generate(Maze& maze, std::mt19937& rng) {
  // The probability that any given wall gets smashed (out of 100).
  // 5% creates a nice balance: enough loops to confuse, but not so many
  // that the maze becomes a wide-open field.
  const int LOOP_CHANCE = 5;

  int m_width = maze.getWidth();
  int m_height = maze.getHeight();

  // Skip the outer border (row 0, last row, column 0, last column)
  // so the maze always stays fully enclosed.
  for (int y = 1; y < m_height - 1; ++y) {
    for (int x = 1; x < m_width - 1; ++x) {

      // Only consider wall cells
      if (maze.getCell(x, y) != Maze::CELL_WALL)
        continue;

      // Roll the dice — 5% chance to break this wall
      if (std::uniform_int_distribution<>(0, 99)(rng) >= LOOP_CHANCE)
        continue;

      // A wall only makes sense to break if it actually separates two
      // carved areas. Check that at least 2 cardinal neighbors are carved
      // (floor or room). If only 0 or 1 neighbors are carved, smashing
      // this wall would just extend a dead end, not create a loop.
      const int dx[] = {1, -1, 0, 0};
      const int dy[] = {0, 0, 1, -1};
      int carvedCount = 0;
      for (int d = 0; d < 4; ++d) {
        int nx = x + dx[d];
        int ny = y + dy[d];
        if (maze.isInBounds(nx, ny)) {
          int t = maze.getCell(nx, ny);
          if (t == Maze::CELL_FLOOR || t == Maze::CELL_ROOM)
            ++carvedCount;
        }
      }

      // We need at least 2 carved neighbors to form an actual loop
      if (carvedCount < 2)
        continue;

      // --- Diagonal Leak Check (same logic as Prim's Step 4b) ---
      // Even though we WANT loops here, we still don't want diagonal-only
      // connections because they break wall rendering.
      bool hasDiagonalLeak = false;
      const int diagX[] = {1, 1, -1, -1};
      const int diagY[] = {1, -1, 1, -1};
      for (int d = 0; d < 4; ++d) {
        int dnx = x + diagX[d];
        int dny = y + diagY[d];
        if (maze.isInBounds(dnx, dny)) {
          int diagType = maze.getCell(dnx, dny);
          if (diagType == Maze::CELL_FLOOR || diagType == Maze::CELL_ROOM) {
            int sharedA = maze.getCell(x + diagX[d], y);
            int sharedB = maze.getCell(x, y + diagY[d]);
            if (sharedA == Maze::CELL_WALL && sharedB == Maze::CELL_WALL) {
              hasDiagonalLeak = true;
              break;
            }
          }
        }
      }
      if (hasDiagonalLeak)
        continue;

      // All checks passed — smash the wall to create a loop!
      maze.setCell(x, y, Maze::CELL_FLOOR);
    }
  }
}
