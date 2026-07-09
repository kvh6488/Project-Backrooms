#pragma once

#include "states/game_state.hpp"
#include "world/maze.hpp"
#include "world/maze_renderer.hpp"
#include "world/item_renderer.hpp"
#include "entities/player.hpp"
#include "entities/player_renderer.hpp"
#include "world/item_spawner.hpp"
#include "core/ui_manager.hpp"
#include "raylib.h"
#include <random>

// The PlayingState represents the core gameplay loop (exploring the maze).
class PlayingState : public GameState {
public:
    PlayingState(UIManager& uiManager);
    ~PlayingState() override;

    void onEnter() override;
    void onExit() override;
    void update(float dt) override;
    void render() override;

private:
    void handleInput();
    void regenerateTicTacToeZones();

    // Reference to the global UI Manager
    UIManager& m_uiManager;

    // --- Core Systems ---
    unsigned int m_seed;
    std::mt19937 m_rng;
    Maze m_maze;
    Player m_player;
    Camera2D m_camera;
    MazeRenderer m_renderer;
    ItemRenderer m_itemRenderer;
    PlayerRenderer m_playerRenderer;
    ItemSpawner m_itemSpawner;

    // --- Visual Effects ---
    Shader m_tripShader;
    int m_tripTimeLoc;
    int m_tripStrengthLoc;
    float m_totalTime;
    RenderTexture2D m_screenTarget;

    // Interaction State
    int m_focusedCupboardX = -1;
    int m_focusedCupboardY = -1;

    // --- Radiation Flicker ---
    float m_radiationFlickerTimer = 0.0f;
    float m_radiationNextFlickerTime = 4.0f;
    float m_radiationDarknessAlpha = 0.0f;

    // --- State flags for Popups ---
    bool m_hasSeenCorridorPopup = false;
    bool m_hasSeenRadiationPopup = false;
    bool m_isDroppingItem = false;
};
