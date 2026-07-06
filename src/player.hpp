#pragma once

#include "maze.hpp"
#include "raylib.h"
#include <array>

// ============================================================================
// FacingDirection Enum
// ============================================================================
// Encodes the four cardinal directions the player can face.
// The integer values (0–3) map directly to spritesheet rows, so the renderer
// can use them as a direct index: sourceY = facingDirection * tileHeight.
// ============================================================================
enum class FacingDirection {
  DOWN  = 0,  // Row 0 of spritesheet
  UP    = 1,  // Row 1
  LEFT  = 2,  // Row 2
  RIGHT = 3   // Row 3
};

// ============================================================================
// Player Class
// ============================================================================
// Handles the player's position, movement (kinematics), and collision detection
// against the Maze's grid cells.
// ============================================================================

// ============================================================================
// Inventory System
// ============================================================================
struct InventorySlot {
  ItemType type = ItemType::NONE;
  int count = 0;
};


class Player {
public:
  // --- Constructor ---
  // Initializes the player at a starting position with a defined size and
  // speed.
  Player(Vector2 startPosition, AreaState startState = AreaState::CORRIDOR);

  // --- Core Methods ---

  // Updates player logic every frame (Input -> Kinematics -> Collision)
  void update(Maze &maze);

  // Getters
  Vector2 getPosition() const { return m_position; }
  AreaState getAreaState() const { return m_areaState; }
  FacingDirection getFacingDirection() const { return m_facingDirection; }
  bool isMoving() const { return m_isMoving; }


  // Get the number of available doors near the player
  int getAvailableDoors(const Maze& maze) const;

  // --- Inventory System ---
  void pickupItem(Maze &maze);
  void dropItem(Maze &maze, int slotIndex);
  void destroyItem(int slotIndex);
  void swapSlots(int slotIndex1, int slotIndex2);
  const std::array<InventorySlot, 20>& getInventory() const { return m_inventory; }

private:
  std::array<InventorySlot, 20> m_inventory;

  Vector2 m_position; // Center of the player in world space (pixels)
  float m_speed;      // Movement speed in pixels per second
  float m_radius;     // Size of the player for the Circle Hitbox

  AreaState m_areaState;
  FacingDirection m_facingDirection; // Which direction the player sprite faces
  bool m_isMoving;                   // True when velocity is non-zero this frame


  // --- Internal Collision Helpers ---

  // Checks if the player's circle hitbox overlaps any solid wall in the maze.
  // We use the "Slide" method and radial push to keep the player out of walls.
  void resolveCollision(const Maze &maze);
};
