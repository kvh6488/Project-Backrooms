#pragma once

#include "maze.hpp"
#include "maze_renderer.hpp"
#include "player.hpp"
#include "player_renderer.hpp"
#include "item_spawner.hpp"
#include "raylib.h"
#include <random>

// The Application class encapsulates the entire game loop and engine state.
// This is an implementation of the Facade Pattern, hiding the complex
// interactions of the subsystems (Maze, Player, Camera, Renderer) from
// main.cpp.
class Application {
public:
  Application();
  ~Application();

  // Starts the main game loop
  void run();

private:
  // --- Game Loop Phases ---
  void update();
  void render();
  void renderUI(); // TODO: Extract to a separate UIManager class later

  // --- Helper Methods ---
  void generateMinimap();
  void regenerateTicTacToeZones();

  // --- Window Configuration ---
  const int m_screenWidth = 1280;
  const int m_screenHeight = 720;

  // --- Core Systems ---
  unsigned int m_seed;
  std::mt19937 m_rng;       // Application-wide RNG, seeded once for determinism
  Maze m_maze;
  Player m_player;
  Camera2D m_camera;
  MazeRenderer m_renderer;
  PlayerRenderer m_playerRenderer;
  ItemSpawner m_itemSpawner; // Decoupled item placement logic

  // --- UI / Minimap State ---
  RenderTexture2D m_minimapTexture;
  bool m_minimapDirty;
  
  // UI / Popups
  float m_popupTimer = 0.0f;
  std::string m_popupText = "";

  // --- Popups ---
  bool m_hasSeenCorridorPopup = false;
  float m_corridorPopupTimer = 0.0f;
  
  bool m_hasSeenRadiationPopup = false;
  float m_radiationPopupTimer = 0.0f;

  // --- Radiation Flicker ---
  float m_radiationFlickerTimer = 0.0f;
  float m_radiationNextFlickerTime = 4.0f;
  float m_radiationDarknessAlpha = 0.0f;

  // --- Debug / Toggles ---
  bool m_flashlightEnabled = true;
  bool m_showGenerationZones = false;
  bool m_showRadiationOnMinimap = false;

  // --- Flashlight Overlay Settings ---
  float m_lightConeAngle = 235.0f;
  float m_lightFadeStrength = 1.5f;
  float m_lightSizeScale = 3.5f;

  // --- View Settings ---
  float m_cameraZoom = 1.2f;
};
