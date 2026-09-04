#pragma once

#include "dev/debug_overlay.hpp"
#include "ui/ui_manager.hpp"
#include "states/game_state.hpp"
#include <memory>


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

  // Starts the main game loop
  void run();

private:
  // --- Window Configuration ---
  const int m_screenWidth = 1280;
  const int m_screenHeight = 720;

  unsigned int m_seed;

  UIManager m_uiManager;
  // Owned here rather than by a state so the panel survives future state
  // switches (menu, death screen) and F1 keeps working across them.
  DebugOverlay m_debugOverlay;
  std::unique_ptr<GameState> m_currentState;
};
