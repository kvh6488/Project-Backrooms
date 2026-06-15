#pragma once
#include "../maze.hpp"
// ============================================================================
// TunnelBorer
// ============================================================================
// Uses an intelligent two-pass Breadth-First Search (BFS) to find completely
// isolated rooms that Prim's algorithm missed, and carves straight tunnels
// through solid rock to ensure 100% connectivity across the entire maze.
//
// Time Complexity: O(V^2) Worst Case (Upper Bound), Closer to O(V) in Average
// Case. It runs a full O(V) Breadth-First Search for every isolated room it
// finds.
// ============================================================================
class TunnelBorer {
public:
  void ensureConnectivity(Maze &maze);
};
