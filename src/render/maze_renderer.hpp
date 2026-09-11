#pragma once
#include "core/render_settings.hpp"
#include "core/viewport.hpp"
#include "world/maze.hpp"
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
  // canvas: the render-texture size the camera projects into
  void render(const Maze &maze, const Camera2D &camera, const Viewport &canvas,
              AreaState state, bool showGenerationZones = true) const;

  // Step 1: Builds the light mask off-screen (Call BEFORE BeginTextureMode!)
  // The mask lives in CANVAS space - same size and same camera as the scene -
  // so the cone lines up with the tiles whatever the window is doing.
  void buildLightMask(Vector2 playerWorldPos, const Camera2D &camera,
                      const Viewport &canvas, AreaState state,
                      FacingDirection dir);

  // Step 2: Draws the built light mask (Call inside BeginTextureMode!)
  void drawLightMask(const Viewport &canvas);

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

  // Seeded from RenderSettings so there is exactly one copy of the shipping
  // values; updateLightSettings() is the only writer after that.
  float m_lightConeAngle = RenderSettings{}.lightConeAngle;
  float m_lightFadeStrength = RenderSettings{}.lightFadeStrength;
  float m_lightSizeScale = RenderSettings{}.lightSizeScale;

  // Texel size of the gradient stamp. It is drawn at ~300 canvas px by
  // default, so 512 keeps the falloff smooth after scaling; the initial
  // build and every slider-driven rebuild use this one value.
  static constexpr int kLightGradientDiameter = 512;

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
  void generateLightGradient();

  // Initializes (or re-initializes) the light mask RenderTexture to match
  // the current screen size. Called on first use and on window resize.
  void initLightMask(const Viewport &canvas);
};
