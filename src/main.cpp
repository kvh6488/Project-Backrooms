#include "raylib.h"
#include "rlImGui.h"
#include "imgui.h"

int main() {
    // 1. Initialization
    const int screenWidth = 1280;
    const int screenHeight = 720;

    // Set some window flags (make it resizable)
    SetConfigFlags(FLAG_WINDOW_RESIZABLE);
    InitWindow(screenWidth, screenHeight, "Project Backrooms - Phase 0");
    SetTargetFPS(60);
    
    // Initialize the ImGui bridge
    rlImGuiSetup(true);

    // 2. Main Game Loop
    while (!WindowShouldClose()) { // Detect window close button or ESC key
        
        // --- Update Logic ---
        // (Nothing here yet, we will add maze logic later)

        // --- Draw Logic ---
        BeginDrawing();
        
        // Clear the screen to a dark horror-esque grey
        ClearBackground(Color{ 20, 20, 25, 255 });

        // Start drawing the ImGui user interface
        rlImGuiBegin();

        // Create a simple debug window
        ImGui::Begin("Debug Engine");
        ImGui::Text("FPS: %d", GetFPS());
        ImGui::Text("Phase 0: Scaffold Complete!");
        ImGui::Spacing();
        ImGui::TextColored(ImVec4(0.0f, 1.0f, 0.0f, 1.0f), "System is ready for Phase 1.");
        ImGui::End();

        // End ImGui drawing
        rlImGuiEnd();

        EndDrawing();
    }

    // 3. De-Initialization
    rlImGuiShutdown();
    CloseWindow();

    return 0;
}
