#pragma once
#include "items/item.hpp"
#include <vector>

// ============================================================================
// ItemDatabase
// ============================================================================
// Central registry for all ItemDefinitions. Provides O(1) lookups for item 
// metadata. Replaces scattered switch statements and hardcoded behavior flags.
class ItemDatabase {
public:
    static void init();
    static const ItemDefinition& getDef(ItemType type);
    
private:
    static std::vector<ItemDefinition> m_definitions;
    static ItemDefinition m_fallbackDef;
};
