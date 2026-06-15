#pragma once
#include "../maze.hpp"
#include <random>
// ============================================================================
// PrimsGenerator
// ============================================================================
// Uses a modified Prim's Algorithm to "grow" a maze through the empty space
// between the BSP rooms. This creates a highly-branched, chaotic web of 
// narrow corridors that connects the entire level.
//
// Time Complexity: O(V) where V is the total number of grid cells. By using
// a std::vector with O(1) swap-and-pop for the frontier list, we avoid the
// typical O(E log V) cost of standard Prim's with a priority queue.
// ============================================================================
class PrimsGenerator {
public:
    void generate(Maze& maze, std::mt19937& rng, int startRoomIndex);

private:
    int carveFromRoom(Maze& maze, std::mt19937& rng, int startRoomIndex);
};
