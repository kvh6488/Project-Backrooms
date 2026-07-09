#include "core/ui_manager.hpp"
#include "imgui.h"
#include "items/crafting_system.hpp"
#include "items/item_database.hpp"
#include "rlImGui.h"
#include <algorithm>
#include <cmath>


UIManager::UIManager(int screenWidth, int screenHeight)
    : m_screenWidth(screenWidth), m_screenHeight(screenHeight) {
  m_debugMapTexture.id = 0;
  m_magicBookMapTexture.id = 0;
}

UIManager::~UIManager() {
  if (m_debugMapTexture.id != 0) {
    UnloadRenderTexture(m_debugMapTexture);
  }
  if (m_magicBookMapTexture.id != 0) {
    UnloadRenderTexture(m_magicBookMapTexture);
  }
  for (auto &pair : m_drawnMaps) {
    UnloadRenderTexture(pair.second.texture);
  }
}

void UIManager::update(float dt) {
  // Update active popups
  for (auto it = m_activePopups.begin(); it != m_activePopups.end();) {
    it->timer -= dt;
    if (it->timer <= 0.0f) {
      it = m_activePopups.erase(it);
    } else {
      ++it;
    }
  }
}

void UIManager::showPopup(const std::string &text, PopupType type,
                          float duration) {
  // Check if we already have this exact popup active, if so, just refresh the
  // timer
  for (auto &popup : m_activePopups) {
    if (popup.text == text && popup.type == type) {
      popup.timer = duration;
      popup.maxDuration = duration;
      return;
    }
  }

  // Otherwise add a new popup
  m_activePopups.push_back({text, type, duration, duration});
}

void UIManager::render(Player &player, Maze &maze, ItemRenderer &itemRenderer,
                       bool isDroppingItem, float totalTime) {
  float scale = std::min((float)GetScreenWidth() / m_screenWidth,
                         (float)GetScreenHeight() / m_screenHeight);
  int screenW = GetScreenWidth();
  int screenH = GetScreenHeight();

  // 1. Check if we need to regenerate debug map
  if (m_debugMapTexture.id == 0 ||
      m_debugMapTexture.texture.width != maze.getWidth() || m_debugMapDirty) {
    if (m_debugMapTexture.id == 0 ||
        m_debugMapTexture.texture.width != maze.getWidth()) {
      if (m_debugMapTexture.id != 0)
        UnloadRenderTexture(m_debugMapTexture);
      m_debugMapTexture = LoadRenderTexture(maze.getWidth(), maze.getHeight());
    }
    generateDebugMap(maze);
    m_debugMapDirty = false;
  }

  // 1.5 Check if we need to regenerate magic book map
  if (m_magicBookMapTexture.id == 0 || m_magicBookMapDirty) {
    if (m_magicBookMapTexture.id == 0) {
      m_magicBookMapTexture = LoadRenderTexture(139, 89);
    }
    generateMagicBookMap(maze);
    m_magicBookMapDirty = false;
  }

  // 2. Door prompt (special hardcoded logic for now)
  int doorCount = player.getAvailableDoors(maze);
  if (doorCount == 1) {
    const char *msg = "Press 'K' to use door";
    int textWidth = MeasureText(msg, 30 * scale);
    DrawText(msg, (screenW - textWidth) / 2, screenH - (115 * scale),
             30 * scale, WHITE);
  } else if (doorCount >= 2) {
    const char *msg = "Press 'K' for door 1, press 'L' for door 2";
    int textWidth = MeasureText(msg, 30 * scale);
    DrawText(msg, (screenW - textWidth) / 2, screenH - (115 * scale),
             30 * scale, WHITE);
  }

  // 3. Drop prompt
  if (isDroppingItem) {
    const char *msg = "Select location to place (Q to cancel)";
    int textWidth = MeasureText(msg, 30 * scale);
    int x = (screenW - textWidth) / 2;
    int y = screenH - (190 * scale);

    DrawRectangle(x - (15 * scale), y - (5 * scale), textWidth + (30 * scale),
                  40 * scale, Fade(BLACK, 0.6f));
    DrawText(msg, x, y, 30 * scale, WHITE);
  }

  // 4. Standardized Popups
  renderPopups(scale, screenW, screenH, totalTime);

  // 5. Inventory
  renderInventory(player, maze, itemRenderer, scale, screenW, screenH);

  // 5.5 Map Rendering
  if (m_showFullscreenMap && (m_openedMapId == -1 || m_drawnMaps.find(m_openedMapId) != m_drawnMaps.end())) {
    
    RenderTexture2D texToDraw;
    int texWidth, texHeight;
    int startX, startY;
    bool isMagicBookMap = (m_openedMapId == -1);

    if (isMagicBookMap) {
      texToDraw = m_magicBookMapTexture;
      texWidth = 139;
      texHeight = 89;
      startX = 55;
      startY = 30;
    } else {
      const auto &mapData = m_drawnMaps[m_openedMapId];
      texToDraw = mapData.texture;
      texWidth = texToDraw.texture.width;
      texHeight = texToDraw.texture.height;
      startX = mapData.centerX - 26;
      startY = mapData.centerY - 17;
    }

    // The texture is flipped vertically by OpenGL, so use negative height
    Rectangle srcRec = {0, 0, (float)texWidth, -(float)texHeight};

    // Calculate relative position for player dot
    Vector2 pPos = player.getPosition();
    int gridX = (int)std::floor(pPos.x / maze.getCellSize());
    int gridY = (int)std::floor(pPos.y / maze.getCellSize());

    int relX = gridX - startX;
    int relY = gridY - startY;

    // Handle wrapping (toroidal)
    if (relX < -maze.getWidth() / 2)
      relX += maze.getWidth();
    else if (relX > maze.getWidth() / 2)
      relX -= maze.getWidth();
    if (relY < -maze.getHeight() / 2)
      relY += maze.getHeight();
    else if (relY > maze.getHeight() / 2)
      relY -= maze.getHeight();

    bool inBounds = (relX >= 0 && relX < texWidth && relY >= 0 && relY < texHeight);

    // Fullscreen Overlay
    DrawRectangle(0, 0, screenW, screenH, Fade(BLACK, 0.85f));

    float fsScale = std::min(screenW / (texWidth * 1.2f), screenH / (texHeight * 1.2f));
    float fsWidth = texWidth * fsScale;
    float fsHeight = texHeight * fsScale;
    float fsX = (screenW - fsWidth) / 2.0f;
    float fsY = (screenH - fsHeight) / 2.0f;

    DrawTexturePro(texToDraw.texture, srcRec,
                    Rectangle{fsX, fsY, fsWidth, fsHeight}, Vector2{0, 0},
                    0.0f, WHITE);

    if (inBounds) {
      float px = fsX + relX * fsScale + (fsScale / 2.0f);
      float py = fsY + relY * fsScale + (fsScale / 2.0f);
      DrawCircle(px, py, 5.0f * scale, RED);
    }
    DrawRectangleLinesEx(Rectangle{fsX, fsY, fsWidth, fsHeight}, 4.0f * scale,
                          WHITE);
  }

  // 6. ImGui Debug Overlay
  rlImGuiBegin();
  ImGui::GetIO().FontGlobalScale = scale;
  renderDebugUI(player, maze, scale);
  rlImGuiEnd();
}

void UIManager::renderPopups(float scale, int screenW, int screenH,
                             float totalTime) {
  for (const auto &popup : m_activePopups) {
    float alpha = std::min(1.0f, popup.timer * 2.0f);

    if (popup.type == PopupType::BOXED_BOTTOM) {
      int textWidth = MeasureText(popup.text.c_str(), 30 * scale);
      int x = (screenW - textWidth) / 2;
      int y = screenH - (147 * scale);

      DrawRectangle(x - (15 * scale), y - (5 * scale), textWidth + (30 * scale),
                    40 * scale, Fade(BLACK, 0.6f));
      DrawText(popup.text.c_str(), x, y, 30 * scale, Fade(WHITE, alpha));
    } else if (popup.type == PopupType::SUBTLE_BOTTOM) {
      int textWidth = MeasureText(popup.text.c_str(), 30 * scale);
      int x = (screenW - textWidth) / 2;
      int y = screenH - (150 * scale);
      DrawText(popup.text.c_str(), x, y, 30 * scale, Fade(WHITE, alpha));
    } else if (popup.type == PopupType::HEADER_GREEN) {
      int textWidth = MeasureText(popup.text.c_str(), 60 * scale);
      int x = (screenW - textWidth) / 2;
      int y = screenH / 8;

      DrawText(popup.text.c_str(), x + 2 * scale, y + 2 * scale, 60 * scale,
               Fade(BLACK, alpha * 0.7f));
      DrawText(popup.text.c_str(), x, y, 60 * scale, Fade(GREEN, alpha));
    } else if (popup.type == PopupType::HEADER_RAINBOW) {
      int textWidth = MeasureText(popup.text.c_str(), 40 * scale);
      int x = (screenW - textWidth) / 2;
      int y = 50 * scale;

      Color rainbow =
          ColorFromHSV(fmodf(totalTime * 100.0f, 360.0f), 1.0f, 1.0f);

      DrawText(popup.text.c_str(), x + 2 * scale, y + 2 * scale, 40 * scale,
               Fade(BLACK, alpha * 0.7f));
      DrawText(popup.text.c_str(), x, y, 40 * scale, Fade(rainbow, alpha));
    }
  }
}

void UIManager::renderInventory(Player &player, Maze &maze,
                                ItemRenderer &itemRenderer, float scale,
                                int screenW, int screenH) {
  float slotSize = 60 * scale;
  float padding = 10 * scale;

  ItemType hoveredItemType = ItemType::NONE;
  int hoveredInstanceId = 0;
  bool hoveredIsCrafting = false;

  auto drawSlot = [&](int index, float x, float y, bool isHotbar,
                      bool isCupboardSlot) {
    Rectangle slotRect = {x, y, slotSize, slotSize};

    // Draw slot background
    bool isHeldSlot =
        (index == m_heldSlotIndex) && (m_heldFromCupboard == isCupboardSlot);
    Color bgColor = isHeldSlot ? Fade(YELLOW, 0.3f) : Fade(BLACK, 0.7f);
    DrawRectangleRec(slotRect, bgColor);

    if (!isCupboardSlot && index == m_activeHotbarSlot) {
      DrawRectangleLinesEx(slotRect, 4.0f * scale, WHITE);
    } else {
      DrawRectangleLinesEx(slotRect, 2.0f * scale, GRAY);
    }

    auto &pInv = player.getInventoryRef();
    auto *cInvPtr =
        m_cupboardInventoryOpen
            ? &maze.getCupboardInventory(m_openedCupboardX, m_openedCupboardY)
            : nullptr;

    auto &currentInv = isCupboardSlot ? (*cInvPtr) : pInv;

    // Mouse Interaction
    if (CheckCollisionPointRec(GetMousePosition(), slotRect)) {
      DrawRectangleLinesEx(slotRect, 3.0f * scale, WHITE);

      if (m_inventoryOpen || m_cupboardInventoryOpen) {
        if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
          if (m_heldSlotIndex == -1) { // Pick up item
            if (currentInv[index].type != ItemType::NONE) {
              m_heldSlotIndex = index;
              m_heldFromCupboard = isCupboardSlot;
            }
          } else { // Swap or place item
            auto &heldInv = m_heldFromCupboard ? (*cInvPtr) : pInv;

            // Merge stacks if same type
            if (heldInv[m_heldSlotIndex].type != ItemType::NONE &&
                heldInv[m_heldSlotIndex].type == currentInv[index].type) {
              int maxStack =
                  ItemDatabase::getDef(currentInv[index].type).maxStackSize;
              int spaceInDest = maxStack - currentInv[index].count;
              int amountToMove = 0;
              if (spaceInDest > 0) {
                amountToMove =
                    std::min(heldInv[m_heldSlotIndex].count, spaceInDest);
              }
              currentInv[index].count += amountToMove;
              heldInv[m_heldSlotIndex].count -= amountToMove;
              if (heldInv[m_heldSlotIndex].count <= 0) {
                heldInv[m_heldSlotIndex].type = ItemType::NONE;
                heldInv[m_heldSlotIndex].count = 0;
              }
            } else {
              // Swap
              std::swap(heldInv[m_heldSlotIndex], currentInv[index]);
            }

            m_heldSlotIndex = -1;
            m_heldFromCupboard = false;
          }
        } else if (IsMouseButtonPressed(MOUSE_RIGHT_BUTTON)) {
          // Consume item on right click (only player can consume)
          if (m_heldSlotIndex == -1 && !isCupboardSlot) {
            player.consumeItem(index);
          }
        }
      }

      if (currentInv[index].type != ItemType::NONE && !isHeldSlot) {
        hoveredItemType = currentInv[index].type;
        hoveredInstanceId = currentInv[index].instanceId;
      }
    }

    // Draw item icon using the renderer
    auto slot = currentInv[index];
    if (slot.type != ItemType::NONE && !isHeldSlot) {
      Rectangle destRect = {x + padding, y + padding, slotSize - 2 * padding,
                            slotSize - 2 * padding};
      itemRenderer.renderItemUI(slot.type, destRect, WHITE);

      if (slot.count > 1) {
        DrawText(TextFormat("%d", slot.count), x + slotSize - 20 * scale,
                 y + slotSize - 20 * scale, 20 * scale, WHITE);
      }
    }

    if (isHotbar) {
      DrawText(TextFormat("%d", index + 1), x + 5 * scale, y + 5 * scale,
               15 * scale, LIGHTGRAY);
    }
  };

  if (m_inventoryOpen || m_cupboardInventoryOpen) {
    DrawRectangle(0, 0, screenW, screenH, Fade(BLACK, 0.75f));
  }

  // 1. Draw Hotbar (Slots 0-4)
  float hotbarTotalWidth = (5 * slotSize) + (4 * padding);
  float startX = (screenW - hotbarTotalWidth) / 2.0f;
  float startY = screenH - slotSize - (20 * scale);

  for (int i = 0; i < 5; ++i) {
    drawSlot(i, startX + i * (slotSize + padding), startY, true, false);
  }

  // 2. Draw Bag (Slots 5-19)
  if (m_inventoryOpen || m_cupboardInventoryOpen) {
    float bagStartX = (screenW - hotbarTotalWidth) / 2.0f;
    float bagStartY = startY - (3 * (slotSize + padding)) - (20 * scale);

    for (int row = 0; row < 3; ++row) {
      for (int col = 0; col < 5; ++col) {
        int index = 5 + (row * 5) + col;
        drawSlot(index, bagStartX + col * (slotSize + padding),
                 bagStartY + row * (slotSize + padding), false, false);
      }
    }

    // 3. Draw Cupboard Inventory
    if (m_cupboardInventoryOpen) {
      float cupboardTotalWidth = (10 * slotSize) + (9 * padding);
      float cupboardStartX = (screenW - cupboardTotalWidth) / 2.0f;
      float cupboardStartY =
          bagStartY - (2 * (slotSize + padding)) - (40 * scale); // 40px gap

      DrawText("Cupboard", cupboardStartX, cupboardStartY - 30 * scale,
               20 * scale, WHITE);

      for (int row = 0; row < 2; ++row) {
        for (int col = 0; col < 10; ++col) {
          int index = (row * 10) + col;
          drawSlot(index, cupboardStartX + col * (slotSize + padding),
                   cupboardStartY + row * (slotSize + padding), false, true);
        }
      }
    }

    // 4. Draw Crafting Menu
    if (m_inventoryOpen && !m_cupboardInventoryOpen) {
      float craftingStartY =
          bagStartY - (260 * scale); // Up a lot higher to fit details menu
      DrawText("Crafting", bagStartX, craftingStartY - 25 * scale, 20 * scale,
               WHITE);

      const auto &recipes = CraftingSystem::getRecipes();
      int visibleIdx = 0;

      for (size_t i = 0; i < recipes.size(); ++i) {
        const auto &recipe = recipes[i];

        // Check visibility: Has unlocked OR has at least one ingredient
        bool isVisible = player.hasUnlockedRecipe(recipe.result);
        if (!isVisible) {
          for (const auto &ing : recipe.ingredients) {
            if (player.hasIngredient(ing.type, 1)) {
              isVisible = true;
              break;
            }
          }
        }

        if (isVisible) {
          float x = bagStartX + visibleIdx * (slotSize + padding);
          float y = craftingStartY;
          Rectangle slotRect = {x, y, slotSize, slotSize};

          // Draw background
          bool isSelected = (m_selectedCraftingRecipeIdx == (int)i);
          Color bgColor = isSelected ? Fade(YELLOW, 0.3f) : Fade(BLACK, 0.7f);
          DrawRectangleRec(slotRect, bgColor);
          DrawRectangleLinesEx(slotRect, 2.0f * scale,
                               isSelected ? WHITE : GRAY);

          // Mouse Interaction
          if (CheckCollisionPointRec(GetMousePosition(), slotRect)) {
            DrawRectangleLinesEx(slotRect, 3.0f * scale, WHITE);
            if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
              if (m_selectedCraftingRecipeIdx == (int)i) {
                m_selectedCraftingRecipeIdx = -1; // Toggle off
              } else {
                m_selectedCraftingRecipeIdx = (int)i; // Select
              }
            }
            hoveredItemType = recipe.result;
            hoveredIsCrafting = true;
          }

          // Draw icon
          Rectangle destRect = {x + padding, y + padding,
                                slotSize - 2 * padding, slotSize - 2 * padding};
          itemRenderer.renderItemUI(recipe.result, destRect, WHITE);

          visibleIdx++;
        }
      }

      if (visibleIdx == 0) {
        DrawText("Go get yourself some items", bagStartX, craftingStartY,
                 20 * scale, GRAY);
      }

      // Draw Selected Recipe Details
      if (m_selectedCraftingRecipeIdx >= 0 &&
          m_selectedCraftingRecipeIdx < recipes.size()) {
        const auto &recipe = recipes[m_selectedCraftingRecipeIdx];
        const auto &resultDef = ItemDatabase::getDef(recipe.result);

        float detailsX = bagStartX;
        float detailsY = craftingStartY + slotSize +
                         (10 * scale); // Below the crafting icons

        // Draw title
        DrawText(TextFormat("Recipe: %s", resultDef.name.c_str()), detailsX,
                 detailsY, 20 * scale, WHITE);

        // Draw ingredients
        float ingX = detailsX;
        float ingY = detailsY + 25 * scale;
        for (size_t i = 0; i < recipe.ingredients.size(); ++i) {
          const auto &ing = recipe.ingredients[i];
          Rectangle ingSlotRect = {ingX, ingY, slotSize * 0.8f,
                                   slotSize * 0.8f};

          DrawRectangleRec(ingSlotRect, Fade(BLACK, 0.7f));
          DrawRectangleLinesEx(ingSlotRect, 2.0f * scale, GRAY);

          Rectangle ingDestRect = {ingX + padding * 0.8f, ingY + padding * 0.8f,
                                   (slotSize - 2 * padding) * 0.8f,
                                   (slotSize - 2 * padding) * 0.8f};
          itemRenderer.renderItemUI(ing.type, ingDestRect, WHITE);

          // Count text
          int playerHas = 0;
          for (auto slot : player.getInventory())
            if (slot.type == ing.type)
              playerHas += slot.count;

          bool hasEnough = playerHas >= ing.count;
          Color textColor = hasEnough ? GREEN : RED;

          if (!hasEnough && GetTime() < m_craftFlashEndTime) {
            if ((int)(GetTime() * 15) % 2 == 0) {
              textColor = WHITE;
            }
          }

          DrawText(TextFormat("%d/%d", playerHas, ing.count),
                   ingX + slotSize * 0.8f + 5 * scale, ingY + slotSize * 0.3f,
                   20 * scale, textColor);

          // Mouse Hover on ingredient
          if (CheckCollisionPointRec(GetMousePosition(), ingSlotRect)) {
            hoveredItemType = ing.type;
            hoveredIsCrafting = true;
          }

          ingX += slotSize * 0.8f + 60 * scale;
        }

        // Draw Craft Button
        bool canCraft = player.canCraft(recipe);
        float btnW = 100 * scale;
        float btnH = 40 * scale;
        float btnX = bagStartX + (5 * slotSize + 4 * padding) - btnW;
        float btnY = detailsY + 15 * scale;

        Rectangle btnRect = {btnX, btnY, btnW, btnH};
        DrawRectangleRec(btnRect,
                         canCraft ? Fade(GREEN, 0.6f) : Fade(RED, 0.6f));
        DrawRectangleLinesEx(btnRect, 2.0f * scale, WHITE);

        int textW = MeasureText("Craft", 20 * scale);
        DrawText("Craft", btnX + (btnW - textW) / 2,
                 btnY + (btnH - 20 * scale) / 2, 20 * scale, WHITE);

        if (CheckCollisionPointRec(GetMousePosition(), btnRect)) {
          DrawRectangleLinesEx(btnRect, 3.0f * scale, YELLOW);
          if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
            if (canCraft) {
              player.craftItem(recipe);
            } else {
              m_craftFlashEndTime = GetTime() + 0.5f; // Flash for 0.5 seconds
            }
          }
        }
      }
    }

    if (m_heldSlotIndex != -1) {
      Vector2 mousePos = GetMousePosition();

      auto &pInv = player.getInventoryRef();
      auto *cInvPtr =
          m_cupboardInventoryOpen
              ? &maze.getCupboardInventory(m_openedCupboardX, m_openedCupboardY)
              : nullptr;
      auto &heldInv = m_heldFromCupboard ? (*cInvPtr) : pInv;
      auto slot = heldInv[m_heldSlotIndex];

      Rectangle destRect = {mousePos.x - slotSize / 2 + padding,
                            mousePos.y - slotSize / 2 + padding,
                            slotSize - 2 * padding, slotSize - 2 * padding};
      itemRenderer.renderItemUI(slot.type, destRect, Fade(WHITE, 0.8f));

      if (slot.count > 1) {
        DrawText(TextFormat("%d", slot.count),
                 mousePos.x + slotSize / 2 - 20 * scale,
                 mousePos.y + slotSize / 2 - 20 * scale, 20 * scale, WHITE);
      }
    }

    if (hoveredItemType != ItemType::NONE && m_heldSlotIndex == -1) {
      const auto &def = ItemDatabase::getDef(hoveredItemType);
      std::string desc = def.description;


      // Simple line break measurement estimation
      int maxDescW = 0;
      int lines = 1;
      std::string currentLineForMeasure;
      for (char c : desc) {
        if (c == '\n') {
          int w = MeasureText(currentLineForMeasure.c_str(), 15 * scale);
          if (w > maxDescW)
            maxDescW = w;
          currentLineForMeasure = "";
          lines++;
        } else {
          currentLineForMeasure += c;
        }
      }
      if (!currentLineForMeasure.empty()) {
        int w = MeasureText(currentLineForMeasure.c_str(), 15 * scale);
        if (w > maxDescW)
          maxDescW = w;
      }

      int nameW = MeasureText(def.name.c_str(), 20 * scale);
      int boxW = std::max(nameW, maxDescW) + 40 * scale;
      int boxH = 40 * scale + (lines * 20 * scale);

      Vector2 mousePos = GetMousePosition();
      DrawRectangle(mousePos.x + 10 * scale, mousePos.y + 10 * scale, boxW,
                    boxH, Fade(BLACK, 0.9f));
      DrawRectangleLines(mousePos.x + 10 * scale, mousePos.y + 10 * scale, boxW,
                         boxH, GRAY);
      DrawText(def.name.c_str(), mousePos.x + 20 * scale,
               mousePos.y + 15 * scale, 20 * scale, YELLOW);

      // Draw multi-line text
      int lineY = mousePos.y + 40 * scale;
      std::string currentLine;
      for (char c : desc) {
        if (c == '\n') {
          DrawText(currentLine.c_str(), mousePos.x + 20 * scale, lineY,
                   15 * scale, LIGHTGRAY);
          currentLine = "";
          lineY += 20 * scale;
        } else {
          currentLine += c;
        }
      }
      if (!currentLine.empty()) {
        DrawText(currentLine.c_str(), mousePos.x + 20 * scale, lineY,
                 15 * scale, LIGHTGRAY);
      }
    }
  }
}

void UIManager::renderDebugUI(Player &player, Maze &maze, float scale) {
  ImGui::Begin("Debug Engine");
  ImGui::Text("FPS: %d", GetFPS());
  ImGui::Separator();
  ImGui::Text("Maze Statistics");
  ImGui::Text("Number of Rooms: %zu", maze.getRooms().size());

  int totalCells = maze.getWidth() * maze.getHeight();
  float coveragePercent = ((float)maze.getNonWallCount() / totalCells) * 100.0f;
  float corridorPercent =
      ((float)maze.getCorridorCount() / totalCells) * 100.0f;

  ImGui::Text("Maze Coverage: %.1f%%", coveragePercent);
  ImGui::Text("Corridor Coverage: %.1f%%", corridorPercent);

  ImGui::Separator();
  ImGui::Checkbox("Flashlight Torch Mode", &m_flashlightEnabled);
  if (ImGui::Checkbox("Show Regeneration Zones", &m_showGenerationZones)) {
    m_debugMapDirty = true;
  }
  if (ImGui::Checkbox("Show Radiation Zones", &m_showRadiationOnDebugMap)) {
    m_debugMapDirty = true;
  }

  ImGui::Separator();
  ImGui::Text("View Settings");
  if (ImGui::Button(IsWindowFullscreen() ? "Exit Fullscreen (F11)"
                                         : "Enter Fullscreen (F11)")) {
    ToggleFullscreen();
  }
  ImGui::SliderFloat("Tile Zoom", &m_cameraZoom, 0.5f, 5.0f);

  if (ImGui::Button("Regenerate Tic-Tac-Toe Zones")) {
    m_triggerTicTacToeRegen = true;
  }

  ImGui::Separator();
  ImGui::Text("Flashlight Tweaks");
  if (ImGui::SliderFloat("Degree Cut", &m_lightConeAngle, 90.0f, 360.0f))
    m_lightSettingsChanged = true;
  if (ImGui::SliderFloat("Circle Size", &m_lightSizeScale, 1.0f, 6.0f))
    m_lightSettingsChanged = true;
  if (ImGui::SliderFloat("Angular Fade Strength", &m_lightFadeStrength, 0.1f,
                         10.0f))
    m_lightSettingsChanged = true;

  ImGui::Separator();
  ImGui::Text("Minimap");

  float availWidth = ImGui::GetContentRegionAvail().x;
  float mapRatio = (float)maze.getHeight() / (float)maze.getWidth();
  Vector2 mapDisplaySize = {availWidth, availWidth * mapRatio};
  ImVec2 mapScreenPos = ImGui::GetCursorScreenPos();
  rlImGuiImageRect(&m_debugMapTexture.texture, (int)mapDisplaySize.x,
                   (int)mapDisplaySize.y,
                   Rectangle{0, 0, (float)m_debugMapTexture.texture.width,
                             -(float)m_debugMapTexture.texture.height});

  // Draw Player Dot on Minimap
  Vector2 pPos = player.getPosition();
  int gridX = (int)std::floor(pPos.x / maze.getCellSize());
  int gridY = (int)std::floor(pPos.y / maze.getCellSize());

  int wrappedX = (gridX % maze.getWidth() + maze.getWidth()) % maze.getWidth();
  int wrappedY =
      (gridY % maze.getHeight() + maze.getHeight()) % maze.getHeight();

  float percentX = (float)wrappedX / maze.getWidth();
  float percentY = (float)wrappedY / maze.getHeight();

  ImVec2 playerScreenPos =
      ImVec2(mapScreenPos.x + (percentX * mapDisplaySize.x),
             mapScreenPos.y + (percentY * mapDisplaySize.y));

  ImGui::GetWindowDrawList()->AddCircleFilled(playerScreenPos, 3.0f,
                                              IM_COL32(255, 0, 0, 255));
  ImGui::End();
}

void UIManager::generateDebugMap(Maze &maze) {
  BeginTextureMode(m_debugMapTexture);
  ClearBackground(BLANK);
  for (int y = 0; y < maze.getHeight(); ++y) {
    for (int x = 0; x < maze.getWidth(); ++x) {
      if (maze.getCell(x, y) == Maze::CELL_WALL) {
        DrawPixel(x, y, Color{100, 100, 100, 255});
      } else {
        if (m_showRadiationOnDebugMap && maze.getRadiationLevel(x, y) > 0) {
          DrawPixel(x, y, Color{0, 255, 0, 255});
        } else if (m_showGenerationZones && maze.isShiftingZone(x, y)) {
          DrawPixel(x, y, Color{255, 100, 100, 255});
        } else {
          DrawPixel(x, y, Color{30, 30, 35, 255});
        }
      }
    }
  }

  if (m_showRadiationOnDebugMap) {
    for (int y = 0; y < maze.getHeight(); ++y) {
      for (int x = 0; x < maze.getWidth(); ++x) {
        if (maze.getItem(x, y) == ItemType::TOXIC_WASTE) {
          DrawRectangle(x - 1, y - 1, 3, 3, BLUE);
        }
      }
    }
  }

  if (maze.isMagicBookSpawned()) {
    DrawRectangle(maze.getMagicBookX() - 1, maze.getMagicBookY() - 1, 3, 3, PURPLE);
  }

  EndTextureMode();
}


void UIManager::generateMagicBookMap(Maze &maze) {
  int startX = 55;
  int startY = 30;
  int width = 139;
  int height = 89;

  BeginTextureMode(m_magicBookMapTexture);
  ClearBackground(BLANK);
  for (int y = 0; y < height; ++y) {
    for (int x = 0; x < width; ++x) {
      int gridX = startX + x;
      int gridY = startY + y;
      
      // Handle wrapping
      gridX = (gridX % maze.getWidth() + maze.getWidth()) % maze.getWidth();
      gridY = (gridY % maze.getHeight() + maze.getHeight()) % maze.getHeight();

      if (maze.getCell(gridX, gridY) == Maze::CELL_WALL) {
        DrawPixel(x, y, Color{100, 100, 100, 255});
      } else {
        if (maze.isShiftingZone(gridX, gridY)) {
          DrawPixel(x, y, Color{255, 100, 100, 255});
        } else {
          DrawPixel(x, y, Color{30, 30, 35, 255});
        }
      }
    }
  }
  EndTextureMode();
}

void UIManager::markMapDrawn(int mapId, Maze &maze, int centerX, int centerY) {
  if (m_drawnMaps.find(mapId) != m_drawnMaps.end())
    return;

  RenderTexture2D tex = LoadRenderTexture(53, 35);
  BeginTextureMode(tex);
  ClearBackground(Color{30, 30, 35, 255}); // default off-map color

  int startX = centerX - 26;
  int startY = centerY - 17;

  for (int y = 0; y < 35; ++y) {
    for (int x = 0; x < 53; ++x) {
      int gridX = (startX + x) % maze.getWidth();
      if (gridX < 0)
        gridX += maze.getWidth();
      int gridY = (startY + y) % maze.getHeight();
      if (gridY < 0)
        gridY += maze.getHeight();

      if (maze.getCell(gridX, gridY) == Maze::CELL_WALL) {
        DrawPixel(x, y, Color{100, 100, 100, 255});
      } else {
        DrawPixel(x, y, Color{30, 30, 35, 255});
      }
    }
  }
  EndTextureMode();

  m_drawnMaps[mapId] = {mapId, centerX, centerY, tex};
}
