#pragma once
#include "../maze.hpp"
#include <random>

class PrimsGenerator {
public:
    void generate(Maze& maze, std::mt19937& rng);
};
