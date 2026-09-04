#include "core/application.hpp"
#include "core/debug_log.hpp"
#include "items/item_database.hpp"
#include "items/crafting_system.hpp"
#include "rlImGui.h"
#include "states/playing_state.hpp"


Application::Application(unsigned int seed, const char *seedNote, bool devMode)
    : m_seed(seed), m_uiManager(m_screenWidth, m_screenHeight),
      m_debugOverlay(devMode) {
  // 0. Logging first, so everything below is readable.
  debuglog::enableAnsiColors();
  // Raylib's own chatter is also tagged "[INFO]", which drowns our messages.
  // Warnings and errors still come through, so real failures (a texture that
  // did not load) remain visible.
  SetTraceLogLevel(LOG_WARNING);
  debuglog::log("SEED", "%u  (%s)", seed,
                seedNote ? seedNote : "unspecified");
  debuglog::log("SEED", "reproduce with:  Backrooms.exe --seed %u", seed);
  if (devMode) {
    debuglog::log("DEV", "debug tools armed  (F1 toggles the panel)");
  }

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

  // 2.5 Initialize Item Database
  ItemDatabase::init();
  CraftingSystem::init();

  // 3. Set Initial State
  m_currentState =
      std::make_unique<PlayingState>(m_uiManager, m_debugOverlay, m_seed);
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
    if (IsKeyPressed(KEY_F1)) {
      m_debugOverlay.toggle();
    }

    if (m_currentState) {
      m_currentState->update(GetFrameTime());
      m_currentState->render();
    }
  }
}
