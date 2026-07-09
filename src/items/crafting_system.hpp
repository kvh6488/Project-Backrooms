#pragma once

#include "items/item.hpp"
#include <vector>

struct Recipe {
    ItemType result;
    std::vector<InventorySlot> ingredients;
};

class CraftingSystem {
public:
    static void init();
    static const std::vector<Recipe>& getRecipes();

private:
    static std::vector<Recipe> m_recipes;
};
