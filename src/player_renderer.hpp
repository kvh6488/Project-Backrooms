#pragma once

#include "player.hpp"
#include <raylib.h>

// ============================================================================
// PlayerRenderer Class
// ============================================================================
// Responsible for drawing the Player to the screen using Raylib.
// This implements the Strategy Pattern by separating the presentation logic 
// from the core data structure (Player).
//
// Animation is driven by a fixed-rate timer (not velocity-dependent).
// Standard practice in 2D top-down pixel games like The Escapists, Stardew
// Valley, and classic Zelda — the walk cycle plays at a constant cadence
// regardless of movement speed.
// ============================================================================
class PlayerRenderer {
public:
  PlayerRenderer();
  ~PlayerRenderer();

  void loadTextures();

  // Advance the animation timer and frame index based on player state.
  // Called once per frame from Application::update().
  void update(float dt, const Player &player);

  // Draw the player sprite. Pure read-only — no state mutation.
  void render(const Player &player) const;

private:
  Texture2D m_playerTexture;

  // --- Animation State ---
  int m_currentFrame;    // Current frame index in the walk cycle (0–3)
  float m_frameTimer;    // Accumulator tracking time since last frame advance

  // Walk cycle plays at ~6.7 FPS (0.15s per frame).
  // This is a tunable constant — increase for slower animation, decrease for
  // faster. Standard range for pixel walk cycles is 0.10–0.20s.
  static constexpr float FRAME_DURATION = 0.20f;

  // Spritesheet constants
  static constexpr int TILE_SIZE = 16;   // Each tile is 16x16 pixels
  static constexpr int FRAME_COUNT = 4;  // 4 frames per walk cycle row
};
