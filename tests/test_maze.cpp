#include <gtest/gtest.h>
#include "maze.hpp"

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
    for(int y = 0; y < 22; ++y) {
        for(int x = 0; x < 40; ++x) {
            if(maze.getCell(x, y) == Maze::CELL_ROOM) {
                roomCountBefore++;
            }
        }
    }
    EXPECT_EQ(roomCountBefore, 0);

    // 2. Execute the BSP algorithm
    maze.generateBSP();

    // 3. After generation, we should have successfully carved hundreds of room tiles
    int roomCountAfter = 0;
    for(int y = 0; y < 22; ++y) {
        for(int x = 0; x < 40; ++x) {
            if(maze.getCell(x, y) == Maze::CELL_ROOM) {
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
    maze.generateBSP();

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
    maze.generateCorridors();

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
