#pragma once

#include "core/input_state.hpp"
#include "dev/debug_overlay.hpp"
#include "ui/ui_manager.hpp"
#include "states/game_state.hpp"
#include <memory>

// How run() should behave. Defaults are the shipping game; the headless
// harness fills in the rest.
struct RunConfig {
  int maxTicks = -1; // -1 = until the window closes
};


// The Application class encapsulates the window and state machine.
class Application {
public:
  // seed fully determines world generation; seedNote is a human-readable
  // description of where that seed came from, purely for the startup log.
  // devMode arms the debug overlay; without it F1 does nothing and the panel
  // never appears. See dev/dev_mode.hpp.
  explicit Application(unsigned int seed, const char *seedNote = nullptr,
                       bool devMode = false);
  ~Application();

  // The frame loop. Every tick advances the simulation by exactly kFixedDt
  // of game time regardless of wall-clock frame time: SetTargetFPS paces the
  // loop to real time when a window is up, and a headless run simply omits
  // the pacing to fast-forward. One code path for play and test.
  void run(const RunConfig &config = RunConfig{});

  static constexpr float kFixedDt = 1.0f / 60.0f;

private:
  // --- Window Configuration ---
  const int m_screenWidth = 1280;
  const int m_screenHeight = 720;

  unsigned int m_seed;

  // Where each tick's InputState comes from. Hardware in the shipping game.
  std::unique_ptr<InputSource> m_input;
  UIManager m_uiManager;
  // Owned here rather than by a state so the panel survives future state
  // switches (menu, death screen) and F1 keeps working across them.
  DebugOverlay m_debugOverlay;
  std::unique_ptr<GameState> m_currentState;
};
