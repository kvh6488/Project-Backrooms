#pragma once

#include "maze.hpp"
#include "maze_renderer.hpp"
#include "player.hpp"
#include "player_renderer.hpp"
#include "raylib.h"

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
  Maze m_maze;
  Player m_player;
  Camera2D m_camera;
  MazeRenderer m_renderer;
  PlayerRenderer m_playerRenderer;

  // --- UI / Minimap State ---
  RenderTexture2D m_minimapTexture;
  bool m_minimapDirty;

  // --- Popups ---
  bool m_hasSeenCorridorPopup = false;
  float m_corridorPopupTimer = 0.0f;

  // --- Debug / Toggles ---
  bool m_flashlightEnabled = true;
  bool m_showGenerationZones = false;
};
