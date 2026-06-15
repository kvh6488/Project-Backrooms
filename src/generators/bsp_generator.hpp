#pragma once
#include "../maze.hpp"
#include <memory>
#include <random>
// ============================================================================
// BSPGenerator
// ============================================================================
// Implements Binary Space Partitioning. It recursively slices the maze into
// smaller rectangles (leaves), and then carves random "office rooms" inside
// those leaves. This ensures rooms are well-distributed and non-overlapping.
//
// Time Complexity: O(R^2) where R is the number of rooms, because the final
// adjacent room merging step compares every leaf to every other leaf. The
// recursive splitting itself is O(R log R).
// ============================================================================
class BSPGenerator {
public:
  // Carve rooms in the entire maze
  void generate(Maze &maze, std::mt19937 &rng);
  void generateZone(Maze& maze, std::mt19937& rng, int startX, int startY, int width, int height);
  int getMiddleRoomIndex() const { return m_middleRoomIndex; }

private:
  int m_middleRoomIndex = -1;
  struct BSPLeaf {
    int x, y, width, height;
    std::shared_ptr<BSPLeaf> leftChild;
    std::shared_ptr<BSPLeaf> rightChild;

    int roomX, roomY, roomWidth, roomHeight;
    bool hasRoom = false;

    BSPLeaf(int _x, int _y, int _w, int _h)
        : x(_x), y(_y), width(_w), height(_h) {}

    bool split(std::mt19937 &rng);
    void createRooms(Maze &maze, std::mt19937 &rng);
  };

  std::shared_ptr<BSPLeaf> m_rootLeaf;
  std::vector<std::shared_ptr<BSPLeaf>> m_leaves;

  void mergeAdjacentRooms(Maze &maze, std::mt19937 &rng);
};
