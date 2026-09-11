#pragma once

#include "raylib.h"
#include <cmath>

// ============================================================================
// Viewport — the rectangle a camera projects into, in that camera's pixels
// ============================================================================
// The scene is drawn onto a CANVAS at camera.zoom = 1.0, so a 32px cell is
// exactly 32 texels, and the canvas is then stamped onto the window at
// RenderSettings::blitScale. Two consequences fall out of that:
//
//   canvas = ceil(window / scale)  - a bigger window is a bigger canvas and
//                                    shows MORE world; sprites never resize.
//   art pixels stay whole          - the canvas holds 2x2 art pixels, and the
//                                    blit is restricted to scales that keep
//                                    2 * scale an integer, so every art pixel
//                                    is a crisp NxN block on screen.
//
// Rounding UP means a window that does not divide evenly bleeds at most one
// canvas pixel off-screen instead of leaving a seam.
//
// A Viewport is just a size; which space it describes (canvas or window) is
// the caller's contract. Renderers take one alongside the camera instead of
// reading GetScreenWidth(), which is only ever the WINDOW size.
// ============================================================================
struct Viewport {
  int width = 0;
  int height = 0;

  static Viewport canvasFor(int windowW, int windowH, float blitScale) {
    if (blitScale < 1.0f)
      blitScale = 1.0f;
    return {(int)std::ceil(windowW / blitScale),
            (int)std::ceil(windowH / blitScale)};
  }

  Vector2 size() const { return {(float)width, (float)height}; }
  Vector2 center() const { return {width / 2.0f, height / 2.0f}; }
};
