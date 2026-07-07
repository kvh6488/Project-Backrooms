#pragma once
#include "maze.hpp"
#include <raylib.h>

// ============================================================================
// MazeRenderer Class
// ============================================================================
// Responsible for drawing the Maze to the screen using Raylib.
// This implements the Strategy Pattern by separating the presentation logic
// from the core data structure (Maze).
//
// The darkness overlay system uses a pre-generated radial gradient texture
// composited via Raylib's blend modes to create a smooth "flashlight" effect
// in corridors. This is the "screen-space overlay" approach from the plan.
// ============================================================================
class MazeRenderer {
public:
  MazeRenderer();
  ~MazeRenderer();

  void loadTextures();

  // Draw the maze based on context (Corridor vs Room) and Frustum Culling
  // camera: the active camera, used for view culling (frustum culling)
  void render(const Maze &maze, const Camera2D &camera, AreaState state,
              bool showGenerationZones = true) const;

  // Step 1: Builds the light mask off-screen (Call BEFORE BeginTextureMode!)
  void buildLightMask(Vector2 playerWorldPos, const Camera2D &camera,
                      AreaState state, FacingDirection dir);

  // Step 2: Draws the built light mask (Call inside BeginTextureMode!)
  void drawLightMask();

  // Updates flashlight parameters dynamically. Regenerates gradient if needed.
  void updateLightSettings(float coneAngle, float fadeStrength,
                           float sizeScale);

private:
  Texture2D m_floorTileset;
  Texture2D m_wallTileset;
  Texture2D m_propTileset;

  // --- Darkness Overlay ---
  // A pre-generated radial gradient texture: white at center, fading to
  // transparent at edges. Used as a "stamp" in the light mask.
  Texture2D m_lightGradient;

  float m_lightConeAngle = 235.0f;
  float m_lightFadeStrength = 2.0f;
  float m_lightSizeScale = 3.125f;

  // Off-screen render texture used as a "light mask".
  // Each frame in corridor mode, we:
  //   1. Clear this to BLACK (total darkness)
  //   2. Draw the gradient circle onto it (white = lit area)
  //   3. Multiply this mask onto the main framebuffer
  // This avoids the ordering bug of drawing black directly onto the
  // framebuffer.
  RenderTexture2D m_lightMask;
  bool m_lightMaskReady;

  // Generates the radial gradient texture for the flashlight effect
  void generateLightGradient(int diameter);

  // Initializes (or re-initializes) the light mask RenderTexture to match
  // the current screen size. Called on first use and on window resize.
  void initLightMask();
};
