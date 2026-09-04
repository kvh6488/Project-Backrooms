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

  // Draws the magic book of maps on its table, as a SEPARATE pass.
  //
  // Why it is not part of render(): the trip shader is a screen-space
  // post-process over the whole scene render texture, so the only way to
  // exempt an object from the distortion is to draw it in a later pass,
  // after EndShaderMode(). The book is a stable ritual object and must not
  // warp with the hallucination.
  // tripOffset shifts the book in WORLD units to track the apparent motion
  // the trip shader gives the table underneath it. Without it the book is the
  // only stationary thing on a swimming screen, which reads as the book
  // floating in circles above the table. PlayingState computes the value
  // because it owns the shader; the renderer stays ignorant of it.
  //
  // glowScale multiplies the halo radius, for tuning by eye.
  void renderMagicBookOverlay(const Maze &maze, const Camera2D &camera,
                              AreaState state, Vector2 tripOffset,
                              float glowScale) const;

  // --- Screen-Space UI Rendering ---
  // Draws an item icon for the inventory/hotbar UI.
  void renderItemUI(ItemType type, Rectangle destRect,
                    Color tint = WHITE) const;

private:
  // Resolved sprite for a table root tile: which atlas rect to sample and
  // where it lands in world space.
  struct TableSprite {
    Rectangle src;  // Source rect within m_postApocWorkshopTextures
    Rectangle dest; // World-space destination rect
    bool valid;     // False when (x, y) is not a table ROOT tile (state 1 or 3)
  };

  // Single source of truth for table geometry. Both the item pass and the
  // magic book overlay pass need the destination rect, and the two variants
  // (grey / non-grey, chosen by a spatial hash) have DIFFERENT pixel
  // dimensions - so computing it in two places silently misplaces the book.
  TableSprite computeTableSprite(const Maze &maze, int x, int y) const;

  // Maps the atlas name an ItemDefinition carries to the loaded handle.
  Texture2D atlasFor(UiTexture which) const;

  Texture2D m_postApocWorkshopTextures;
  Texture2D m_doodadsTexture;
  Texture2D m_mushroomTexture;
  Texture2D m_postApocIconsTexture;
  Texture2D m_ritualTexture;
};
