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
  
  // World & Inventory 
  MUSHROOM = 3,
  MAGIC_MUSHROOM = 4,

  // Inventory Only
  PAPER = 5,
  PENCIL = 6,
  MAP = 7
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
};

// ============================================================================
// InventorySlot
// ============================================================================
struct InventorySlot {
  ItemType type = ItemType::NONE;
  int count = 0;
};
