#pragma once

#include "dev/debug_overlay.hpp"
#include "core/render_settings.hpp"
#include "ui/ui_manager.hpp"
#include "entities/player.hpp"
#include "render/player_renderer.hpp"
#include "render/item_renderer.hpp"
#include "raylib.h"
#include "states/game_state.hpp"
#include "world/item_spawner.hpp"
#include "world/maze.hpp"
#include "render/maze_renderer.hpp"
#include <random>
#include <vector>


// The PlayingState represents the core gameplay loop (exploring the maze).
class PlayingState : public GameState {
public:
  // seed of 0 means "pick one from the clock".
  PlayingState(UIManager &uiManager, DebugOverlay &debugOverlay,
               unsigned int seed = 0);
  ~PlayingState() override;

  void onEnter() override;
  void onExit() override;
  void update(float dt) override;
  void render() override;

  // The eight shifting strips, derived from the world's dimensions rather than
  // hardcoded to 250x150. A pure function of its arguments — no instance state
  // — so the layout can be pinned by a test without standing up a window.
  static std::vector<Maze::Room> buildTicTacToeZones(int width, int height,
                                                     int thickness);

private:
  void handleInput();
  void regenerateTicTacToeZones();

  // Width of every shifting strip, in cells. The roadmap plans to vary this
  // per night as the run escalates.
  // Keep it >= 6: BSPLeaf::createRooms assumes a leaf at least 6 cells across
  // and its room-size distribution inverts below that.
  int m_zoneThickness = 14;

  // Reference to the global UI Manager
  UIManager &m_uiManager;
  // Development panel. It only edits m_renderSettings and raises request
  // flags; it never touches the world directly.
  DebugOverlay &m_debugOverlay;

  // Presentation values the debug panel tunes but the game owns.
  RenderSettings m_renderSettings;

  // --- Core Systems ---
  unsigned int m_seed;
  std::mt19937 m_rng;
  Maze m_maze;
  Player m_player;
  Camera2D m_camera;
  MazeRenderer m_renderer;
  ItemRenderer m_itemRenderer;
  PlayerRenderer m_playerRenderer;
  ItemSpawner m_itemSpawner;

  // --- Visual Effects ---
  Shader m_tripShader;
  int m_tripTimeLoc;
  int m_tripStrengthLoc;
  float m_totalTime;
  RenderTexture2D m_screenTarget;

  // Interaction State
  int m_focusedCupboardX = -1;
  int m_focusedCupboardY = -1;

  // Radiation rendering state
  float m_radiationFlickerTimer = 0.0f;
  float m_radiationDarknessAlpha = 0.0f;
  float m_radiationNextFlickerTime = 4.0f;

  // Attempts a magic book spawn at the player's current tile and records the
  // outcome in the UI. Shared by the mushroom-trip path and the debug button.
  void attemptMagicBookSpawn();

  // Apparent world-space shift the trip shader gives the scene at the magic
  // book's position this frame. See the implementation for the derivation.
  Vector2 computeTripFollowOffset() const;

  // --- State flags for Popups ---
  bool m_hasSeenCorridorPopup = false;
  bool m_inRadiationZone = false;
  bool m_isDroppingItem = false;
};
