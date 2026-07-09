#pragma once

#include "core/ui_manager.hpp"
#include "states/game_state.hpp"
#include <memory>


// The Application class encapsulates the window and state machine.
class Application {
public:
  Application();
  ~Application();

  // Starts the main game loop
  void run();

private:
  // --- Window Configuration ---
  const int m_screenWidth = 1280;
  const int m_screenHeight = 720;

  UIManager m_uiManager;
  std::unique_ptr<GameState> m_currentState;
};
