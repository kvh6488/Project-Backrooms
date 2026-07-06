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

// 4. Test setCell and getCell
TEST(MazeTest, CellReadWrite) {
  Maze maze(10, 10, 32, 12345);
  EXPECT_EQ(maze.getCell(3, 3), Maze::CELL_WALL);

  maze.setCell(3, 3, Maze::CELL_CORRIDOR);
  EXPECT_EQ(maze.getCell(3, 3), Maze::CELL_CORRIDOR);

  maze.setCell(8, 8, Maze::CELL_ROOM);
  EXPECT_EQ(maze.getCell(8, 8), Maze::CELL_ROOM);
}

// 5. Test Toroidal Modulo Wrapping
TEST(MazeTest, ToroidalWrapping) {
  Maze maze(10, 10, 32, 12345);

  // Write to a coordinate physically outside the 10x10 grid.
  // X = -5 should wrap to X = 5
  // Y = 20 should wrap to Y = 0
  maze.setCell(-5, 20, Maze::CELL_CORRIDOR);
  
  // Verify that the mathematical wrap successfully mapped it to (5, 0)
  EXPECT_EQ(maze.getCell(5, 0), Maze::CELL_CORRIDOR);

  // Verify that reading from (-5, 20) also fetches the wrapped cell (5, 0)
  EXPECT_EQ(maze.getCell(-5, 20), Maze::CELL_CORRIDOR);
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

  // 1. Before corridors, there should be zero CELL_CORRIDOR tiles
  //    (BSP only creates CELL_ROOM tiles, not CELL_CORRIDOR)
  int floorCountBefore = 0;
  for (int y = 0; y < 22; ++y) {
    for (int x = 0; x < 40; ++x) {
      if (maze.getCell(x, y) == Maze::CELL_CORRIDOR) {
        ++floorCountBefore;
      }
    }
  }
  EXPECT_EQ(floorCountBefore, 0);

  // 2. Execute Prim's corridor generation
  PrimsGenerator prims;
  prims.generate(maze, rng, bsp.getMiddleRoomIndex());

  // 3. After corridors, we should have many CELL_CORRIDOR tiles
  int floorCountAfter = 0;
  for (int y = 0; y < 22; ++y) {
    for (int x = 0; x < 40; ++x) {
      if (maze.getCell(x, y) == Maze::CELL_CORRIDOR) {
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

      int nIndex = maze.getIndex(nx, ny);
      if (!visited[nIndex] && maze.getCell(nx, ny) != Maze::CELL_WALL) {
        visited[nIndex] = true;
        q.push({nx, ny});
        reachableCells++;
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

TEST(MazeTest, ZoneRegenerationConnectivity) {
  unsigned int seed = 123456789;
  std::mt19937 rng(seed);
  Maze maze(250, 150, 32, seed);

  // 1. Initial full generation
  BSPGenerator bsp;
  bsp.generate(maze, rng);
  PrimsGenerator prims;
  prims.generate(maze, rng, bsp.getMiddleRoomIndex());
  LoopGenerator loops;
  loops.generate(maze, rng);
  TunnelBorer borer;
  borer.ensureConnectivity(maze);

  // 2. Erase and regenerate the Tic-Tac-Toe zones
  std::vector<Maze::Room> zones = {
      {55, 0, 14, 150},   // V-Left
      {180, 0, 14, 150},  // V-Right
      {0, 30, 55, 14},    // H-Top-Left
      {69, 30, 111, 14},  // H-Top-Mid
      {194, 30, 56, 14},  // H-Top-Right
      {0, 105, 55, 14},   // H-Bot-Left
      {69, 105, 111, 14}, // H-Bot-Mid
      {194, 105, 56, 14}  // H-Bot-Right
  };

  for (const auto& z : zones) {
    maze.eraseZone(z.x, z.y, z.width, z.height);
  }
  for (const auto& z : zones) {
    bsp.generateZone(maze, rng, z.x, z.y, z.width, z.height);
  }
  for (const auto& z : zones) {
    prims.generateZone(maze, rng, z.x, z.y, z.width, z.height);
  }
  for (const auto& z : zones) {
    loops.generateZone(maze, rng, z.x, z.y, z.width, z.height);
  }
  borer.ensureConnectivity(maze);

  // 4. Test connectivity
  int totalValidCells = maze.getNonWallCount();
  int startX = -1, startY = -1;

  for (int y = 0; y < maze.getHeight(); ++y) {
    for (int x = 0; x < maze.getWidth(); ++x) {
      if (maze.getCell(x, y) != Maze::CELL_WALL) {
        startX = x;
        startY = y;
        break;
      }
    }
    if (startX != -1) break;
  }

  EXPECT_GT(totalValidCells, 0);
  EXPECT_NE(startX, -1);

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

      int nIndex = maze.getIndex(nx, ny);
      if (!visited[nIndex] && maze.getCell(nx, ny) != Maze::CELL_WALL) {
        visited[nIndex] = true;
        q.push({nx, ny});
        reachableCells++;
      }
    }
  }

  EXPECT_EQ(reachableCells, totalValidCells)
      << "\n============================================\n"
      << "ZONE REGENERATION BROKE CONNECTIVITY!\n"
      << "Seed: " << seed << "\n"
      << "============================================\n";
}

TEST(MazeTest, NoDiagonalLeaks) {
  unsigned int seed = 987654321;
  std::mt19937 rng(seed);
  Maze maze(250, 150, 32, seed);

  auto scanForLeaks = [&](const std::string& stepName) {
    bool foundDiagonalLeak = false;
    int leakX = -1, leakY = -1;

    for (int y = 0; y < maze.getHeight(); ++y) {
      for (int x = 0; x < maze.getWidth(); ++x) {
        int tMain = maze.getCell(x, y);
        if (tMain == Maze::CELL_CORRIDOR || tMain == Maze::CELL_ROOM) {
          if (maze.hasDiagonalLeak(x, y)) {
            foundDiagonalLeak = true;
            leakX = x;
            leakY = y;
            break;
          }
        }
      }
      if (foundDiagonalLeak) break;
    }

    EXPECT_FALSE(foundDiagonalLeak) << "Diagonal leak found at (" << leakX << ", " << leakY << ") after " << stepName << "!";
  };

  // 1. Initial full generation
  BSPGenerator bsp;
  bsp.generate(maze, rng);
  PrimsGenerator prims;
  prims.generate(maze, rng, bsp.getMiddleRoomIndex());
  LoopGenerator loops;
  loops.generate(maze, rng);
  TunnelBorer borer;
  borer.ensureConnectivity(maze);

  // 2. Scan after initial generation
  scanForLeaks("Initial Generation");

  // 3. Erase and regenerate the Tic-Tac-Toe zones to test sleep mutation
  std::vector<Maze::Room> zones = {
      {55, 0, 14, 150},   // V-Left
      {180, 0, 14, 150},  // V-Right
      {0, 30, 55, 14},    // H-Top-Left
      {69, 30, 111, 14},  // H-Top-Mid
      {194, 30, 56, 14},  // H-Top-Right
      {0, 105, 55, 14},   // H-Bot-Left
      {69, 105, 111, 14}, // H-Bot-Mid
      {194, 105, 56, 14}  // H-Bot-Right
  };

  for (const auto& z : zones) {
    maze.eraseZone(z.x, z.y, z.width, z.height);
  }
  for (const auto& z : zones) {
    bsp.generateZone(maze, rng, z.x, z.y, z.width, z.height);
  }
  for (const auto& z : zones) {
    prims.generateZone(maze, rng, z.x, z.y, z.width, z.height);
  }
  for (const auto& z : zones) {
    loops.generateZone(maze, rng, z.x, z.y, z.width, z.height);
  }
  borer.ensureConnectivity(maze);

  // 4. Final scan after regeneration
  scanForLeaks("Zone Regeneration");
}

// 10. Test Uniform Room Radiation
TEST(MazeTest, UniformRoomRadiation) {
  // Test with 5 different seeds
  unsigned int seeds[5] = {101, 202, 303, 404, 505};
  
  for (int i = 0; i < 5; ++i) {
    unsigned int seed = seeds[i];
    std::mt19937 rng(seed);
    Maze maze(100, 100, 32, seed);
    
    // Generate full maze
    BSPGenerator bsp;
    bsp.generate(maze, rng);
    PrimsGenerator prims;
    prims.generate(maze, rng, bsp.getMiddleRoomIndex());
    LoopGenerator loops;
    loops.generate(maze, rng);
    TunnelBorer borer;
    borer.ensureConnectivity(maze);
    prims.pruneSmallAlcoves(maze, 5);
    
    // Spawn barrels
    maze.spawnRadiationBarrels(15);
    
    // Check uniform radiation in every contiguous CELL_ROOM component
    std::vector<bool> visited(maze.getWidth() * maze.getHeight(), false);
    
    for (int y = 0; y < maze.getHeight(); ++y) {
      for (int x = 0; x < maze.getWidth(); ++x) {
        int startIdx = maze.getIndex(x, y);
        if (maze.getCell(x, y) == Maze::CELL_ROOM && !visited[startIdx]) {
          
          bool isComponentRadiated = maze.getRadiationLevel(x, y) > 0;
          
          // Flood fill to find all contiguous room tiles
          std::deque<std::pair<int, int>> q;
          q.push_back({x, y});
          visited[startIdx] = true;
          
          while (!q.empty()) {
            auto [cx, cy] = q.front();
            q.pop_front();
            
            bool currentRadiated = maze.getRadiationLevel(cx, cy) > 0;
            EXPECT_EQ(currentRadiated, isComponentRadiated) 
                << "Mismatch in contiguous room radiation! Seed: " << seed 
                << " at (" << cx << "," << cy << ")";
            
            const int dx[] = {1, -1, 0, 0};
            const int dy[] = {0, 0, 1, -1};
            for (int d = 0; d < 4; ++d) {
              int nx = (cx + dx[d]) % maze.getWidth();
              if (nx < 0) nx += maze.getWidth();
              int ny = (cy + dy[d]) % maze.getHeight();
              if (ny < 0) ny += maze.getHeight();
              
              int nIdx = maze.getIndex(nx, ny);
              if (maze.getCell(nx, ny) == Maze::CELL_ROOM && !visited[nIdx]) {
                visited[nIdx] = true;
                q.push_back({nx, ny});
              }
            }
          }
        }
      }
    }
  }
}
