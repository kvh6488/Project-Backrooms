#pragma once

#include "raylib.h"
#include "world/maze.hpp"
#include "entities/player.hpp"
#include "world/item_renderer.hpp"
#include <string>
#include <vector>

enum class PopupType {
    BOXED_BOTTOM,  // Black box, white text (e.g., "Press K to use door", "Dam it's dark")
    SUBTLE_BOTTOM, // Plain white text slightly above hotbar (e.g., "Cannot place there!")
    HEADER_GREEN,  // Large text at top, green with shadow (e.g., "Radiation!")
    HEADER_RAINBOW // Large text at top, color cycling (e.g., "Magic mushroom consumed!")
};

struct ActivePopup {
    std::string text;
    PopupType type;
    float timer;
    float maxDuration;
};

class UIManager {
public:
    UIManager(int screenWidth, int screenHeight);
    ~UIManager();

    // Setup and input handling
    void update(float dt);
    
    // Renders the popups, inventory, and ImGui debug overlay
    // Requires references to current game state data
    void render(Player& player, Maze& maze, ItemRenderer& itemRenderer, bool isDroppingItem, float totalTime);

    // Standardized popup system
    void showPopup(const std::string& text, PopupType type, float duration = 3.0f);

    // --- State Toggles ---
    bool isInventoryOpen() const { return m_inventoryOpen; }
    void toggleInventory() { m_inventoryOpen = !m_inventoryOpen; }
    
    int getActiveHotbarSlot() const { return m_activeHotbarSlot; }
    void setActiveHotbarSlot(int slot) { m_activeHotbarSlot = slot; }

    int getHeldSlotIndex() const { return m_heldSlotIndex; }
    void setHeldSlotIndex(int index) { m_heldSlotIndex = index; }

    // --- Debug Flags (Used by PlayingState) ---
    bool isFlashlightEnabled() const { return m_flashlightEnabled; }
    bool showGenerationZones() const { return m_showGenerationZones; }
    
    // Flashlight config
    float getLightConeAngle() const { return m_lightConeAngle; }
    float getLightFadeStrength() const { return m_lightFadeStrength; }
    float getLightSizeScale() const { return m_lightSizeScale; }
    bool hasLightSettingsChanged() const { return m_lightSettingsChanged; }
    void clearLightSettingsChanged() { m_lightSettingsChanged = false; }
    
    float getCameraZoom() const { return m_cameraZoom; }
    
    bool triggerTicTacToeRegen() const { return m_triggerTicTacToeRegen; }
    void clearTicTacToeRegen() { m_triggerTicTacToeRegen = false; }

    void markMinimapDirty() { m_minimapDirty = true; }

private:
    void renderInventory(Player& player, ItemRenderer& itemRenderer, float scale, int screenW, int screenH);
    void renderPopups(float scale, int screenW, int screenH, float totalTime);
    void renderDebugUI(Player& player, Maze& maze, float scale);
    void generateMinimap(Maze& maze);

    // Screen dimensions
    int m_screenWidth;
    int m_screenHeight;

    // Popups
    std::vector<ActivePopup> m_activePopups;

    // Inventory State
    bool m_inventoryOpen = false;
    int m_heldSlotIndex = -1; // -1 if no item is held
    int m_activeHotbarSlot = 0;

    // Minimap
    RenderTexture2D m_minimapTexture;
    bool m_minimapDirty = true;
    bool m_showRadiationOnMinimap = false;

    // Debug / Toggles
    bool m_flashlightEnabled = true;
    bool m_showGenerationZones = false;
    bool m_triggerTicTacToeRegen = false;

    // Flashlight Overlay Settings
    float m_lightConeAngle = 235.0f;
    float m_lightFadeStrength = 1.5f;
    float m_lightSizeScale = 3.5f;
    bool m_lightSettingsChanged = false;

    // View Settings
    float m_cameraZoom = 1.2f;
};
