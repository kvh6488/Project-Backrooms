#pragma once
#include <string>
#include <raylib.h>

// ============================================================================
// ItemType Enum
// ============================================================================
// Unified ID system for all items and grid interactables in the game.
// Some of these exist only on the maze grid (isPlaceable), some only in 
// inventories (isPickable), and some in both.
enum class ItemType : int {
  NONE = 0,
  
  // World Features (Grid only)
  TOXIC_WASTE = 1,
  CUPBOARD = 2,
  TABLE = 8,
  
  // World & Inventory 
  MUSHROOM = 3,
  MAGIC_MUSHROOM = 4,

  // Inventory Only
  PAPER = 5,
  PENCIL = 6,
  MAP = 7,
  MAGIC_BOOK_OF_MAPS = 9
};

// ============================================================================
// UiTexture
// ============================================================================
// Names the atlas a UI icon is cut from. The definition stores the name; the
// ItemRenderer owns the actual Texture2D handles and resolves the name at draw
// time. That keeps the enum free of any dependency on loaded GPU resources,
// and means a new item is one edit in ItemDatabase rather than two.
enum class UiTexture : int {
  MUSHROOMS = 0, // mushrooms_pixel_asset.png — also the fallback
  WORKSHOP,      // PostApoc_Workshop_WithShadow.png
  WORKSHOP_ICONS,// PostApoc_Workshop_Icons.png
  RITUAL         // Spritesheet_TheDarkRitual_BigWander.png
};

// ============================================================================
// ItemDefinition
// ============================================================================
// Centralized metadata for a specific ItemType (Type Object Pattern).
struct ItemDefinition {
    std::string name;
    std::string description;

    // Core behaviors
    bool isPlaceable;   // Can this be instantiated as a physical tile in the maze?
    bool isPickable;    // Can this be picked up and stored in an inventory?
    bool isConsumable;  // Can the player "use" or "eat" this item directly?

    int maxStackSize;
    Rectangle uiSpriteRect; // The texture rectangle for rendering this item in UI
    UiTexture uiTexture;    // Which atlas uiSpriteRect indexes into
};

// ============================================================================
// InventorySlot
// ============================================================================
struct InventorySlot {
  ItemType type = ItemType::NONE;
  int count = 0;
  int instanceId = 0;
};

// ============================================================================
// Inventory sizing
// ============================================================================
// One container size serves the player's bag and every cupboard, so a stack
// can move between them without a capacity check. The hotbar is not a separate
// container - it is the first HOTBAR_SLOTS entries of the player's bag, which
// is why slot 0-4 indices appear in both the hotbar draw and the bag logic.
// ============================================================================
inline constexpr int INVENTORY_SLOTS = 20;
inline constexpr int HOTBAR_SLOTS = 5;
