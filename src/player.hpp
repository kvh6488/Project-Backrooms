#pragma once

#include "maze.hpp"
#include "raylib.h"

// ============================================================================
// Player Class
// ============================================================================
// Handles the player's position, movement (kinematics), and collision detection
// against the Maze's grid cells.
// ============================================================================

class Player {
public:
  // --- Constructor ---
  // Initializes the player at a starting position with a defined size and
  // speed.
  Player(Vector2 startPosition, AreaState startState = AreaState::CORRIDOR);

  // --- Core Methods ---

  // Updates player logic every frame (Input -> Kinematics -> Collision)
  void update(const Maze &maze);

  // Renders the player as a circle onto the screen
  void render() const;

  // Getters
  Vector2 getPosition() const { return m_position; }
  AreaState getAreaState() const { return m_areaState; }

  // Check if player is near a valid door
  bool canUseDoor(const Maze& maze) const;

private:
  Vector2 m_position; // Center of the player in world space (pixels)
  float m_speed;      // Movement speed in pixels per second
  float m_radius;     // Size of the player for the Circle Hitbox

  AreaState m_areaState;

  // --- Internal Collision Helpers ---

  // Checks if the player's circle hitbox overlaps any solid wall in the maze.
  // We use the "Slide" method and radial push to keep the player out of walls.
  void resolveCollision(const Maze &maze);
};
