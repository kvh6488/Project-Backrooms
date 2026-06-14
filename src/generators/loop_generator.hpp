#pragma once
#include "../maze.hpp"
#include <random>

class LoopGenerator {
public:
    void generate(Maze& maze, std::mt19937& rng);
};
