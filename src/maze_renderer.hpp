#pragma once
#include "maze.hpp"
#include <raylib.h>

// ============================================================================
// MazeRenderer Class
// ============================================================================
// Responsible for drawing the Maze to the screen using Raylib.
// This implements the Strategy Pattern by separating the presentation logic 
// from the core data structure (Maze).
// ============================================================================
class MazeRenderer {
public:
  MazeRenderer();
  ~MazeRenderer();

  void loadTextures();

  // Draw the maze based on context (Corridor vs Room) and Frustum Culling
  void render(const Maze &maze, const Camera2D &camera, AreaState state) const;

private:
  Texture2D m_floorTileset;
  Texture2D m_wallTileset;
  Texture2D m_propTileset;
};
