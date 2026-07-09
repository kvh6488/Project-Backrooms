#include "items/item_database.hpp"

// Initialize static members
std::vector<ItemDefinition> ItemDatabase::m_definitions;
ItemDefinition ItemDatabase::m_fallbackDef = {
    "Unknown Item", "Report this bug.", false, false, false, 0, {0, 0, 0, 0}};

void ItemDatabase::init() {
  // Determine the max enum value we need to store.
  m_definitions.resize(9); // NONE = 0, up to TABLE = 8

  // NONE
  m_definitions[(int)ItemType::NONE] = {"None", "", false,       false,
                                        false,  0,  {0, 0, 0, 0}};

  // TOXIC_WASTE
  m_definitions[(int)ItemType::TOXIC_WASTE] = {
      "Toxic Waste", "Highly radioactive waste barrel. Avoid at all costs.",
      true,          false,
      false,         1,
      {0, 0, 16, 16} // Will draw standard green rect for UI
  };

  // CUPBOARD
  m_definitions[(int)ItemType::CUPBOARD] = {
      "Cupboard",    "A sturdy filing cabinet. Might contain supplies.",
      true,          false,
      false,         1,
      {0, 0, 16, 16}};

  // MUSHROOM
  m_definitions[(int)ItemType::MUSHROOM] = {
      "Mushroom",     "A weird looking mushroom. Probably shouldn't eat it.",
      true,           true,
      true,           6,
      {16, 0, 16, 16} // MUSHROOM UI Rect
  };

  // MAGIC_MUSHROOM
  m_definitions[(int)ItemType::MAGIC_MUSHROOM] = {
      "Magic Mushroom",
      "It glows slightly. Eating it might show you the way...",
      true,
      true,
      true,
      6,
      {0, 32, 16, 16} // MAGIC_MUSHROOM UI Rect
  };

  // PAPER
  m_definitions[(int)ItemType::PAPER] = {
      "Paper",
      "A blank piece of paper. Could be useful for mapping.",
      false,
      true,
      false,
      4,
      {96, 339, 14, 10}};

  // PENCIL
  m_definitions[(int)ItemType::PENCIL] = {
      "Pencil", "A simple wooden pencil.", false, true, false,
      1,        {114, 305, 13, 12}};

  // MAP
  m_definitions[(int)ItemType::MAP] = {"Map",
                                       "A handmade map of the area",
                                       false,
                                       true,
                                       true,
                                       1,
                                       {160, 96, 16, 16}};

  // TABLE
  m_definitions[(int)ItemType::TABLE] = {
      "Table", "A simple wooden table.", true, false, false, 1, {0, 0, 16, 16}};
}

const ItemDefinition &ItemDatabase::getDef(ItemType type) {
  int index = static_cast<int>(type);
  if (index >= 0 && index < m_definitions.size()) {
    return m_definitions[index];
  }
  return m_fallbackDef;
}
