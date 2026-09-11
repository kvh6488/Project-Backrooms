#include "core/application.hpp"
#include "dev/debug_log.hpp"
#include "items/item_database.hpp"
#include "items/crafting_system.hpp"
#include "imgui.h"
#include "rlImGui.h"
#include "states/playing_state.hpp"


Application::Application(unsigned int seed, const char *seedNote, bool devMode)
    : m_seed(seed), m_input(std::make_unique<HardwareInput>()),
      m_uiManager(m_screenWidth, m_screenHeight),
      m_debugOverlay(devMode) {
  // 0. main() has already armed the logger (it logs before we exist).
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

void Application::run(const RunConfig &config) {
  for (int tick = 0; !WindowShouldClose(); ++tick) {
    if (config.maxTicks >= 0 && tick >= config.maxTicks) {
      break;
    }

    // F1 is deliberately outside InputState: it is a dev-tool switch, not a
    // player action, and a scripted scenario must not be able to reach it.
    if (IsKeyPressed(KEY_F1)) {
      m_debugOverlay.toggle();
    }

    InputState in = m_input->sample(tick);
    // A click on the debug panel is the panel's, not the game's. Without this
    // gate a slider drag also placed items and picked up inventory.
    if (m_debugOverlay.isVisible() && ImGui::GetIO().WantCaptureMouse) {
      in.mouseLeftPressed = false;
      in.mouseRightPressed = false;
    }

    if (m_currentState) {
      m_currentState->update(kFixedDt, in);
      m_currentState->render(in);

      // Honour a transition only here, after the frame is fully drawn. The
      // state that raised the request is still live during update/render, so
      // destroying it any earlier would pull the object out from under itself.
      if (m_currentState->hasPendingTransition()) {
        if (m_currentState->wantsQuit()) {
          break; // ~Application calls onExit on the way out
        }
        std::unique_ptr<GameState> next = m_currentState->takeNextState();
        m_currentState->onExit();
        m_currentState = std::move(next);
        m_currentState->onEnter();
      }
    }
  }
}
