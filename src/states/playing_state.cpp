#include "states/playing_state.hpp"
#include "items/item_database.hpp"
#include "world/generators/bsp_generator.hpp"
#include "world/generators/loop_generator.hpp"
#include "world/generators/prims_generator.hpp"
#include "world/generators/tunnel_borer.hpp"
#include <algorithm>
#include <cmath>
#include "core/debug_log.hpp"
#include <ctime>
#include <iostream>

PlayingState::PlayingState(UIManager &uiManager, unsigned int seed)
    : m_uiManager(uiManager),
      m_seed(seed != 0 ? seed : (unsigned int)std::time(nullptr)), m_rng(m_seed),
      m_maze(250, 150, 32, m_seed), m_player(Vector2{0, 0}, AreaState::ROOM),
      m_itemSpawner(m_rng), m_totalTime(0.0f) {}

PlayingState::~PlayingState() {}

void PlayingState::onEnter() {
  // 1. Load Textures
  m_renderer.loadTextures();
  m_itemRenderer.loadTextures();
  m_playerRenderer.loadTextures();

  // 2. Generate Initial Maze
  debuglog::log("MAZE", "generating %dx%d world from seed %u",
                m_maze.getWidth(), m_maze.getHeight(), m_seed);
  m_uiManager.setSeed(m_seed);

  BSPGenerator bsp;
  bsp.generate(m_maze, m_rng);
  int midRoomIdx = bsp.getMiddleRoomIndex();

  PrimsGenerator prims;
  prims.generate(m_maze, m_rng, midRoomIdx);

  LoopGenerator loops;
  loops.generate(m_maze, m_rng);

  TunnelBorer borer;
  borer.ensureConnectivity(m_maze);

  prims.pruneSmallAlcoves(m_maze, 5);
  m_itemSpawner.spawnInitialItems(m_maze);

  m_uiManager.markDebugMapDirty();

  // 3. Initialize Player Position
  Vector2 playerStartPos = {0.0f, 0.0f};
  if (!m_maze.getRooms().empty() && midRoomIdx >= 0) {
    const auto &closestRoom = m_maze.getRooms()[midRoomIdx];
    playerStartPos.x =
        (closestRoom.x + closestRoom.width / 2.0f) * m_maze.getCellSize();
    playerStartPos.y =
        (closestRoom.y + closestRoom.height / 2.0f) * m_maze.getCellSize();
  }
  m_player = Player(playerStartPos, AreaState::ROOM);

  // 4. Initialize Camera
  m_camera = {0};
  m_camera.target = m_player.getPosition();
  m_camera.offset = Vector2{GetScreenWidth() / 2.0f, GetScreenHeight() / 2.0f};
  m_camera.rotation = 0.0f;
  m_camera.zoom = 1.0f;

  // 5. Load Shaders
  m_tripShader = LoadShader(0, "assets/magic_trip.fs");
  m_tripTimeLoc = GetShaderLocation(m_tripShader, "time");
  m_tripStrengthLoc = GetShaderLocation(m_tripShader, "strength");
  m_screenTarget = LoadRenderTexture(GetScreenWidth(), GetScreenHeight());
}

void PlayingState::onExit() {
  UnloadRenderTexture(m_screenTarget);
  UnloadShader(m_tripShader);
}

void PlayingState::update(float dt) {
  if (IsKeyPressed(KEY_F11)) {
    ToggleFullscreen();
  }

  if (m_uiManager.triggerTicTacToeRegen()) {
    regenerateTicTacToeZones();
    m_uiManager.clearTicTacToeRegen();
  }

  if (m_uiManager.hasLightSettingsChanged()) {
    m_renderer.updateLightSettings(m_uiManager.getLightConeAngle(),
                                   m_uiManager.getLightFadeStrength(),
                                   m_uiManager.getLightSizeScale());
    m_uiManager.clearLightSettingsChanged();
  }

  handleInput();

  m_player.update(m_maze, !m_uiManager.isInventoryOpen() &&
                              !m_uiManager.isFullscreenMapOpen());

  int playerGridX = static_cast<int>(
      std::floor(m_player.getPosition().x / m_maze.getCellSize()));
  int playerGridY = static_cast<int>(
      std::floor(m_player.getPosition().y / m_maze.getCellSize()));

  m_maze.updateVisibility(playerGridX, playerGridY, m_player.getAreaState());
  m_playerRenderer.update(dt, m_player);

  // --- Cupboard Interaction Logic ---
  m_focusedCupboardX = -1;
  m_focusedCupboardY = -1;

  Vector2 lookPos = m_player.getPosition();
  switch (m_player.getFacingDirection()) {
  case FacingDirection::UP:
    lookPos.y -= 20.0f;
    break;
  case FacingDirection::DOWN:
    lookPos.y += 20.0f;
    break;
  case FacingDirection::LEFT:
    lookPos.x -= 20.0f;
    break;
  case FacingDirection::RIGHT:
    lookPos.x += 20.0f;
    break;
  }

  int lookGridX =
      static_cast<int>(std::floor(lookPos.x / m_maze.getCellSize()));
  int lookGridY =
      static_cast<int>(std::floor(lookPos.y / m_maze.getCellSize()));

  if (m_maze.getItem(lookGridX, lookGridY) == ItemType::CUPBOARD) {
    bool wallAbove =
        m_maze.getCell(lookGridX, lookGridY - 1) == Maze::CELL_WALL;
    bool wallRight =
        m_maze.getCell(lookGridX + 1, lookGridY) == Maze::CELL_WALL;
    bool wallLeft = m_maze.getCell(lookGridX - 1, lookGridY) == Maze::CELL_WALL;

    bool canAccess = false;
    if (wallAbove) {
      canAccess = (m_player.getFacingDirection() == FacingDirection::UP);
    } else if (wallRight) {
      canAccess = (m_player.getFacingDirection() == FacingDirection::RIGHT);
    } else if (wallLeft) {
      canAccess = (m_player.getFacingDirection() == FacingDirection::LEFT);
    }

    if (canAccess) {
      m_focusedCupboardX = lookGridX;
      m_focusedCupboardY = lookGridY;
      if (!m_uiManager.isCupboardInventoryOpen()) {
        m_uiManager.showPopup("Press 'o' to search cupboard",
                              PopupType::SUBTLE_BOTTOM, 0.5f);
      }
    }
  }

  if (m_focusedCupboardX == -1 && m_uiManager.isCupboardInventoryOpen()) {
    m_maze.setItemState(m_uiManager.getOpenedCupboardX(),
                        m_uiManager.getOpenedCupboardY(), 0);
    m_uiManager.closeCupboard();
    if (m_uiManager.isInventoryOpen()) {
      m_uiManager.toggleInventory();
    }
  }

  float scale =
      std::min((float)GetScreenWidth() / 1280, (float)GetScreenHeight() / 720);
  m_camera.target = {std::round(m_player.getPosition().x),
                     std::round(m_player.getPosition().y)};
  m_camera.zoom = m_uiManager.getCameraZoom() * scale;
  m_camera.offset =
      Vector2{(float)GetScreenWidth() / 2.0f, (float)GetScreenHeight() / 2.0f};

  // --- Popups Logic (via UIManager) ---
  if (m_player.getAreaState() == AreaState::CORRIDOR &&
      !m_hasSeenCorridorPopup) {
    m_hasSeenCorridorPopup = true;
    m_uiManager.showPopup("Dam it's dark in the corridor",
                          PopupType::BOXED_BOTTOM, 4.0f);
  }

  bool currentlyInRadiation =
      m_maze.getRadiationLevel(playerGridX, playerGridY) > 0;
  if (currentlyInRadiation && !m_inRadiationZone) {
    m_inRadiationZone = true;
    m_uiManager.showPopup("Radiation!", PopupType::HEADER_GREEN, 3.0f);
  } else if (!currentlyInRadiation && m_inRadiationZone) {
    m_inRadiationZone = false;
  }

  // --- Radiation Flicker Logic ---
  if (m_maze.getRadiationLevel(playerGridX, playerGridY) > 0) {
    m_radiationFlickerTimer += dt;
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
    if (m_radiationDarknessAlpha > 0.0f) {
      m_radiationDarknessAlpha -= dt * 2.0f;
      if (m_radiationDarknessAlpha < 0.0f)
        m_radiationDarknessAlpha = 0.0f;
    }
    m_radiationFlickerTimer = 0.0f;
    m_radiationNextFlickerTime = (float)GetRandomValue(350, 550) / 100.0f;
  }

  // --- Check Player Events ---
  if (m_player.pollEventMushroomFullTripStarted()) {
    if (m_player.hasPickedUpMagicBook()) {
      debuglog::log("BOOK", "skipped - already picked up this run");
      m_uiManager.setMagicBookStatus("skipped: book already picked up");
    } else if (GetRandomValue(0, 2) != 0) { // 33% chance
      debuglog::log("BOOK", "skipped - lost the 1-in-3 roll");
      m_uiManager.setMagicBookStatus("skipped: lost the 1-in-3 roll");
    } else {
      attemptMagicBookSpawn();
    }
  }

  // Debug: force a spawn from the ImGui panel, bypassing the whole ritual.
  if (m_uiManager.triggerMagicBookSpawn()) {
    m_uiManager.clearMagicBookSpawn();
    attemptMagicBookSpawn();
  }

  // --- Debug trip controls ---
  if (m_uiManager.triggerForceTrip()) {
    m_uiManager.clearForceTrip();
    m_player.debugForceTrip(60.0f);
    debuglog::log("TRIP", "forced full trip for 60s");
  }
  if (m_uiManager.triggerEndTrip()) {
    m_uiManager.clearEndTrip();
    m_player.debugEndTrip();
    debuglog::log("TRIP", "trip ended by debug panel");
  }

  if (m_maze.isMagicBookSpawned() && !m_uiManager.isMagicBookPinned() &&
      m_player.getMushroomEffectStrength() < 1.0f) {
    debuglog::log("BOOK", "despawned - trip strength fell below 1.0");
    m_maze.despawnMagicBook();
    m_uiManager.setMagicBookStatus("despawned: trip strength fell below 1.0");
    m_uiManager.markDebugMapDirty();
  }

  if (m_player.pollEventMapCrafted()) {
    int gridX = static_cast<int>(
        std::floor(m_player.getPosition().x / m_maze.getCellSize()));
    int gridY = static_cast<int>(
        std::floor(m_player.getPosition().y / m_maze.getCellSize()));
    m_uiManager.markMapDrawn(m_player.getLastConsumedMapId(), m_maze, gridX,
                             gridY);
  }

  if (m_player.pollEventMapOpened()) {
    m_uiManager.openFullscreenMap(m_player.getLastConsumedMapId());
    if (m_uiManager.isInventoryOpen())
      m_uiManager.toggleInventory();
  }

  if (m_player.pollEventMagicBookOpened()) {
    m_uiManager.openFullscreenMap(-1);
    if (m_uiManager.isInventoryOpen())
      m_uiManager.toggleInventory();
  }

  if (m_player.pollEventMushroomConsumed()) {
    m_uiManager.showPopup("Magic mushroom consumed!", PopupType::HEADER_RAINBOW,
                          3.0f);
  }
  if (m_player.pollEventMushroomThree()) {
    m_uiManager.showPopup("I hope I don't trip for too long",
                          PopupType::BOXED_BOTTOM, 4.0f);
  }
  if (m_player.pollEventMushroomWeird()) {
    m_uiManager.showPopup("Wow I feel weird", PopupType::BOXED_BOTTOM, 4.0f);
  }
  if (m_player.pollEventMushroomOver()) {
    m_uiManager.showPopup("I'm glad that is over", PopupType::BOXED_BOTTOM,
                          4.0f);
  }
  if (m_player.pollEventMushroomFirstPickup()) {
    m_uiManager.showPopup("Wonder what these do?", PopupType::BOXED_BOTTOM,
                          4.0f);
  }
  if (m_player.pollEventPassOutComplete()) {
    const auto &rooms = m_maze.getRooms();
    if (!rooms.empty()) {
      int randomIdx = GetRandomValue(0, rooms.size() - 1);
      const auto &room = rooms[randomIdx];
      Vector2 newPos = {(room.x + room.width / 2.0f) * m_maze.getCellSize(),
                        (room.y + room.height / 2.0f) * m_maze.getCellSize()};
      m_player.teleport(newPos, AreaState::ROOM);
    }
    m_uiManager.showPopup("What a trip, where am I?", PopupType::BOXED_BOTTOM,
                          4.0f);
  }

  m_totalTime += dt;
  m_uiManager.update(dt);
}

void PlayingState::handleInput() {
  if (IsKeyPressed(KEY_I)) {
    m_uiManager.toggleInventory();
    if (!m_uiManager.isInventoryOpen()) {
      m_uiManager.setHeldSlotIndex(-1);
      if (m_uiManager.getActiveHotbarSlot() > 4) {
        m_uiManager.setActiveHotbarSlot(m_uiManager.getActiveHotbarSlot() % 5);
      }
      if (m_uiManager.isCupboardInventoryOpen()) {
        m_maze.setItemState(m_uiManager.getOpenedCupboardX(),
                            m_uiManager.getOpenedCupboardY(), 0);
        m_uiManager.closeCupboard();
      }
    }
  }

  if (IsKeyPressed(KEY_O) && m_focusedCupboardX != -1) {
    if (m_uiManager.isCupboardInventoryOpen()) {
      m_maze.setItemState(m_focusedCupboardX, m_focusedCupboardY, 0);
      m_uiManager.closeCupboard();
      if (m_uiManager.isInventoryOpen()) {
        m_uiManager.toggleInventory();
      }
    } else {
      m_maze.setItemState(m_focusedCupboardX, m_focusedCupboardY, 1);
      m_uiManager.openCupboard(m_focusedCupboardX, m_focusedCupboardY);
      if (!m_uiManager.isInventoryOpen()) {
        m_uiManager.toggleInventory();
      }
    }
  }

  if (m_uiManager.isInventoryOpen()) {
    int hotbar = m_uiManager.getActiveHotbarSlot();
    if (IsKeyPressed(KEY_RIGHT) && (hotbar % 5 != 4))
      hotbar++;
    if (IsKeyPressed(KEY_LEFT) && (hotbar % 5 != 0))
      hotbar--;
    if (IsKeyPressed(KEY_DOWN)) {
      if (hotbar >= 5 && hotbar <= 14)
        hotbar += 5;
      else if (hotbar >= 15 && hotbar <= 19)
        hotbar -= 15;
    }
    if (IsKeyPressed(KEY_UP)) {
      if (hotbar >= 10 && hotbar <= 19)
        hotbar -= 5;
      else if (hotbar >= 0 && hotbar <= 4)
        hotbar += 15;
    }
    m_uiManager.setActiveHotbarSlot(hotbar);
  } else {
    if (IsKeyPressed(KEY_ONE))
      m_uiManager.setActiveHotbarSlot(0);
    if (IsKeyPressed(KEY_TWO))
      m_uiManager.setActiveHotbarSlot(1);
    if (IsKeyPressed(KEY_THREE))
      m_uiManager.setActiveHotbarSlot(2);
    if (IsKeyPressed(KEY_FOUR))
      m_uiManager.setActiveHotbarSlot(3);
    if (IsKeyPressed(KEY_FIVE))
      m_uiManager.setActiveHotbarSlot(4);
  }

  if (IsKeyPressed(KEY_U)) {
    if (m_uiManager.isFullscreenMapOpen()) {
      m_uiManager.closeFullscreenMap();
    } else {
      m_player.consumeItem(m_uiManager.getActiveHotbarSlot());
    }
  }

  if (IsKeyPressed(KEY_Q) && !m_uiManager.isInventoryOpen()) {
    if (m_isDroppingItem) {
      m_isDroppingItem = false;
    } else {
      ItemType activeType =
          m_player.getInventory()[m_uiManager.getActiveHotbarSlot()].type;
      if (activeType != ItemType::NONE) {
        if (ItemDatabase::getDef(activeType).isPlaceable) {
          m_isDroppingItem = true;
        } else {
          m_uiManager.showPopup("Item cannot be placed",
                                PopupType::SUBTLE_BOTTOM, 2.0f);
        }
      }
    }
  }

  if (m_isDroppingItem && IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
    Vector2 mouseWorld = GetScreenToWorld2D(GetMousePosition(), m_camera);
    int gridX =
        static_cast<int>(std::floor(mouseWorld.x / m_maze.getCellSize()));
    int gridY =
        static_cast<int>(std::floor(mouseWorld.y / m_maze.getCellSize()));

    if (m_maze.getItem(gridX, gridY) == ItemType::NONE) {
      int cellType = m_maze.getCell(gridX, gridY);
      if (cellType == Maze::CELL_ROOM || cellType == Maze::CELL_CORRIDOR) {
        if (m_maze.isVisible(gridX, gridY)) {
          m_maze.setItem(
              gridX, gridY,
              m_player.getInventory()[m_uiManager.getActiveHotbarSlot()].type);
          m_player.destroyItem(m_uiManager.getActiveHotbarSlot());
          m_isDroppingItem = false;
        } else {
          m_uiManager.showPopup("Cannot place there!", PopupType::SUBTLE_BOTTOM,
                                2.0f);
        }
      }
    }
  }
}

// ============================================================================
// computeTripFollowOffset - Make the book ride the shader's distortion
// ============================================================================
// The book is drawn after EndShaderMode() so it never warps. The table under
// it IS warped, so with no correction the book is the only stationary object
// on a swimming screen - which the eye reads as the book floating in circles.
//
// magic_trip.fs does an INVERSE uv lookup: the pixel at uv displays whatever
// lives at (uv + wave). So scene content at uv appears shifted by -wave. The
// blit uses a negative source height, flipping V, which cancels the sign on
// the vertical axis - hence -waveX but +waveY below.
//
// Evaluating the wave at the content's own uv rather than the screen uv is a
// first-order approximation, exact enough here: peak displacement is ~3% of
// the screen, well inside the range where the two agree.
// ============================================================================
Vector2 PlayingState::computeTripFollowOffset() const {
  float strength = m_player.getMushroomEffectStrength();
  if (strength <= 0.0f || !m_maze.isMagicBookSpawned()) {
    return {0.0f, 0.0f};
  }

  int cellSize = m_maze.getCellSize();
  Vector2 bookWorld = {(m_maze.getMagicBookX() + 0.5f) * cellSize,
                       (m_maze.getMagicBookY() + 0.5f) * cellSize};
  Vector2 screen = GetWorldToScreen2D(bookWorld, m_camera);

  float w = (float)GetScreenWidth();
  float h = (float)GetScreenHeight();
  if (w <= 0.0f || h <= 0.0f) {
    return {0.0f, 0.0f};
  }

  float u = screen.x / w;
  float v = 1.0f - (screen.y / h); // V is flipped by the render-texture blit
  float t = m_totalTime;

  // These four terms must mirror assets/magic_trip.fs exactly.
  float waveX = sinf(v * 10.0f + t * 2.5f) * 0.01f * strength +
                sinf(u * 3.0f - t * 1.5f) * 0.02f * strength;
  float waveY = cosf(u * 12.0f + t * 2.0f) * 0.015f * strength +
                cosf(v * 4.0f + t * 1.2f) * 0.02f * strength;

  float follow = m_uiManager.getBookTripFollow();
  float zoom = m_camera.zoom != 0.0f ? m_camera.zoom : 1.0f;

  // UV -> screen pixels -> world units (BeginMode2D scales by zoom).
  return {(-waveX * w * follow) / zoom, (waveY * h * follow) / zoom};
}

// ============================================================================
// attemptMagicBookSpawn - Single entry point for placing the magic book
// ============================================================================
// Both the mushroom-trip path and the debug button funnel through here so the
// outcome is reported identically. The status string is the diagnostic that
// tells a blank screen apart from a failed placement.
void PlayingState::attemptMagicBookSpawn() {
  int gridX = static_cast<int>(
      std::floor(m_player.getPosition().x / m_maze.getCellSize()));
  int gridY = static_cast<int>(
      std::floor(m_player.getPosition().y / m_maze.getCellSize()));

  ItemSpawner::BookSpawnResult result =
      m_itemSpawner.spawnMagicBookOfMaps(m_maze, gridX, gridY);

  if (result == ItemSpawner::BookSpawnResult::SPAWNED) {
    debuglog::log("BOOK", "SPAWNED at (%d, %d), player at (%d, %d)",
                  m_maze.getMagicBookX(), m_maze.getMagicBookY(), gridX, gridY);
    m_uiManager.setMagicBookStatus(
        TextFormat("SPAWNED at (%d, %d), player at (%d, %d)",
                   m_maze.getMagicBookX(), m_maze.getMagicBookY(), gridX,
                   gridY));
    // The debug map draws a purple marker at the book's cell - that marker is
    // the bisector: visible on the map but absent on screen means the bug is
    // in the renderer, not the spawner.
    m_uiManager.markDebugMapDirty();
  } else {
    debuglog::log("BOOK", "FAILED - no table within 20 tiles of (%d, %d)",
                  gridX, gridY);
    m_uiManager.setMagicBookStatus(TextFormat(
        "FAILED: no table within 20 tiles of (%d, %d)", gridX, gridY));
  }
}

void PlayingState::render() {
  if (m_screenTarget.texture.width != GetScreenWidth() ||
      m_screenTarget.texture.height != GetScreenHeight()) {
    UnloadRenderTexture(m_screenTarget);
    m_screenTarget = LoadRenderTexture(GetScreenWidth(), GetScreenHeight());
  }

  if (m_uiManager.isFlashlightEnabled()) {
    m_renderer.buildLightMask(m_player.getPosition(), m_camera,
                              m_player.getAreaState(),
                              m_player.getFacingDirection());
  }

  BeginTextureMode(m_screenTarget);
  ClearBackground(Color{20, 20, 25, 255});

  BeginMode2D(m_camera);
  m_renderer.render(m_maze, m_camera, m_player.getAreaState(),
                    m_uiManager.showGenerationZones());
  m_playerRenderer.render(m_player);
  m_itemRenderer.render(m_maze, m_camera, m_player.getAreaState());
  EndMode2D();

  if (m_uiManager.isFlashlightEnabled() &&
      m_player.getAreaState() == AreaState::CORRIDOR) {
    m_renderer.drawLightMask();
  }

  if (m_radiationDarknessAlpha > 0.0f) {
    DrawRectangle(0, 0, GetScreenWidth(), GetScreenHeight(),
                  Fade(BLACK, m_radiationDarknessAlpha));
  }
  EndTextureMode();

  BeginDrawing();
  ClearBackground(BLACK);

  float tripStrength = m_player.getMushroomEffectStrength();
  if (tripStrength > 0.0f) {
    SetShaderValue(m_tripShader, m_tripTimeLoc, &m_totalTime,
                   SHADER_UNIFORM_FLOAT);
    SetShaderValue(m_tripShader, m_tripStrengthLoc, &tripStrength,
                   SHADER_UNIFORM_FLOAT);
    BeginShaderMode(m_tripShader);
  }

  Rectangle sourceRec = {0.0f, 0.0f, (float)m_screenTarget.texture.width,
                         -(float)m_screenTarget.texture.height};
  Rectangle destRec = {0.0f, 0.0f, (float)GetScreenWidth(),
                       (float)GetScreenHeight()};
  DrawTexturePro(m_screenTarget.texture, sourceRec, destRec,
                 Vector2{0.0f, 0.0f}, 0.0f, WHITE);

  if (tripStrength > 0.0f) {
    EndShaderMode();
  }

  // Overlay layer (exempt from main shader)
  BeginMode2D(m_camera);
  m_itemRenderer.renderMagicBookOverlay(m_maze, m_camera,
                                        m_player.getAreaState(),
                                        computeTripFollowOffset(),
                                        m_uiManager.getBookGlowScale());
  EndMode2D();

  if (m_player.isPassingOut()) {
    float passOutTimer = m_player.getPassOutTimer();
    float fadeAlpha = 0.0f;
    if (passOutTimer > 5.0f) {
      fadeAlpha = 1.0f - ((passOutTimer - 5.0f) / 5.0f);
    } else {
      fadeAlpha = passOutTimer / 5.0f;
    }

    if (fadeAlpha > 1.0f)
      fadeAlpha = 1.0f;
    if (fadeAlpha < 0.0f)
      fadeAlpha = 0.0f;
    DrawRectangle(0, 0, GetScreenWidth(), GetScreenHeight(),
                  Fade(BLACK, fadeAlpha));
  }

  // Hand off UI rendering to UIManager
  m_uiManager.render(m_player, m_maze, m_itemRenderer, m_isDroppingItem,
                     m_totalTime);

  EndDrawing();
}

void PlayingState::regenerateTicTacToeZones() {
  std::vector<Maze::Room> zones = {{55, 0, 14, 150},   {180, 0, 14, 150},
                                   {0, 30, 55, 14},    {69, 30, 111, 14},
                                   {194, 30, 56, 14},  {0, 105, 55, 14},
                                   {69, 105, 111, 14}, {194, 105, 56, 14}};

  std::vector<std::map<ItemType, int>> removedPerZone(zones.size());
  for (size_t i = 0; i < zones.size(); ++i) {
    const auto &z = zones[i];
    int startX = std::max(0, z.x - 1);
    int startY = std::max(0, z.y - 1);
    int w = std::min(m_maze.getWidth() - startX, z.width + 2);
    int h = std::min(m_maze.getHeight() - startY, z.height + 2);
    removedPerZone[i] = m_maze.clearItemsInZone(startX, startY, w, h);
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

  for (size_t i = 0; i < zones.size(); ++i) {
    const auto &z = zones[i];
    int startX = std::max(0, z.x - 1);
    int startY = std::max(0, z.y - 1);
    int w = std::min(m_maze.getWidth() - startX, z.width + 2);
    int h = std::min(m_maze.getHeight() - startY, z.height + 2);
    m_itemSpawner.respawnItems(m_maze, removedPerZone[i], startX, startY, w, h);
  }

  m_maze.calculateRadiationZones();
  m_uiManager.markDebugMapDirty();
}
