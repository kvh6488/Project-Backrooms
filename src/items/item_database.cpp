#include "items/item_database.hpp"

// Initialize static members
std::vector<ItemDefinition> ItemDatabase::m_definitions;
ItemDefinition ItemDatabase::m_fallbackDef = {
    "Unknown Item", "Report this bug.", false, false, false, 0, {0, 0, 0, 0},
    UiTexture::MUSHROOMS};

void ItemDatabase::init() {
  // Determine the max enum value we need to store.
  m_definitions.resize(10); // NONE = 0, up to MAGIC_BOOK_OF_MAPS = 9

  // NONE
  m_definitions[(int)ItemType::NONE] = {"None",       "", false, false,
                                        false,        0,  {0, 0, 0, 0},
                                        UiTexture::MUSHROOMS};

  // TOXIC_WASTE
  m_definitions[(int)ItemType::TOXIC_WASTE] = {
      "Toxic Waste", "Highly radioactive waste barrel. Avoid at all costs.",
      true,          false,
      false,         1,
      {0, 0, 16, 16}, // Drawn as a plain green rect in UI, not from an atlas
      UiTexture::MUSHROOMS};

  // CUPBOARD
  m_definitions[(int)ItemType::CUPBOARD] = {
      "Cupboard",     "A sturdy filing cabinet. Might contain supplies.",
      true,           false,
      false,          1,
      {0, 0, 16, 16}, // Never shown in UI — cupboards are not pickable
      UiTexture::MUSHROOMS};

  // MUSHROOM
  m_definitions[(int)ItemType::MUSHROOM] = {
      "Mushroom",      "A standard mushroom.",
      true,            true,
      true,            6,
      {16, 0, 16, 16}, // MUSHROOM UI Rect
      UiTexture::MUSHROOMS};

  // MAGIC_MUSHROOM
  m_definitions[(int)ItemType::MAGIC_MUSHROOM] = {
      "Magic Mushroom",
      "It glows slightly. Eating it might show you the way...",
      true,
      true,
      true,
      6,
      {0, 32, 16, 16}, // MAGIC_MUSHROOM UI Rect
      UiTexture::MUSHROOMS};

  // PAPER
  m_definitions[(int)ItemType::PAPER] = {
      "Paper",
      "A blank piece of paper. Could be useful for mapping.",
      false,
      true,
      false,
      4,
      {96, 339, 14, 10},
      UiTexture::WORKSHOP};

  // PENCIL
  m_definitions[(int)ItemType::PENCIL] = {
      "Pencil", "A simple wooden pencil.", false, true, false,
      1,        {114, 305, 13, 12},        UiTexture::WORKSHOP};

  // MAP
  m_definitions[(int)ItemType::MAP] = {"Map",
                                       "A handmade map of the area",
                                       false,
                                       true,
                                       true,
                                       1,
                                       {160, 96, 16, 16},
                                       UiTexture::WORKSHOP_ICONS};

  // TABLE
  m_definitions[(int)ItemType::TABLE] = {
      "Table", "A simple wooden table.", true, false, false, 1,
      {0, 0, 16, 16}, // Never shown in UI — tables are not pickable
      UiTexture::MUSHROOMS};

  // MAGIC_BOOK_OF_MAPS
  m_definitions[(int)ItemType::MAGIC_BOOK_OF_MAPS] = {
      "Magic book of maps",
      "Every day is a new chapter",
      false,
      true,
      false,
      1,
      {48.0f, 97.0f, 16.0f, 16.0f},
      UiTexture::RITUAL};
}

const ItemDefinition &ItemDatabase::getDef(ItemType type) {
  int index = static_cast<int>(type);
  if (index >= 0 && index < (int)m_definitions.size()) {
    return m_definitions[index];
  }
  return m_fallbackDef;
}
