#pragma once
#include "../maze.hpp"
// ============================================================================
// TunnelBorer
// ============================================================================
// Uses an intelligent two-pass Breadth-First Search (BFS) to find completely
// isolated rooms that Prim's algorithm missed, and carves straight tunnels 
// through solid rock to ensure 100% connectivity across the entire maze.
// ============================================================================
class TunnelBorer {
public:
    void ensureConnectivity(Maze& maze);
};
