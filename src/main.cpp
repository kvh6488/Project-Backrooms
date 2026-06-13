#include "raylib.h"
#include "rlImGui.h"
#include "imgui.h"
#include "maze.hpp"
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

    // Initialize the Maze (width: 40, height: 22, cellSize: 32)
    // 40 * 32 = 1280, 22 * 32 = 704
    Maze maze(40, 22, 32);
    
    // Quick test: carve a little floor space in the top left
    maze.setCell(1, 1, Maze::CELL_FLOOR);
    maze.setCell(2, 1, Maze::CELL_FLOOR);
    maze.setCell(3, 1, Maze::CELL_ROOM);

    // 2. Main Game Loop
    while (!WindowShouldClose()) { // Detect window close button or ESC key
        
        // --- Update Logic ---
        // (Nothing here yet, we will add maze logic later)

        // --- Draw Logic ---
        BeginDrawing();
        
        // Clear the screen to a dark horror-esque grey
        ClearBackground(Color{ 20, 20, 25, 255 });

        // Draw the maze first (so UI draws over it)
        maze.render();

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
