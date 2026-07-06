#include "application.hpp"
#include "generators/bsp_generator.hpp"
#include "generators/loop_generator.hpp"
#include "generators/prims_generator.hpp"
#include "generators/tunnel_borer.hpp"
#include "imgui.h"
#include "rlImGui.h"
#include <cmath>
#include <ctime>
#include <iostream>

Application::Application()
    : m_seed(std::time(nullptr)), m_rng(m_seed),
      m_maze(250, 150, 32, m_seed),
      m_player(Vector2{0, 0}, AreaState::ROOM),
      m_itemSpawner(m_rng), m_minimapDirty(false) {

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
  m_renderer.loadTextures();
  m_playerRenderer.loadTextures();

  // 3. Generate Initial Maze
  std::cout << "[INFO] Initializing Maze with seed: " << m_seed << std::endl;

  BSPGenerator bsp;
  bsp.generate(m_maze, m_rng);

  int midRoomIdx = bsp.getMiddleRoomIndex();

  PrimsGenerator prims;
  prims.generate(m_maze, m_rng, midRoomIdx);

  LoopGenerator loops;
  loops.generate(m_maze, m_rng);

  TunnelBorer borer;
  borer.ensureConnectivity(m_maze);

  // Post-Prims cleanup
  prims.pruneSmallAlcoves(m_maze, 5);

  // Phase 3: Spawn Initial Items (barrels, etc.) — count decided internally
  m_itemSpawner.spawnInitialItems(m_maze);

  // 4. Initialize Player Position
  Vector2 playerStartPos = {0.0f, 0.0f};
  if (!m_maze.getRooms().empty() && midRoomIdx >= 0) {
    const auto &closestRoom = m_maze.getRooms()[midRoomIdx];
    playerStartPos.x =
        (closestRoom.x + closestRoom.width / 2.0f) * m_maze.getCellSize();
    playerStartPos.y =
        (closestRoom.y + closestRoom.height / 2.0f) * m_maze.getCellSize();
  }

  // Reconstruct player with correct position
  m_player = Player(playerStartPos, AreaState::ROOM);

  // 5. Initialize Camera
  m_camera = {0};
  m_camera.target = m_player.getPosition();
  m_camera.offset = Vector2{m_screenWidth / 2.0f, m_screenHeight / 2.0f};
  m_camera.rotation = 0.0f;
  m_camera.zoom = 1.0f;

  // 6. Initialize Minimap
  m_minimapTexture = LoadRenderTexture(m_maze.getWidth(), m_maze.getHeight());
  generateMinimap();
}

Application::~Application() {
  UnloadRenderTexture(m_minimapTexture);
  rlImGuiShutdown();
  CloseWindow();
}

void Application::run() {
  while (!WindowShouldClose()) {
    update();
    render();
  }
}

void Application::update() {
  if (IsKeyPressed(KEY_F11)) {
    ToggleFullscreen();
  }

  if (IsKeyPressed(KEY_O)) {
    int px = static_cast<int>(
        std::floor(m_player.getPosition().x / m_maze.getCellSize()));
    int py = static_cast<int>(
        std::floor(m_player.getPosition().y / m_maze.getCellSize()));
    if (m_maze.isBarrelNear(px, py, 1)) {
      m_maze.destroyBarrelNear(px, py, 1);
      m_minimapDirty = true;
    }
  }

  // --- Pickup Interaction ---
  if (IsKeyPressed(KEY_P)) {
    int px = static_cast<int>(std::floor(m_player.getPosition().x / m_maze.getCellSize()));
    int py = static_cast<int>(std::floor(m_player.getPosition().y / m_maze.getCellSize()));
    
    bool pickedUp = false;
    for (int y = py - 1; y <= py + 1 && !pickedUp; ++y) {
      for (int x = px - 1; x <= px + 1 && !pickedUp; ++x) {
        ItemType type = m_maze.getItem(x, y);
        if (type == ItemType::MUSHROOM || type == ItemType::MAGIC_MUSHROOM) {
          m_maze.setItem(x, y, ItemType::NONE);
          m_popupText = (type == ItemType::MAGIC_MUSHROOM) ? "Picked up Magic Mushroom!" : "Picked up Mushroom!";
          m_popupTimer = 3.0f;
          pickedUp = true;
        }
      }
    }
  }

  if (m_popupTimer > 0.0f) {
    m_popupTimer -= GetFrameTime();
  }

  m_player.update(m_maze);

  // Convert player pixel position to grid coordinates for the FOV system
  int playerGridX = static_cast<int>(
      std::floor(m_player.getPosition().x / m_maze.getCellSize()));
  int playerGridY = static_cast<int>(
      std::floor(m_player.getPosition().y / m_maze.getCellSize()));

  // Compute FOV: rooms use full-lit flood fill.
  // Corridors ignore FOV and render all tiles, masked by the darkness overlay.
  m_maze.updateVisibility(playerGridX, playerGridY, m_player.getAreaState());

  m_playerRenderer.update(GetFrameTime(), m_player);

  if (m_minimapDirty) {
    generateMinimap();
    m_minimapDirty = false;
  }

  // Scale dynamically based on window size
  float scale = std::min((float)GetScreenWidth() / m_screenWidth,
                         (float)GetScreenHeight() / m_screenHeight);

  // Camera tracking: round to integer to prevent subpixel tile gaps!
  m_camera.target = {std::round(m_player.getPosition().x),
                     std::round(m_player.getPosition().y)};
  m_camera.zoom = m_cameraZoom * scale;
  m_camera.offset =
      Vector2{(float)GetScreenWidth() / 2.0f, (float)GetScreenHeight() / 2.0f};

  // --- Popups Logic ---
  if (m_player.getAreaState() == AreaState::CORRIDOR &&
      !m_hasSeenCorridorPopup) {
    m_hasSeenCorridorPopup = true;
    m_corridorPopupTimer = 4.0f; // Show for 4 seconds
  }

  if (m_corridorPopupTimer > 0.0f) {
    m_corridorPopupTimer -= GetFrameTime();
  }

  if (m_maze.getRadiationLevel(playerGridX, playerGridY) > 0 &&
      !m_hasSeenRadiationPopup) {
    m_hasSeenRadiationPopup = true;
    m_radiationPopupTimer = 3.0f; // Show for 3 seconds
  }

  if (m_radiationPopupTimer > 0.0f) {
    m_radiationPopupTimer -= GetFrameTime();
  }

  // --- Radiation Flicker Logic ---
  if (m_maze.getRadiationLevel(playerGridX, playerGridY) > 0) {
    m_radiationFlickerTimer += GetFrameTime();
    if (m_radiationFlickerTimer > m_radiationNextFlickerTime) {
      float flickerDuration = 2.0f;
      float t = m_radiationFlickerTimer - m_radiationNextFlickerTime;
      if (t < flickerDuration) {
        float envelope = sinf((t / flickerDuration) * PI);
        float jitter = ((float)GetRandomValue(0, 100) / 100.0f) * 0.4f;
        m_radiationDarknessAlpha = envelope * 0.7f + jitter * envelope;
        if (m_radiationDarknessAlpha > 0.98f)
          m_radiationDarknessAlpha = 0.98f;
        if (m_radiationDarknessAlpha < 0.0f)
          m_radiationDarknessAlpha = 0.0f;
      } else {
        m_radiationDarknessAlpha = 0.0f;
        m_radiationFlickerTimer = 0.0f;
        m_radiationNextFlickerTime = (float)GetRandomValue(350, 550) / 100.0f;
      }
    } else {
      m_radiationDarknessAlpha = 0.0f;
    }
  } else {
    // Fade out quickly if not radiated
    if (m_radiationDarknessAlpha > 0.0f) {
      m_radiationDarknessAlpha -= GetFrameTime() * 2.0f;
      if (m_radiationDarknessAlpha < 0.0f)
        m_radiationDarknessAlpha = 0.0f;
    }
    m_radiationFlickerTimer = 0.0f;
    m_radiationNextFlickerTime = (float)GetRandomValue(350, 550) / 100.0f;
  }
}

void Application::render() {
  BeginDrawing();
  ClearBackground(Color{20, 20, 25, 255});

  // --- World Space ---
  BeginMode2D(m_camera);
  m_renderer.render(m_maze, m_camera, m_player.getAreaState(),
                    m_showGenerationZones);
  m_playerRenderer.render(m_player);
  EndMode2D();

  // --- Smooth Darkness Overlay (Screen Space) ---
  if (m_flashlightEnabled) {
    m_renderer.renderDarknessOverlay(m_player.getPosition(), m_camera,
                                     m_player.getAreaState(),
                                     m_player.getFacingDirection());
  }

  // --- Radiation Darkness Overlay ---
  if (m_radiationDarknessAlpha > 0.0f) {
    DrawRectangle(0, 0, GetScreenWidth(), GetScreenHeight(),
                  Fade(BLACK, m_radiationDarknessAlpha));
  }

  // --- Screen Space (UI Popups) ---
  float scale = std::min((float)GetScreenWidth() / m_screenWidth,
                         (float)GetScreenHeight() / m_screenHeight);
  int screenW = GetScreenWidth();
  int screenH = GetScreenHeight();

  int doorCount = m_player.getAvailableDoors(m_maze);
  if (doorCount == 1) {
    const char *msg = "Press 'K' to use door";
    int textWidth = MeasureText(msg, 30 * scale);
    DrawText(msg, (screenW - textWidth) / 2, screenH - (100 * scale),
             30 * scale, WHITE);
  } else if (doorCount >= 2) {
    const char *msg = "Press 'K' for door 1, press 'L' for door 2";
    int textWidth = MeasureText(msg, 30 * scale);
    DrawText(msg, (screenW - textWidth) / 2, screenH - (100 * scale),
             30 * scale, WHITE);
  }

  // Draw one-time corridor popup
  if (m_corridorPopupTimer > 0.0f) {
    const char *msg = "Dam it's dark in the corridor";
    int textWidth = MeasureText(msg, 30 * scale);
    int x = (screenW - textWidth) / 2;
    int y = screenH - (150 * scale);

    DrawRectangle(x - (15 * scale), y - (5 * scale), textWidth + (30 * scale),
                  40 * scale, Fade(BLACK, 0.6f));
    DrawText(msg, x, y, 30 * scale, Fade(WHITE, std::min(1.0f, m_corridorPopupTimer)));
  }

  // Draw pickup popup
  if (m_popupTimer > 0.0f) {
    int textWidth = MeasureText(m_popupText.c_str(), 30 * scale);
    int x = (screenW - textWidth) / 2;
    int y = screenH - (150 * scale); // Display slightly above hotbar/bottom
    DrawText(m_popupText.c_str(), x, y, 30 * scale, Fade(WHITE, std::min(1.0f, m_popupTimer)));
  }

  // Draw radiation zone popup
  if (m_radiationPopupTimer > 0.0f) {
    const char *msg = "Radiation!";
    int textWidth = MeasureText(msg, 60 * scale);
    int x = (screenW - textWidth) / 2;
    int y = screenH / 8; // Closer to the top of the screen

    float alpha = 1.0f;
    if (m_radiationPopupTimer < 1.0f) {
      alpha = m_radiationPopupTimer; // Fade out over the last second
    }

    // Shadow
    DrawText(msg, x + 2 * scale, y + 2 * scale, 60 * scale,
             Fade(BLACK, alpha * 0.7f));
    // Text
    DrawText(msg, x, y, 60 * scale, Fade(GREEN, alpha));
  }

  // Draw radiation barrel interact popup
  int px = static_cast<int>(
      std::floor(m_player.getPosition().x / m_maze.getCellSize()));
  int py = static_cast<int>(
      std::floor(m_player.getPosition().y / m_maze.getCellSize()));
  if (m_maze.isBarrelNear(px, py, 1)) {
    const char *msg = "Press 'O' to destroy radiation barrel";
    int textWidth = MeasureText(msg, 30 * scale);
    int x = (screenW - textWidth) / 2;
    int y = screenH - (200 * scale);

    DrawRectangle(x - (15 * scale), y - (5 * scale), textWidth + (30 * scale),
                  40 * scale, Fade(BLACK, 0.6f));
    DrawText(msg, x, y, 30 * scale, GREEN);
  }

  // 3. Draw ImGui with matching scale
  rlImGuiBegin();
  ImGui::GetIO().FontGlobalScale = scale;
  renderUI();
  rlImGuiEnd();

  EndDrawing();
}

// TODO: Extract to a separate UIManager class later
void Application::renderUI() {
  ImGui::Begin("Debug Engine");
  ImGui::Text("FPS: %d", GetFPS());
  ImGui::Separator();
  ImGui::Text("Maze Statistics");
  ImGui::Text("Random Seed: %u", m_seed);
  ImGui::Text("Number of Rooms: %zu", m_maze.getRooms().size());

  int totalCells = m_maze.getWidth() * m_maze.getHeight();
  float coveragePercent =
      ((float)m_maze.getNonWallCount() / totalCells) * 100.0f;
  float corridorPercent =
      ((float)m_maze.getCorridorCount() / totalCells) * 100.0f;

  ImGui::Text("Maze Coverage: %.1f%%", coveragePercent);
  ImGui::Text("Corridor Coverage: %.1f%%", corridorPercent);

  ImGui::Separator();
  ImGui::Checkbox("Flashlight Torch Mode", &m_flashlightEnabled);
  if (ImGui::Checkbox("Show Regeneration Zones", &m_showGenerationZones)) {
    m_minimapDirty = true;
  }
  if (ImGui::Checkbox("Show Radiation Zones", &m_showRadiationOnMinimap)) {
    m_minimapDirty = true;
  }

  ImGui::Separator();
  ImGui::Text("View Settings");
  if (ImGui::Button(IsWindowFullscreen() ? "Exit Fullscreen (F11)"
                                         : "Enter Fullscreen (F11)")) {
    ToggleFullscreen();
  }
  ImGui::SliderFloat("Tile Zoom", &m_cameraZoom, 0.5f, 5.0f);

  if (ImGui::Button("Regenerate Tic-Tac-Toe Zones")) {
    regenerateTicTacToeZones();
  }

  ImGui::Separator();
  ImGui::Text("Flashlight Tweaks");
  bool lightSettingsChanged = false;
  if (ImGui::SliderFloat("Degree Cut", &m_lightConeAngle, 90.0f, 360.0f))
    lightSettingsChanged = true;
  if (ImGui::SliderFloat("Circle Size", &m_lightSizeScale, 1.0f, 6.0f))
    lightSettingsChanged = true;
  if (ImGui::SliderFloat("Angular Fade Strength", &m_lightFadeStrength, 0.1f,
                         10.0f))
    lightSettingsChanged = true;

  if (lightSettingsChanged) {
    m_renderer.updateLightSettings(m_lightConeAngle, m_lightFadeStrength,
                                   m_lightSizeScale);
  }

  ImGui::Separator();
  ImGui::Text("Minimap");

  float availWidth = ImGui::GetContentRegionAvail().x;
  float mapRatio = (float)m_maze.getHeight() / (float)m_maze.getWidth();
  Vector2 mapDisplaySize = {availWidth, availWidth * mapRatio};

  ImVec2 mapScreenPos = ImGui::GetCursorScreenPos();

  rlImGuiImageRect(&m_minimapTexture.texture, (int)mapDisplaySize.x,
                   (int)mapDisplaySize.y,
                   Rectangle{0, 0, (float)m_minimapTexture.texture.width,
                             -(float)m_minimapTexture.texture.height});

  // Draw Player Dot on Minimap
  Vector2 pPos = m_player.getPosition();
  int gridX = (int)std::floor(pPos.x / m_maze.getCellSize());
  int gridY = (int)std::floor(pPos.y / m_maze.getCellSize());

  int wrappedX =
      (gridX % m_maze.getWidth() + m_maze.getWidth()) % m_maze.getWidth();
  int wrappedY =
      (gridY % m_maze.getHeight() + m_maze.getHeight()) % m_maze.getHeight();

  float percentX = (float)wrappedX / m_maze.getWidth();
  float percentY = (float)wrappedY / m_maze.getHeight();

  ImVec2 playerScreenPos =
      ImVec2(mapScreenPos.x + (percentX * mapDisplaySize.x),
             mapScreenPos.y + (percentY * mapDisplaySize.y));

  ImGui::GetWindowDrawList()->AddCircleFilled(playerScreenPos, 3.0f,
                                              IM_COL32(255, 0, 0, 255));
  ImGui::End();
}

void Application::generateMinimap() {
  BeginTextureMode(m_minimapTexture);
  ClearBackground(BLANK);
  for (int y = 0; y < m_maze.getHeight(); ++y) {
    for (int x = 0; x < m_maze.getWidth(); ++x) {
      if (m_maze.getCell(x, y) == Maze::CELL_WALL) {
        DrawPixel(x, y, Color{100, 100, 100, 255});
      } else {
        if (m_showRadiationOnMinimap && m_maze.getRadiationLevel(x, y) > 0) {
          DrawPixel(x, y, Color{0, 255, 0, 255});
        } else if (m_showGenerationZones && m_maze.isShiftingZone(x, y)) {
          DrawPixel(x, y, Color{255, 100, 100, 255});
        } else {
          DrawPixel(x, y, Color{30, 30, 35, 255});
        }
      }
    }
  }

  // Draw barrels on top of the terrain so they are highly visible
  if (m_showRadiationOnMinimap) {
    for (int y = 0; y < m_maze.getHeight(); ++y) {
      for (int x = 0; x < m_maze.getWidth(); ++x) {
        if (m_maze.getItem(x, y) == ItemType::BARREL) {
          DrawRectangle(x - 1, y - 1, 3, 3, BLUE);
        }
      }
    }
  }

  EndTextureMode();
}

void Application::regenerateTicTacToeZones() {
  std::vector<Maze::Room> zones = {
      {55, 0, 14, 150},   // V-Left
      {180, 0, 14, 150},  // V-Right
      {0, 30, 55, 14},    // H-Top-Left
      {69, 30, 111, 14},  // H-Top-Mid
      {194, 30, 56, 14},  // H-Top-Right
      {0, 105, 55, 14},   // H-Bot-Left
      {69, 105, 111, 14}, // H-Bot-Mid
      {194, 105, 56, 14}  // H-Bot-Right
  };

  // Phase 3 interaction: Remove items BEFORE erasing zones.
  // Track what was destroyed in each zone so we can respawn replacements later.
  std::vector<std::map<ItemType, int>> removedPerZone(zones.size());
  for (size_t i = 0; i < zones.size(); ++i) {
    const auto &z = zones[i];
    removedPerZone[i] = m_maze.clearItemsInZone(z.x, z.y, z.width, z.height);
  }

  m_maze.clearShiftingZones();
  for (const auto &z : zones) {
    m_maze.addShiftingZone(z.x, z.y, z.width, z.height);
    m_maze.eraseZone(z.x, z.y, z.width, z.height);
  }

  std::mt19937 regenRng(time(0));
  BSPGenerator bsp;
  PrimsGenerator prims;
  LoopGenerator loops;

  for (const auto &z : zones) {
    bsp.generateZone(m_maze, regenRng, z.x, z.y, z.width, z.height);
  }
  for (const auto &z : zones) {
    prims.generateZone(m_maze, regenRng, z.x, z.y, z.width, z.height);
  }
  for (const auto &z : zones) {
    loops.generateZone(m_maze, regenRng, z.x, z.y, z.width, z.height);
  }

  TunnelBorer borer;
  borer.ensureConnectivity(m_maze);
  prims.pruneSmallAlcoves(m_maze, 5);

  // Phase 3 interaction: Respawn items in the freshly carved rooms.
  for (size_t i = 0; i < zones.size(); ++i) {
    const auto &z = zones[i];
    m_itemSpawner.respawnItems(m_maze, removedPerZone[i],
                               z.x, z.y, z.width, z.height);
  }

  // Recalculate all radiation BFS zones with the updated item set
  m_maze.calculateRadiationZones();

  m_minimapDirty = true;
}
