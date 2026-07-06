#pragma once
#include "maze.hpp"
#include <map>
#include <random>

// ============================================================================
// ItemSpawner — Encapsulated Item Placement Logic
// ============================================================================
// This class is solely responsible for evaluating spawn conditions and
// placing items onto the Maze's item grid. It does NOT own the items after
// placement — the Maze's m_items array is the single source of truth.
//
// Design Pattern: Strategy / Command.
// Each item type has its own placement rules (e.g., barrels avoid doorways,
// mushrooms only spawn in radiated rooms). Adding a new item type means
// adding a new spawn method here — the Maze and Renderer remain untouched
// beyond recognising the new ItemType enum value.
//
// The spawner holds a reference to a shared std::mt19937 RNG so that the
// same seed always produces the same item layout, regardless of whether
// the spawner is called from the initial generation or from zone
// regeneration.
// ============================================================================
class ItemSpawner {
public:
  // Constructor takes a reference to the application-wide RNG.
  // This ensures deterministic item placement from a given seed.
  explicit ItemSpawner(std::mt19937& rng);

  // --- Initial Spawning ---
  // Called once after maze generation to populate the world with items.
  // Internally decides how many of each item to spawn (e.g., 3-5 barrels).
  void spawnInitialItems(Maze& maze);

  // --- Zone Regeneration Support ---
  // After a shifting zone regenerates, this method replaces destroyed items
  // inside the newly carved zone. The itemsToSpawn map comes from
  // Maze::clearItemsInZone(), telling us exactly what was lost.
  void respawnItems(Maze& maze, const std::map<ItemType, int>& itemsToSpawn,
                    int zoneX, int zoneY, int zoneW, int zoneH);

private:
  std::mt19937& m_rng;

  // --- Per-Item-Type Spawn Logic ---
  // Each method encapsulates the unique placement rules for its item type.

  // Spawn 'count' barrels anywhere in the maze (or within a bounding box).
  // Rules: Must be CELL_ROOM, must NOT have an adjacent CELL_CORRIDOR
  // (avoids doorway cells), must have NONE item currently.
  void spawnBarrels(Maze& maze, int count,
                    int boundsX, int boundsY, int boundsW, int boundsH);

  // Spawn mushrooms (normal or magic) in room corners across the maze or bounding box.
  // Target parameter tells us exactly how many to spawn during respawning
  // (target = -1 means normal generation based on 6% chance).
  void spawnMushrooms(Maze& maze, ItemType type, int target = -1,
                      int boundsX = 0, int boundsY = 0,
                      int boundsW = -1, int boundsH = -1);

  // Grows a single clump of 2-5 mushrooms starting at (startX, startY).
  // Returns the actual number of mushrooms placed.
  int spawnMushroomClump(Maze& maze, int startX, int startY, ItemType type);

  // Helper: Check if a room cell is adjacent to any corridor cell
  // (i.e., it's a doorway transition cell and items should not go here).
  bool isNearCorridor(const Maze& maze, int x, int y) const;

  // Helper: Check if a room cell is in a corner (walls on two adjacent sides).
  bool isRoomCorner(const Maze& maze, int x, int y) const;
};
