#include "generators/bsp_generator.hpp"
#include "generators/loop_generator.hpp"
#include "generators/prims_generator.hpp"
#include "generators/tunnel_borer.hpp"
#include "imgui.h"
#include "maze.hpp"
#include "player.hpp"
#include "raylib.h"
#include "rlImGui.h"
#include <iostream>
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
  std::cout << "[INFO] Initializing Maze with seed: " << seed << std::endl;

  // Phase 1: Massive Scale.
  // We're increasing the maze from (40, 22) to (250, 150) so it doesn't fit on
  // one screen anymore!
  Maze maze(250, 150, 32, seed);
  std::mt19937 rng(seed);

  BSPGenerator bsp;
  bsp.generate(maze, rng);

  int midRoomIdx = bsp.getMiddleRoomIndex();

  PrimsGenerator prims;
  prims.generate(maze, rng, midRoomIdx);

  LoopGenerator loops;
  loops.generate(maze, rng);

  TunnelBorer borer;
  borer.ensureConnectivity(maze);

  // Spawn player in the center of the middle room
  Vector2 playerStartPos = {0.0f, 0.0f};
  if (!maze.getRooms().empty() && midRoomIdx >= 0) {
    const auto& closestRoom = maze.getRooms()[midRoomIdx];
    playerStartPos.x = (closestRoom.x + closestRoom.width / 2.0f) * maze.getCellSize();
    playerStartPos.y = (closestRoom.y + closestRoom.height / 2.0f) * maze.getCellSize();
  }
  Player player(playerStartPos);

  // --- Calculate Maze Statistics ---
  int totalRooms = maze.getRooms().size();
  int totalCells = maze.getWidth() * maze.getHeight();
  int nonWallCells = maze.getNonWallCount();
  float coveragePercent = ((float)nonWallCells / totalCells) * 100.0f;

  // --- Camera Initialization ---
  Camera2D camera = {0};
  // The target is what the camera is pointing at (World Space). We'll update
  // this every frame to follow the player.
  camera.target = player.getPosition();
  // The offset is where we want the target to appear on the screen (Screen
  // Space). We want it dead center!
  camera.offset = Vector2{screenWidth / 2.0f, screenHeight / 2.0f};
  camera.rotation = 0.0f;
  camera.zoom = 1.0f; // 1x scale

  // --- Minimap Framebuffer Initialization ---
  // Create an invisible texture in GPU memory to hold our static minimap (1
  // pixel per cell)
  RenderTexture2D minimapTexture =
      LoadRenderTexture(maze.getWidth(), maze.getHeight());

  // Instruct the GPU to draw to our invisible texture instead of the screen
  BeginTextureMode(minimapTexture);
  ClearBackground(BLANK); // Transparent background

  // Draw the entire maze once. This takes O(V) time but we only do it here at
  // startup!
  for (int y = 0; y < maze.getHeight(); ++y) {
    for (int x = 0; x < maze.getWidth(); ++x) {
      if (maze.getCell(x, y) == Maze::CELL_WALL) {
        // Draw a single 1x1 pixel for the wall
        DrawPixel(x, y, Color{100, 100, 100, 255});
      } else {
        DrawPixel(x, y, Color{30, 30, 35, 255});
      }
    }
  }
  // Tell the GPU we are done drawing to the texture, go back to normal screen
  // drawing
  EndTextureMode();

  // 2. Main Game Loop
  while (!WindowShouldClose()) { // Detect window close button or ESC key

    // --- Update Logic ---
    player.update(maze);

    // Update the camera to perfectly track the player's world position
    camera.target = player.getPosition();

    // --- Draw Logic ---
    BeginDrawing();

    // Clear the screen to a dark horror-esque grey
    ClearBackground(Color{20, 20, 25, 255});

    // --- BEGIN 2D CAMERA MATRIX ---
    // Tell GPU to multiply everything drawn from here by our camera's
    // transformation matrix!
    BeginMode2D(camera);

    // Draw the maze first (so UI draws over it)
    maze.render();

    // Draw the player
    player.render();

    // --- END 2D CAMERA MATRIX ---
    // Stop applying the matrix so our UI can be drawn in standard Screen Space
    // (locked to the window)
    EndMode2D();

    // Start drawing the ImGui user interface
    rlImGuiBegin();

    // Create a simple debug window
    ImGui::Begin("Debug Engine");
    ImGui::Text("FPS: %d", GetFPS());
    ImGui::Separator();
    ImGui::Text("Maze Statistics");
    ImGui::Text("Random Seed: %u", seed);
    ImGui::Text("Number of Rooms: %d", totalRooms);
    ImGui::Text("Maze Coverage: %.1f%%", coveragePercent);

    // --- Minimap Rendering ---
    ImGui::Separator();
    ImGui::Text("Minimap");

    // Calculate a nice fit size for the minimap inside the ImGui window
    float availWidth = ImGui::GetContentRegionAvail().x;
    // Keep the aspect ratio
    float mapRatio = (float)maze.getHeight() / (float)maze.getWidth();
    Vector2 mapDisplaySize = {availWidth, availWidth * mapRatio};

    // We need to know exactly where on the screen ImGui is about to draw the
    // image so we can draw the player dot on top of it later.
    ImVec2 mapScreenPos = ImGui::GetCursorScreenPos();

    // Draw the generated texture as an image in ImGui.
    // We pass a negative height in the source rectangle to flip the OpenGL
    // Y-axis!
    rlImGuiImageRect(&minimapTexture.texture, (int)mapDisplaySize.x,
                     (int)mapDisplaySize.y,
                     Rectangle{0, 0, (float)minimapTexture.texture.width,
                               -(float)minimapTexture.texture.height});

    // Draw the Player Dot
    // 1. Get player's world position
    Vector2 pPos = player.getPosition();
    // 2. Convert world pixels to maze grid coordinates (0 to 250)
    float gridX = pPos.x / maze.getCellSize();
    float gridY = pPos.y / maze.getCellSize();
    // 3. Convert grid coordinates to percentages (0.0 to 1.0)
    float percentX = gridX / maze.getWidth();
    float percentY = gridY / maze.getHeight();
    // 4. Map those percentages to the actual screen space pixels of the ImGui
    // image
    ImVec2 playerScreenPos =
        ImVec2(mapScreenPos.x + (percentX * mapDisplaySize.x),
               mapScreenPos.y + (percentY * mapDisplaySize.y));

    // Ask ImGui to draw a red dot right on top of our image
    ImGui::GetWindowDrawList()->AddCircleFilled(playerScreenPos, 3.0f,
                                                IM_COL32(255, 0, 0, 255));
    ImGui::End();

    // End ImGui drawing
    rlImGuiEnd();
    EndDrawing();
  }

  // 3. De-Initialization
  UnloadRenderTexture(minimapTexture); // Free the GPU memory!
  rlImGuiShutdown();
  CloseWindow();

  return 0;
}
