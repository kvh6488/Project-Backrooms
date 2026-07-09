#include "states/playing_state.hpp"
#include "world/generators/bsp_generator.hpp"
#include "world/generators/loop_generator.hpp"
#include "world/generators/prims_generator.hpp"
#include "world/generators/tunnel_borer.hpp"
#include <ctime>
#include <cmath>
#include <iostream>
#include <algorithm>

PlayingState::PlayingState(UIManager& uiManager)
    : m_uiManager(uiManager), m_seed(std::time(nullptr)), m_rng(m_seed),
      m_maze(250, 150, 32, m_seed),
      m_player(Vector2{0, 0}, AreaState::ROOM), m_itemSpawner(m_rng),
      m_totalTime(0.0f) {
}

PlayingState::~PlayingState() {
}

void PlayingState::onEnter() {
    // 1. Load Textures
    m_renderer.loadTextures();
    m_itemRenderer.loadTextures();
    m_playerRenderer.loadTextures();

    // 2. Generate Initial Maze
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

    prims.pruneSmallAlcoves(m_maze, 5);
    m_itemSpawner.spawnInitialItems(m_maze);

    // 3. Initialize Player Position
    Vector2 playerStartPos = {0.0f, 0.0f};
    if (!m_maze.getRooms().empty() && midRoomIdx >= 0) {
        const auto &closestRoom = m_maze.getRooms()[midRoomIdx];
        playerStartPos.x = (closestRoom.x + closestRoom.width / 2.0f) * m_maze.getCellSize();
        playerStartPos.y = (closestRoom.y + closestRoom.height / 2.0f) * m_maze.getCellSize();
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

    m_player.update(m_maze, !m_uiManager.isInventoryOpen());

    int playerGridX = static_cast<int>(std::floor(m_player.getPosition().x / m_maze.getCellSize()));
    int playerGridY = static_cast<int>(std::floor(m_player.getPosition().y / m_maze.getCellSize()));

    m_maze.updateVisibility(playerGridX, playerGridY, m_player.getAreaState());
    m_playerRenderer.update(dt, m_player);

    float scale = std::min((float)GetScreenWidth() / 1280, (float)GetScreenHeight() / 720);
    m_camera.target = {std::round(m_player.getPosition().x), std::round(m_player.getPosition().y)};
    m_camera.zoom = m_uiManager.getCameraZoom() * scale;
    m_camera.offset = Vector2{(float)GetScreenWidth() / 2.0f, (float)GetScreenHeight() / 2.0f};

    // --- Popups Logic (via UIManager) ---
    if (m_player.getAreaState() == AreaState::CORRIDOR && !m_hasSeenCorridorPopup) {
        m_hasSeenCorridorPopup = true;
        m_uiManager.showPopup("Dam it's dark in the corridor", PopupType::BOXED_BOTTOM, 4.0f);
    }

    if (m_maze.getRadiationLevel(playerGridX, playerGridY) > 0 && !m_hasSeenRadiationPopup) {
        m_hasSeenRadiationPopup = true;
        m_uiManager.showPopup("Radiation!", PopupType::HEADER_GREEN, 3.0f);
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
                if (m_radiationDarknessAlpha > 0.98f) m_radiationDarknessAlpha = 0.98f;
                if (m_radiationDarknessAlpha < 0.0f) m_radiationDarknessAlpha = 0.0f;
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
            if (m_radiationDarknessAlpha < 0.0f) m_radiationDarknessAlpha = 0.0f;
        }
        m_radiationFlickerTimer = 0.0f;
        m_radiationNextFlickerTime = (float)GetRandomValue(350, 550) / 100.0f;
    }

    // --- Check Player Events ---
    if (m_player.pollEventMushroomConsumed()) {
        m_uiManager.showPopup("Magic mushroom consumed!", PopupType::HEADER_RAINBOW, 3.0f);
    }
    if (m_player.pollEventMushroomThree()) {
        m_uiManager.showPopup("I hope I don't trip for too long", PopupType::BOXED_BOTTOM, 4.0f);
    }
    if (m_player.pollEventMushroomWeird()) {
        m_uiManager.showPopup("Wow I feel weird", PopupType::BOXED_BOTTOM, 4.0f);
    }
    if (m_player.pollEventMushroomOver()) {
        m_uiManager.showPopup("I'm glad that is over", PopupType::BOXED_BOTTOM, 4.0f);
    }
    if (m_player.pollEventMushroomFirstPickup()) {
        m_uiManager.showPopup("Wonder what these do?", PopupType::BOXED_BOTTOM, 4.0f);
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
        m_uiManager.showPopup("What a trip, where am I?", PopupType::BOXED_BOTTOM, 4.0f);
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
        }
    }

    if (m_uiManager.isInventoryOpen()) {
        int hotbar = m_uiManager.getActiveHotbarSlot();
        if (IsKeyPressed(KEY_RIGHT) && (hotbar % 5 != 4)) hotbar++;
        if (IsKeyPressed(KEY_LEFT) && (hotbar % 5 != 0)) hotbar--;
        if (IsKeyPressed(KEY_DOWN)) {
            if (hotbar >= 5 && hotbar <= 14) hotbar += 5;
            else if (hotbar >= 15 && hotbar <= 19) hotbar -= 15;
        }
        if (IsKeyPressed(KEY_UP)) {
            if (hotbar >= 10 && hotbar <= 19) hotbar -= 5;
            else if (hotbar >= 0 && hotbar <= 4) hotbar += 15;
        }
        m_uiManager.setActiveHotbarSlot(hotbar);
    } else {
        if (IsKeyPressed(KEY_ONE)) m_uiManager.setActiveHotbarSlot(0);
        if (IsKeyPressed(KEY_TWO)) m_uiManager.setActiveHotbarSlot(1);
        if (IsKeyPressed(KEY_THREE)) m_uiManager.setActiveHotbarSlot(2);
        if (IsKeyPressed(KEY_FOUR)) m_uiManager.setActiveHotbarSlot(3);
        if (IsKeyPressed(KEY_FIVE)) m_uiManager.setActiveHotbarSlot(4);
    }

    if (IsKeyPressed(KEY_U)) {
        m_player.consumeItem(m_uiManager.getActiveHotbarSlot());
    }

    if (IsKeyPressed(KEY_Q)) {
        if (m_isDroppingItem) {
            m_isDroppingItem = false;
        } else {
            if (m_player.getInventory()[m_uiManager.getActiveHotbarSlot()].type != ItemType::NONE) {
                m_isDroppingItem = true;
            }
        }
    }

    if (m_isDroppingItem && IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
        Vector2 mouseWorld = GetScreenToWorld2D(GetMousePosition(), m_camera);
        int gridX = static_cast<int>(std::floor(mouseWorld.x / m_maze.getCellSize()));
        int gridY = static_cast<int>(std::floor(mouseWorld.y / m_maze.getCellSize()));

        if (m_maze.getItem(gridX, gridY) == ItemType::NONE) {
            int cellType = m_maze.getCell(gridX, gridY);
            if (cellType == Maze::CELL_ROOM || cellType == Maze::CELL_CORRIDOR) {
                if (m_maze.isVisible(gridX, gridY)) {
                    m_maze.setItem(gridX, gridY, m_player.getInventory()[m_uiManager.getActiveHotbarSlot()].type);
                    m_player.destroyItem(m_uiManager.getActiveHotbarSlot());
                    m_isDroppingItem = false;
                } else {
                    m_uiManager.showPopup("Cannot place there!", PopupType::SUBTLE_BOTTOM, 2.0f);
                }
            }
        }
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
    m_renderer.render(m_maze, m_camera, m_player.getAreaState(), m_uiManager.showGenerationZones());
    m_playerRenderer.render(m_player);
    m_itemRenderer.render(m_maze, m_camera, m_player.getAreaState());
    EndMode2D();

    if (m_uiManager.isFlashlightEnabled() && m_player.getAreaState() == AreaState::CORRIDOR) {
        m_renderer.drawLightMask();
    }

    if (m_radiationDarknessAlpha > 0.0f) {
        DrawRectangle(0, 0, GetScreenWidth(), GetScreenHeight(), Fade(BLACK, m_radiationDarknessAlpha));
    }
    EndTextureMode();

    BeginDrawing();
    ClearBackground(BLACK);

    float tripStrength = m_player.getMushroomEffectStrength();
    if (tripStrength > 0.0f) {
        SetShaderValue(m_tripShader, m_tripTimeLoc, &m_totalTime, SHADER_UNIFORM_FLOAT);
        SetShaderValue(m_tripShader, m_tripStrengthLoc, &tripStrength, SHADER_UNIFORM_FLOAT);
        BeginShaderMode(m_tripShader);
    }

    Rectangle sourceRec = {0.0f, 0.0f, (float)m_screenTarget.texture.width, -(float)m_screenTarget.texture.height};
    Rectangle destRec = {0.0f, 0.0f, (float)GetScreenWidth(), (float)GetScreenHeight()};
    DrawTexturePro(m_screenTarget.texture, sourceRec, destRec, Vector2{0.0f, 0.0f}, 0.0f, WHITE);

    if (tripStrength > 0.0f) {
        EndShaderMode();
    }

    if (m_player.isPassingOut()) {
        float passOutTimer = m_player.getPassOutTimer();
        float fadeAlpha = 0.0f;
        if (passOutTimer > 5.0f) {
            fadeAlpha = 1.0f - ((passOutTimer - 5.0f) / 5.0f);
        } else {
            fadeAlpha = passOutTimer / 5.0f;
        }

        if (fadeAlpha > 1.0f) fadeAlpha = 1.0f;
        if (fadeAlpha < 0.0f) fadeAlpha = 0.0f;
        DrawRectangle(0, 0, GetScreenWidth(), GetScreenHeight(), Fade(BLACK, fadeAlpha));
    }

    // Hand off UI rendering to UIManager
    m_uiManager.render(m_player, m_maze, m_itemRenderer, m_isDroppingItem, m_totalTime);

    EndDrawing();
}

void PlayingState::regenerateTicTacToeZones() {
    std::vector<Maze::Room> zones = {
        {55, 0, 14, 150},
        {180, 0, 14, 150},
        {0, 30, 55, 14},
        {69, 30, 111, 14},
        {194, 30, 56, 14},
        {0, 105, 55, 14},
        {69, 105, 111, 14},
        {194, 105, 56, 14}
    };

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

    for (size_t i = 0; i < zones.size(); ++i) {
        const auto &z = zones[i];
        m_itemSpawner.respawnItems(m_maze, removedPerZone[i], z.x, z.y, z.width, z.height);
    }

    m_maze.calculateRadiationZones();
    m_uiManager.markMinimapDirty();
}
