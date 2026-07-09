#pragma once
#include "world/maze.hpp"
#include <raylib.h>

// ============================================================================
// ItemRenderer — Decoupled Item Rendering System
// ============================================================================
// Owns all item-related textures and handles both world-space item drawing
// and screen-space inventory icon drawing. Extracted from MazeRenderer to
// enforce Single Responsibility: MazeRenderer handles terrain/walls,
// ItemRenderer handles items on top.
//
// Design Pattern: Strategy (separating rendering concerns by domain).
//
// Crucially, items are rendered in a SEPARATE pass AFTER the player, so
// furniture (cupboards, etc.) renders ON TOP of the player sprite. This
// gives the visual effect of the player walking "behind" tall furniture.
// ============================================================================
class ItemRenderer {
public:
  ItemRenderer();
  ~ItemRenderer();

  void loadTextures();

  // --- World-Space Rendering ---
  // Draws all visible items in the maze. Called AFTER the player renderer
  // inside BeginMode2D so items layer on top of the player sprite.
  void render(const Maze &maze, const Camera2D &camera,
              AreaState state) const;

  // --- Screen-Space UI Rendering ---
  // Draws an item icon for the inventory/hotbar UI.
  void renderItemUI(ItemType type, Rectangle destRect,
                    Color tint = WHITE) const;

private:
  Texture2D m_postApocWorkshopTextures;
  Texture2D m_doodadsTexture;
  Texture2D m_mushroomTexture;
  Texture2D m_postApocIconsTexture;
  Texture2D m_ritualTexture;
};
