#pragma once

#include "core/ui_manager.hpp"
#include "states/game_state.hpp"
#include <memory>


// The Application class encapsulates the window and state machine.
class Application {
public:
  // seed fully determines world generation; seedNote is a human-readable
  // description of where that seed came from, purely for the startup log.
  explicit Application(unsigned int seed, const char *seedNote = nullptr);
  ~Application();

  // Starts the main game loop
  void run();

private:
  // --- Window Configuration ---
  const int m_screenWidth = 1280;
  const int m_screenHeight = 720;

  unsigned int m_seed;

  UIManager m_uiManager;
  std::unique_ptr<GameState> m_currentState;
};
