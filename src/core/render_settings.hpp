#pragma once

// ============================================================================
// RenderSettings — Tunable presentation values
// ============================================================================
// These are read every frame by PlayingState and MazeRenderer, so they must
// exist whether or not the debug overlay does. DebugOverlay edits them through
// a reference; it does not own them. That keeps the overlay a pure view and
// makes it removable without taking rendering config with it.
//
// The defaults are the shipping values — the ones the game uses when nobody
// touches a slider.
// ============================================================================

struct RenderSettings {
  // --- Flashlight / corridor light mask ---
  bool flashlightEnabled = true;
  float lightConeAngle = 235.0f;   // degrees of the visible cone
  float lightFadeStrength = 1.5f;  // falloff sharpness at the cone edges
  float lightSizeScale = 3.5f;     // cone radius, in tiles

  // Raised when any of the three light values above changes. PlayingState
  // polls and clears it, then rebuilds the mask texture — regenerating it
  // every frame would be wasteful.
  bool lightSettingsChanged = false;

  // --- Camera ---
  float cameraZoom = 1.2f;

  // --- Debug visualisation ---
  // Tints the Tic-Tac-Toe regeneration zones in the world and on the minimap.
  bool showGenerationZones = false;
};
