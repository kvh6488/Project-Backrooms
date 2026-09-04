#pragma once

#include "entities/player.hpp"
#include "items/item_renderer.hpp"
#include "raylib.h"
#include "world/maze.hpp"
#include <string>
#include <vector>
#include <unordered_map>

struct DrawnMapData {
  int id;
  int centerX;
  int centerY;
  RenderTexture2D texture;
};

enum class PopupType {
  BOXED_BOTTOM, // Black box, white text (e.g., "Press K to use door", "Dam it's
                // dark")
  SUBTLE_BOTTOM, // Plain white text slightly above hotbar (e.g., "Cannot place
                 // there!")
  HEADER_GREEN,  // Large text at top, green with shadow (e.g., "Radiation!")
  HEADER_RAINBOW // Large text at top, color cycling (e.g., "Magic mushroom
                 // consumed!")
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

  // Renders the shipping UI: popups, inventory, cupboards and map overlays.
  // The debug panel is DebugOverlay's job and draws after this.
  void render(Player &player, Maze &maze, ItemRenderer &itemRenderer,
              bool isDroppingItem, float totalTime);

  // The scale UIManager lays its own widgets out with. DebugOverlay reuses it
  // so the panel's text tracks the window size the same way the game UI does.
  float getUIScale() const {
    float sx = (float)GetScreenWidth() / m_screenWidth;
    float sy = (float)GetScreenHeight() / m_screenHeight;
    return sx < sy ? sx : sy;
  }

  // Standardized popup system
  void showPopup(const std::string &text, PopupType type,
                 float duration = 3.0f);

  // --- State Toggles ---
  bool isInventoryOpen() const { return m_inventoryOpen; }
  void toggleInventory() { m_inventoryOpen = !m_inventoryOpen; }

  int getActiveHotbarSlot() const { return m_activeHotbarSlot; }
  void setActiveHotbarSlot(int slot) { m_activeHotbarSlot = slot; }

  int getHeldSlotIndex() const { return m_heldSlotIndex; }
  void setHeldSlotIndex(int index) {
    m_heldSlotIndex = index;
    m_heldFromCupboard = false;
  }

  // --- Cupboard State ---
  bool isCupboardInventoryOpen() const { return m_cupboardInventoryOpen; }
  int getOpenedCupboardX() const { return m_openedCupboardX; }
  int getOpenedCupboardY() const { return m_openedCupboardY; }
  void openCupboard(int x, int y) {
    m_cupboardInventoryOpen = true;
    m_openedCupboardX = x;
    m_openedCupboardY = y;
  }
  void closeCupboard() {
    m_cupboardInventoryOpen = false;
    m_openedCupboardX = -1;
    m_openedCupboardY = -1;
  }

  // --- Map Overlay State ---
  void openFullscreenMap(int mapId) {
    m_showFullscreenMap = true;
    m_openedMapId = mapId;
  }
  void closeFullscreenMap() { m_showFullscreenMap = false; }
  bool isFullscreenMapOpen() const { return m_showFullscreenMap; }
  int getOpenedMapId() const { return m_openedMapId; }
  void markMapDrawn(int mapId, Maze& maze, int centerX, int centerY);



  // The magic book's map is a cached texture of a fixed slice of the world,
  // so any layout change invalidates it.
  void markMagicBookMapDirty() { m_magicBookMapDirty = true; }

private:
  void renderInventory(Player &player, Maze &maze, ItemRenderer &itemRenderer,
                       float scale, int screenW, int screenH);
  void renderPopups(float scale, int screenW, int screenH, float totalTime);
  void generateMagicBookMap(Maze &maze);

  // Screen dimensions
  int m_screenWidth;
  int m_screenHeight;

  // Popups
  std::vector<ActivePopup> m_activePopups;

  // Inventory State
  bool m_inventoryOpen = false;
  int m_heldSlotIndex = -1; // -1 if no item is held
  bool m_heldFromCupboard = false;
  int m_activeHotbarSlot = 0;

  // Cupboard State
  bool m_cupboardInventoryOpen = false;
  int m_openedCupboardX = -1;
  int m_openedCupboardY = -1;

  // Crafting State
  int m_selectedCraftingRecipeIdx = -1;
  float m_craftFlashEndTime = 0.0f;

  // Drawn Maps Cache
  std::unordered_map<int, DrawnMapData> m_drawnMaps;
  bool m_showFullscreenMap = false;
  int m_openedMapId = 0;

  // Magic Book Map
  RenderTexture2D m_magicBookMapTexture;
  bool m_magicBookMapDirty = true;
};
