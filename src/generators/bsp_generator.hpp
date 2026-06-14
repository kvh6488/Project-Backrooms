#pragma once
#include "../maze.hpp"
#include <random>
#include <memory>

class BSPGenerator {
public:
    void generate(Maze& maze, std::mt19937& rng);

private:
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

    void mergeAdjacentRooms(Maze& maze, std::mt19937& rng);
};
