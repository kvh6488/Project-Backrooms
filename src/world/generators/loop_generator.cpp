#include "world/generators/loop_generator.hpp"
#include "world/generators/generator_utils.hpp"

// ============================================================================
// LOOP GENERATION — Breaking the Perfect Maze
// ============================================================================
void LoopGenerator::generate(Maze& maze, std::mt19937& rng) {
  generateZone(maze, rng, 0, 0, maze.getWidth(), maze.getHeight());
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

      int carvedCount = GeneratorUtils::countCarvedNeighbors(maze, x, y);

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
