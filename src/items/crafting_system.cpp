#include "items/crafting_system.hpp"

std::vector<Recipe> CraftingSystem::m_recipes;

void CraftingSystem::init() {
    m_recipes.clear();

    // Map recipe: 1 Pencil + 1 Paper
    Recipe mapRecipe;
    mapRecipe.result = ItemType::MAP;
    mapRecipe.ingredients.push_back({ItemType::PENCIL, 1});
    mapRecipe.ingredients.push_back({ItemType::PAPER, 1});
    
    m_recipes.push_back(mapRecipe);
}

const std::vector<Recipe>& CraftingSystem::getRecipes() {
    return m_recipes;
}
