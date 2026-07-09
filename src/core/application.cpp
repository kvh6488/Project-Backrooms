#include "core/application.hpp"
#include "rlImGui.h"
#include "states/playing_state.hpp"


Application::Application() : m_uiManager(m_screenWidth, m_screenHeight) {
  // 1. Initialize Raylib System
  SetConfigFlags(FLAG_WINDOW_RESIZABLE);
  InitWindow(m_screenWidth, m_screenHeight, "Project Backrooms");
  SetWindowMinSize(m_screenWidth, m_screenHeight);
  SetTargetFPS(60);

  // 1.5 Set Window Icon
  Image iconImage = LoadImage("assets/guard_yellow_spritesheet.png");
  if (IsImageReady(iconImage)) {
    ImageCrop(&iconImage, Rectangle{0, 0, 16, 16});
    SetWindowIcon(iconImage);
    UnloadImage(iconImage);
  }

  // 2. Initialize ImGui and Textures
  rlImGuiSetup(true);

  // 3. Set Initial State
  m_currentState = std::make_unique<PlayingState>(m_uiManager);
  m_currentState->onEnter();
}

Application::~Application() {
  if (m_currentState) {
    m_currentState->onExit();
  }
  rlImGuiShutdown();
  CloseWindow();
}

void Application::run() {
  while (!WindowShouldClose()) {
    if (m_currentState) {
      m_currentState->update(GetFrameTime());
      m_currentState->render();
    }
  }
}
