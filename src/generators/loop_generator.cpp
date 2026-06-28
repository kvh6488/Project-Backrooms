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
        int t = maze.getCell(nx, ny);
        if (t == Maze::CELL_CORRIDOR || t == Maze::CELL_ROOM)
          ++carvedCount;
      }

      // We need at least 2 carved neighbors to form an actual loop
      if (carvedCount < 2)
        continue;

      if (maze.hasDiagonalLeak(x, y))
        continue;

      // Ensure this loop doesn't create an invalid door (e.g. into an inner corner of a room)
      if (!maze.isValidDoorPlacement(x, y))
        continue;

      // All checks passed — smash the wall to create a loop!
      maze.setCell(x, y, Maze::CELL_CORRIDOR);
    }
  }
}

// ============================================================================
// LOOP GENERATION — Zone Restricted
// ============================================================================
void LoopGenerator::generateZone(Maze& maze, std::mt19937& rng, int startX, int startY, int width, int height) {
  const int LOOP_CHANCE = 5;

  int endX = startX + width - 1;
  int endY = startY + height - 1;

  for (int y = startY; y <= endY; ++y) {
    if (y <= 0 || y >= maze.getHeight() - 1) continue;

    for (int x = startX; x <= endX; ++x) {
      if (x <= 0 || x >= maze.getWidth() - 1) continue;

      if (maze.getCell(x, y) != Maze::CELL_WALL)
        continue;

      if (std::uniform_int_distribution<>(0, 99)(rng) >= LOOP_CHANCE)
        continue;

      const int dx[] = {1, -1, 0, 0};
      const int dy[] = {0, 0, 1, -1};
      int carvedCount = 0;
      for (int d = 0; d < 4; ++d) {
        int nx = x + dx[d];
        int ny = y + dy[d];
        int t = maze.getCell(nx, ny);
        if (t == Maze::CELL_CORRIDOR || t == Maze::CELL_ROOM)
          ++carvedCount;
      }

      if (carvedCount < 2)
        continue;

      if (maze.hasDiagonalLeak(x, y))
        continue;

      if (!maze.isValidDoorPlacement(x, y))
        continue;

      maze.setCell(x, y, Maze::CELL_CORRIDOR);
    }
  }
}
