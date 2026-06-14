#pragma once
#include "../maze.hpp"
#include <random>
// ============================================================================
// PrimsGenerator
// ============================================================================
// Uses a modified Prim's Algorithm to "grow" a maze through the empty space
// between the BSP rooms. This creates a highly-branched, chaotic web of 
// narrow corridors that connects the entire level.
// ============================================================================
class PrimsGenerator {
public:
    void generate(Maze& maze, std::mt19937& rng);
};
