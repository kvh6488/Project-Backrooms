#include "core/ui_manager.hpp"
#include "imgui.h"
#include "rlImGui.h"
#include <cmath>
#include <algorithm>

UIManager::UIManager(int screenWidth, int screenHeight)
    : m_screenWidth(screenWidth), m_screenHeight(screenHeight) {
    m_minimapTexture.id = 0; // Initialize as empty
}

UIManager::~UIManager() {
    if (m_minimapTexture.id != 0) {
        UnloadRenderTexture(m_minimapTexture);
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

void UIManager::showPopup(const std::string& text, PopupType type, float duration) {
    // Check if we already have this exact popup active, if so, just refresh the timer
    for (auto& popup : m_activePopups) {
        if (popup.text == text && popup.type == type) {
            popup.timer = duration;
            popup.maxDuration = duration;
            return;
        }
    }
    
    // Otherwise add a new popup
    m_activePopups.push_back({text, type, duration, duration});
}

void UIManager::render(Player& player, Maze& maze, ItemRenderer& itemRenderer, bool isDroppingItem, float totalTime) {
    float scale = std::min((float)GetScreenWidth() / m_screenWidth,
                           (float)GetScreenHeight() / m_screenHeight);
    int screenW = GetScreenWidth();
    int screenH = GetScreenHeight();

    // 1. Check if we need to regenerate minimap
    if (m_minimapTexture.id == 0 || m_minimapTexture.texture.width != maze.getWidth() || m_minimapDirty) {
        if (m_minimapTexture.id == 0 || m_minimapTexture.texture.width != maze.getWidth()) {
            if (m_minimapTexture.id != 0) UnloadRenderTexture(m_minimapTexture);
            m_minimapTexture = LoadRenderTexture(maze.getWidth(), maze.getHeight());
        }
        generateMinimap(maze);
        m_minimapDirty = false;
    }

    // 2. Door prompt (special hardcoded logic for now)
    int doorCount = player.getAvailableDoors(maze);
    if (doorCount == 1) {
        const char *msg = "Press 'K' to use door";
        int textWidth = MeasureText(msg, 30 * scale);
        DrawText(msg, (screenW - textWidth) / 2, screenH - (115 * scale), 30 * scale, WHITE);
    } else if (doorCount >= 2) {
        const char *msg = "Press 'K' for door 1, press 'L' for door 2";
        int textWidth = MeasureText(msg, 30 * scale);
        DrawText(msg, (screenW - textWidth) / 2, screenH - (115 * scale), 30 * scale, WHITE);
    }

    // 3. Drop prompt
    if (isDroppingItem) {
        const char *msg = "Select location to place (Q to cancel)";
        int textWidth = MeasureText(msg, 30 * scale);
        int x = (screenW - textWidth) / 2;
        int y = screenH - (190 * scale);

        DrawRectangle(x - (15 * scale), y - (5 * scale), textWidth + (30 * scale), 40 * scale, Fade(BLACK, 0.6f));
        DrawText(msg, x, y, 30 * scale, WHITE);
    }

    // 4. Standardized Popups
    renderPopups(scale, screenW, screenH, totalTime);

    // 5. Inventory
    renderInventory(player, itemRenderer, scale, screenW, screenH);

    // 6. ImGui Debug Overlay
    rlImGuiBegin();
    ImGui::GetIO().FontGlobalScale = scale;
    renderDebugUI(player, maze, scale);
    rlImGuiEnd();
}

void UIManager::renderPopups(float scale, int screenW, int screenH, float totalTime) {
    for (const auto& popup : m_activePopups) {
        float alpha = std::min(1.0f, popup.timer);

        if (popup.type == PopupType::BOXED_BOTTOM) {
            int textWidth = MeasureText(popup.text.c_str(), 30 * scale);
            int x = (screenW - textWidth) / 2;
            int y = screenH - (147 * scale);

            DrawRectangle(x - (15 * scale), y - (5 * scale), textWidth + (30 * scale),
                          40 * scale, Fade(BLACK, 0.6f));
            DrawText(popup.text.c_str(), x, y, 30 * scale, Fade(WHITE, alpha));
        }
        else if (popup.type == PopupType::SUBTLE_BOTTOM) {
            int textWidth = MeasureText(popup.text.c_str(), 30 * scale);
            int x = (screenW - textWidth) / 2;
            int y = screenH - (150 * scale); 
            DrawText(popup.text.c_str(), x, y, 30 * scale, Fade(WHITE, alpha));
        }
        else if (popup.type == PopupType::HEADER_GREEN) {
            int textWidth = MeasureText(popup.text.c_str(), 60 * scale);
            int x = (screenW - textWidth) / 2;
            int y = screenH / 8;

            DrawText(popup.text.c_str(), x + 2 * scale, y + 2 * scale, 60 * scale, Fade(BLACK, alpha * 0.7f));
            DrawText(popup.text.c_str(), x, y, 60 * scale, Fade(GREEN, alpha));
        }
        else if (popup.type == PopupType::HEADER_RAINBOW) {
            int textWidth = MeasureText(popup.text.c_str(), 40 * scale);
            int x = (screenW - textWidth) / 2;
            int y = 50 * scale;

            Color rainbow = ColorFromHSV(fmodf(totalTime * 100.0f, 360.0f), 1.0f, 1.0f);

            DrawText(popup.text.c_str(), x + 2 * scale, y + 2 * scale, 40 * scale, Fade(BLACK, alpha * 0.7f));
            DrawText(popup.text.c_str(), x, y, 40 * scale, Fade(rainbow, alpha));
        }
    }
}

void UIManager::renderInventory(Player& player, ItemRenderer& itemRenderer, float scale, int screenW, int screenH) {
    float slotSize = 60 * scale;
    float padding = 10 * scale;

    auto drawSlot = [&](int index, float x, float y, bool isHotbar) {
        Rectangle slotRect = {x, y, slotSize, slotSize};

        // Draw slot background
        Color bgColor = (index == m_heldSlotIndex) ? Fade(YELLOW, 0.3f) : Fade(BLACK, 0.7f);
        DrawRectangleRec(slotRect, bgColor);

        if (index == m_activeHotbarSlot) {
            DrawRectangleLinesEx(slotRect, 4.0f * scale, WHITE);
        } else {
            DrawRectangleLinesEx(slotRect, 2.0f * scale, GRAY);
        }

        // Mouse Interaction
        if (CheckCollisionPointRec(GetMousePosition(), slotRect)) {
            DrawRectangleLinesEx(slotRect, 3.0f * scale, WHITE);

            if (m_inventoryOpen) {
                if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
                    if (m_heldSlotIndex == -1) { // Pick up item
                        if (player.getInventory()[index].type != ItemType::NONE) {
                            m_heldSlotIndex = index;
                        }
                    } else { // Swap or place item
                        player.swapSlots(m_heldSlotIndex, index);
                        m_heldSlotIndex = -1;
                    }
                } else if (IsMouseButtonPressed(MOUSE_RIGHT_BUTTON)) {
                    // Consume item on right click
                    if (m_heldSlotIndex == -1) {
                        player.consumeItem(index);
                    }
                }
            }
        }

        // Draw item icon using the renderer
        auto slot = player.getInventory()[index];
        if (slot.type != ItemType::NONE && index != m_heldSlotIndex) {
            Rectangle destRect = {x + padding, y + padding, slotSize - 2 * padding, slotSize - 2 * padding};
            itemRenderer.renderItemUI(slot.type, destRect, WHITE);

            if (slot.count > 1) {
                DrawText(TextFormat("%d", slot.count), x + slotSize - 20 * scale,
                         y + slotSize - 20 * scale, 20 * scale, WHITE);
            }
        }

        if (isHotbar) {
            DrawText(TextFormat("%d", index + 1), x + 5 * scale, y + 5 * scale, 15 * scale, LIGHTGRAY);
        }
    };

    // 1. Draw Hotbar (Slots 0-4)
    float hotbarTotalWidth = (5 * slotSize) + (4 * padding);
    float startX = (screenW - hotbarTotalWidth) / 2.0f;
    float startY = screenH - slotSize - (20 * scale);

    for (int i = 0; i < 5; ++i) {
        drawSlot(i, startX + i * (slotSize + padding), startY, true);
    }

    // 2. Draw Bag (Slots 5-19)
    if (m_inventoryOpen) {
        DrawRectangle(0, 0, screenW, screenH, Fade(BLACK, 0.75f));

        float bagStartX = (screenW - hotbarTotalWidth) / 2.0f;
        float bagStartY = startY - (3 * (slotSize + padding)) - (20 * scale);

        for (int row = 0; row < 3; ++row) {
            for (int col = 0; col < 5; ++col) {
                int index = 5 + (row * 5) + col;
                drawSlot(index, bagStartX + col * (slotSize + padding), bagStartY + row * (slotSize + padding), false);
            }
        }

        if (m_heldSlotIndex != -1) {
            Vector2 mousePos = GetMousePosition();
            auto slot = player.getInventory()[m_heldSlotIndex];

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
    }
}

void UIManager::renderDebugUI(Player& player, Maze& maze, float scale) {
    ImGui::Begin("Debug Engine");
    ImGui::Text("FPS: %d", GetFPS());
    ImGui::Separator();
    ImGui::Text("Maze Statistics");
    ImGui::Text("Number of Rooms: %zu", maze.getRooms().size());

    int totalCells = maze.getWidth() * maze.getHeight();
    float coveragePercent = ((float)maze.getNonWallCount() / totalCells) * 100.0f;
    float corridorPercent = ((float)maze.getCorridorCount() / totalCells) * 100.0f;

    ImGui::Text("Maze Coverage: %.1f%%", coveragePercent);
    ImGui::Text("Corridor Coverage: %.1f%%", corridorPercent);

    ImGui::Separator();
    ImGui::Checkbox("Flashlight Torch Mode", &m_flashlightEnabled);
    if (ImGui::Checkbox("Show Regeneration Zones", &m_showGenerationZones)) {
        m_minimapDirty = true;
    }
    if (ImGui::Checkbox("Show Radiation Zones", &m_showRadiationOnMinimap)) {
        m_minimapDirty = true;
    }

    ImGui::Separator();
    ImGui::Text("View Settings");
    if (ImGui::Button(IsWindowFullscreen() ? "Exit Fullscreen (F11)" : "Enter Fullscreen (F11)")) {
        ToggleFullscreen();
    }
    ImGui::SliderFloat("Tile Zoom", &m_cameraZoom, 0.5f, 5.0f);

    if (ImGui::Button("Regenerate Tic-Tac-Toe Zones")) {
        m_triggerTicTacToeRegen = true;
    }

    ImGui::Separator();
    ImGui::Text("Flashlight Tweaks");
    if (ImGui::SliderFloat("Degree Cut", &m_lightConeAngle, 90.0f, 360.0f)) m_lightSettingsChanged = true;
    if (ImGui::SliderFloat("Circle Size", &m_lightSizeScale, 1.0f, 6.0f)) m_lightSettingsChanged = true;
    if (ImGui::SliderFloat("Angular Fade Strength", &m_lightFadeStrength, 0.1f, 10.0f)) m_lightSettingsChanged = true;

    ImGui::Separator();
    ImGui::Text("Minimap");

    float availWidth = ImGui::GetContentRegionAvail().x;
    float mapRatio = (float)maze.getHeight() / (float)maze.getWidth();
    Vector2 mapDisplaySize = {availWidth, availWidth * mapRatio};

    ImVec2 mapScreenPos = ImGui::GetCursorScreenPos();

    rlImGuiImageRect(&m_minimapTexture.texture, (int)mapDisplaySize.x, (int)mapDisplaySize.y,
                     Rectangle{0, 0, (float)m_minimapTexture.texture.width, -(float)m_minimapTexture.texture.height});

    // Draw Player Dot on Minimap
    Vector2 pPos = player.getPosition();
    int gridX = (int)std::floor(pPos.x / maze.getCellSize());
    int gridY = (int)std::floor(pPos.y / maze.getCellSize());

    int wrappedX = (gridX % maze.getWidth() + maze.getWidth()) % maze.getWidth();
    int wrappedY = (gridY % maze.getHeight() + maze.getHeight()) % maze.getHeight();

    float percentX = (float)wrappedX / maze.getWidth();
    float percentY = (float)wrappedY / maze.getHeight();

    ImVec2 playerScreenPos = ImVec2(mapScreenPos.x + (percentX * mapDisplaySize.x),
                                    mapScreenPos.y + (percentY * mapDisplaySize.y));

    ImGui::GetWindowDrawList()->AddCircleFilled(playerScreenPos, 3.0f, IM_COL32(255, 0, 0, 255));
    ImGui::End();
}

void UIManager::generateMinimap(Maze& maze) {
    BeginTextureMode(m_minimapTexture);
    ClearBackground(BLANK);
    for (int y = 0; y < maze.getHeight(); ++y) {
        for (int x = 0; x < maze.getWidth(); ++x) {
            if (maze.getCell(x, y) == Maze::CELL_WALL) {
                DrawPixel(x, y, Color{100, 100, 100, 255});
            } else {
                if (m_showRadiationOnMinimap && maze.getRadiationLevel(x, y) > 0) {
                    DrawPixel(x, y, Color{0, 255, 0, 255});
                } else if (m_showGenerationZones && maze.isShiftingZone(x, y)) {
                    DrawPixel(x, y, Color{255, 100, 100, 255});
                } else {
                    DrawPixel(x, y, Color{30, 30, 35, 255});
                }
            }
        }
    }

    if (m_showRadiationOnMinimap) {
        for (int y = 0; y < maze.getHeight(); ++y) {
            for (int x = 0; x < maze.getWidth(); ++x) {
                if (maze.getItem(x, y) == ItemType::TOXIC_WASTE) {
                    DrawRectangle(x - 1, y - 1, 3, 3, BLUE);
                }
            }
        }
    }

    EndTextureMode();
}
