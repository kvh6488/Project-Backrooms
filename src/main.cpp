#include "generators/bsp_generator.hpp"
#include "generators/loop_generator.hpp"
#include "generators/prims_generator.hpp"
#include "generators/tunnel_borer.hpp"
#include "imgui.h"
#include "maze.hpp"
#include "player.hpp"
#include "raylib.h"
#include "rlImGui.h"
#include <ctime>
int main() {
  // 1. Initialization
  const int screenWidth = 1280;
  const int screenHeight = 720;

  // Set some window flags (make it resizable)
  SetConfigFlags(FLAG_WINDOW_RESIZABLE);
  InitWindow(screenWidth, screenHeight, "Project Backrooms");
  SetTargetFPS(60);

  // Initialize the ImGui bridge
  rlImGuiSetup(true);

  unsigned int seed = std::time(nullptr);
  Maze maze(40, 22, 32, seed);
  std::mt19937 rng(seed);

  BSPGenerator bsp;
  bsp.generate(maze, rng);

  PrimsGenerator prims;
  prims.generate(maze, rng);

  LoopGenerator loops;
  loops.generate(maze, rng);

  TunnelBorer borer;
  borer.ensureConnectivity(maze);

  // Spawn player in the center of the first generated room
  Vector2 playerStartPos = {0.0f, 0.0f};
  if (!maze.getRooms().empty()) {
    const auto &firstRoom = maze.getRooms()[0];
    playerStartPos.x =
        (firstRoom.x + firstRoom.width / 2.0f) * maze.getCellSize();
    playerStartPos.y =
        (firstRoom.y + firstRoom.height / 2.0f) * maze.getCellSize();
  }
  Player player(playerStartPos);

  // 2. Main Game Loop
  while (!WindowShouldClose()) { // Detect window close button or ESC key

    // --- Update Logic ---
    player.update(maze);

    // --- Draw Logic ---
    BeginDrawing();

    // Clear the screen to a dark horror-esque grey
    ClearBackground(Color{20, 20, 25, 255});

    // Draw the maze first (so UI draws over it)
    maze.render();

    // Draw the player
    player.render();

    // Start drawing the ImGui user interface
    rlImGuiBegin();

    // Create a simple debug window
    ImGui::Begin("Debug Engine");
    ImGui::Text("FPS: %d", GetFPS());
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
