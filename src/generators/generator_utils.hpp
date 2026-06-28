#pragma once
#include "../maze.hpp"
#include <vector>
#include <random>

namespace GeneratorUtils {

// --- Directional Constants ---
// Cardinal directions (Right, Left, Down, Up)
constexpr int DX[] = {1, -1, 0, 0};
constexpr int DY[] = {0, 0, 1, -1};

// Diagonal directions (Top-Left, Bottom-Right, Top-Right, Bottom-Left)
constexpr int DIAG_DX[] = {-1, 1, -1, 1};
constexpr int DIAG_DY[] = {-1, 1, 1, -1};

// --- Shared Utility Functions ---
// Counts how many cardinal neighbors are already carved (CELL_ROOM or CELL_CORRIDOR)
int countCarvedNeighbors(const Maze& maze, int x, int y);

// Scans the perimeter of a list of rooms and explicitly punches exactly one door
// per room to connect it to the surrounding corridor network.
void punchDoorways(Maze& maze, std::mt19937& rng, const std::vector<Maze::Room>& roomsToProcess);

} // namespace GeneratorUtils
