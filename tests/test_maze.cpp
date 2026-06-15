#include "generators/bsp_generator.hpp"
#include "generators/loop_generator.hpp"
#include "generators/prims_generator.hpp"
#include "generators/tunnel_borer.hpp"
#include "maze.hpp"
#include <ctime>
#include <gtest/gtest.h>
#include <queue>

// 1. Initialize a small 10x10 maze
TEST(MazeTest, Constructor) {
  Maze maze(10, 10, 32, 12345);
  EXPECT_EQ(maze.getWidth(), 10);
  EXPECT_EQ(maze.getHeight(), 10);
  EXPECT_EQ(maze.getCellSize(), 32);
}

// 2. Test getIndex
TEST(MazeTest, GetIndexMath) {
  Maze maze(10, 10, 32, 12345);
  EXPECT_EQ(maze.getIndex(5, 2), 25);
  EXPECT_EQ(maze.getIndex(0, 0), 0);
  EXPECT_EQ(maze.getIndex(9, 9), 99);
}

// 3. Test isInBounds
TEST(MazeTest, IsInBoundsChecks) {
  Maze maze(10, 10, 32, 12345);
  EXPECT_TRUE(maze.isInBounds(0, 0));
  EXPECT_TRUE(maze.isInBounds(9, 9));
  EXPECT_FALSE(maze.isInBounds(10, 9));
  EXPECT_FALSE(maze.isInBounds(9, 10));
  EXPECT_FALSE(maze.isInBounds(-1, 5));
  EXPECT_FALSE(maze.isInBounds(5, -1));
}

// 4. Test setCell and getCell
TEST(MazeTest, CellReadWrite) {
  Maze maze(10, 10, 32, 12345);
  EXPECT_EQ(maze.getCell(3, 3), Maze::CELL_WALL);

  maze.setCell(3, 3, Maze::CELL_FLOOR);
  EXPECT_EQ(maze.getCell(3, 3), Maze::CELL_FLOOR);

  maze.setCell(8, 8, Maze::CELL_ROOM);
  EXPECT_EQ(maze.getCell(8, 8), Maze::CELL_ROOM);
}

// 5. Test Out-of-Bounds Safety
TEST(MazeTest, OutOfBoundsSafety) {
  Maze maze(10, 10, 32, 12345);

  // Writing out of bounds should be silently ignored (not crash)
  maze.setCell(-5, 20, Maze::CELL_FLOOR);
  maze.setCell(100, 100, Maze::CELL_ROOM);

  // Reading out of bounds should return CELL_WALL (0) safely
  EXPECT_EQ(maze.getCell(-1, 0), Maze::CELL_WALL);
  EXPECT_EQ(maze.getCell(0, 10), Maze::CELL_WALL);
}

// 6. Test BSP Generation
TEST(MazeTest, BSPGenerationCarvesRooms) {
  // We use a fixed seed (12345) so the test is fully deterministic
  Maze maze(40, 22, 32, 12345);

  // 1. Before generation, the entire maze should be solid rock (0 rooms)
  int roomCountBefore = 0;
  for (int y = 0; y < 22; ++y) {
    for (int x = 0; x < 40; ++x) {
      if (maze.getCell(x, y) == Maze::CELL_ROOM) {
        roomCountBefore++;
      }
    }
  }
  EXPECT_EQ(roomCountBefore, 0);

  // 2. Execute the BSP algorithm
  std::mt19937 rng(12345);
  BSPGenerator bsp;
  bsp.generate(maze, rng);

  // 3. After generation, we should have successfully carved hundreds of room
  // tiles
  int roomCountAfter = 0;
  for (int y = 0; y < 22; ++y) {
    for (int x = 0; x < 40; ++x) {
      if (maze.getCell(x, y) == Maze::CELL_ROOM) {
        roomCountAfter++;
      }
    }
  }

  // We expect greater than 0 room tiles to exist
  EXPECT_GT(roomCountAfter, 0);
}

// 7. Test Corridor Generation (Prim's Algorithm)
TEST(MazeTest, CorridorGenerationCarvesFloors) {
  Maze maze(40, 22, 32, 12345);

  // Generate rooms first (corridors need rooms to grow from)
  std::mt19937 rng(12345);
  BSPGenerator bsp;
  bsp.generate(maze, rng);

  // 1. Before corridors, there should be zero CELL_FLOOR tiles
  //    (BSP only creates CELL_ROOM tiles, not CELL_FLOOR)
  int floorCountBefore = 0;
  for (int y = 0; y < 22; ++y) {
    for (int x = 0; x < 40; ++x) {
      if (maze.getCell(x, y) == Maze::CELL_FLOOR) {
        ++floorCountBefore;
      }
    }
  }
  EXPECT_EQ(floorCountBefore, 0);

  // 2. Execute Prim's corridor generation
  PrimsGenerator prims;
  prims.generate(maze, rng, bsp.getMiddleRoomIndex());

  // 3. After corridors, we should have many CELL_FLOOR tiles
  int floorCountAfter = 0;
  for (int y = 0; y < 22; ++y) {
    for (int x = 0; x < 40; ++x) {
      if (maze.getCell(x, y) == Maze::CELL_FLOOR) {
        ++floorCountAfter;
      }
    }
  }

  // The mold should have carved a significant number of corridor tiles
  EXPECT_GT(floorCountAfter, 0);
}

// 8. Test 100% Connectivity (Tunnel Borer)
TEST(MazeTest, FullConnectivityEnsured) {
  // We use a random seed so the test validates a new maze structure every
  // single run! If a specific test breaks, you can manually hardcode the seed
  // here to reproduce it:
  // unsigned int seed = 7;
  // unsigned int seed = 1781431085;
  unsigned int seed = std::time(nullptr);

  // Record the seed in the GTest XML results for CI/CD tracking
  testing::Test::RecordProperty("RandomSeed", std::to_string(seed));

  Maze maze(250, 150, 32, seed);

  // Run the full generation pipeline
  std::mt19937 rng(seed);
  BSPGenerator bsp;
  bsp.generate(maze, rng);
  PrimsGenerator prims;
  prims.generate(maze, rng, bsp.getMiddleRoomIndex());
  LoopGenerator loops;
  loops.generate(maze, rng);
  TunnelBorer borer;
  borer.ensureConnectivity(maze);

  // 1. Get total valid cells (O(1) lookup!)
  int totalValidCells = maze.getNonWallCount();
  int startX = -1, startY = -1;

  // Find the first valid cell to use as our BFS starting point
  for (int y = 0; y < maze.getHeight(); ++y) {
    for (int x = 0; x < maze.getWidth(); ++x) {
      if (maze.getCell(x, y) != Maze::CELL_WALL) {
        startX = x;
        startY = y;
        break; // Found it, stop searching!
      }
    }
    if (startX != -1)
      break;
  }

  // Ensure we actually generated a maze!
  EXPECT_GT(totalValidCells, 0);
  EXPECT_NE(startX, -1);

  // 2. Run a BFS to see how many cells are actually reachable
  int reachableCells = 0;
  std::vector<bool> visited(maze.getWidth() * maze.getHeight(), false);
  std::queue<std::pair<int, int>> q;

  q.push({startX, startY});
  visited[maze.getIndex(startX, startY)] = true;
  reachableCells++;

  const int dx[] = {0, 1, 0, -1};
  const int dy[] = {-1, 0, 1, 0};

  while (!q.empty()) {
    auto [cx, cy] = q.front();
    q.pop();

    for (int i = 0; i < 4; ++i) {
      int nx = cx + dx[i];
      int ny = cy + dy[i];

      if (maze.isInBounds(nx, ny)) {
        int nIndex = maze.getIndex(nx, ny);
        if (!visited[nIndex] && maze.getCell(nx, ny) != Maze::CELL_WALL) {
          visited[nIndex] = true;
          q.push({nx, ny});
          reachableCells++;
        }
      }
    }
  }

  // 3. The true test of connectivity:
  // Are the cells we can walk to equal to the total number of walk-able cells?
  // If not, it means there are isolated islands we couldn't reach!
  EXPECT_EQ(reachableCells, totalValidCells)
      << "\n============================================\n"
      << "ISOLATED ROOMS DETECTED!\n"
      << "To reproduce this exact maze, hardcode this seed:\n"
      << "unsigned int seed = " << seed << ";\n"
      << "============================================\n";
}
