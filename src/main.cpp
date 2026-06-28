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
#include <iostream>
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

  // unsigned int seed = std::time(nullptr);
  unsigned int seed = 1782631554;
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

  // Post-Prims cleanup: remove tiny corridor clusters (< 5 tiles)
  // that only connect to a single room (useless alcoves).
  // Moved to the very end to catch artifacts from LoopGenerator and
  // TunnelBorer!
  prims.pruneSmallAlcoves(maze, 5);

  // Spawn player in the center of the middle room
  Vector2 playerStartPos = {0.0f, 0.0f};
  if (!maze.getRooms().empty() && midRoomIdx >= 0) {
    const auto &closestRoom = maze.getRooms()[midRoomIdx];
    playerStartPos.x =
        (closestRoom.x + closestRoom.width / 2.0f) * maze.getCellSize();
    playerStartPos.y =
        (closestRoom.y + closestRoom.height / 2.0f) * maze.getCellSize();
  }
  Player player(playerStartPos, AreaState::ROOM);

  // --- Calculate Maze Statistics ---
  int totalRooms = maze.getRooms().size();
  int totalCells = maze.getWidth() * maze.getHeight();
  int nonWallCells = maze.getNonWallCount();
  int corridorCells = maze.getCorridorCount();
  float coveragePercent = ((float)nonWallCells / totalCells) * 100.0f;
  float corridorPercent = ((float)corridorCells / totalCells) * 100.0f;

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

  // --- Minimap Generation Lambda ---
  auto generateMinimap = [&maze, &minimapTexture]() {
    BeginTextureMode(minimapTexture);
    ClearBackground(BLANK);
    for (int y = 0; y < maze.getHeight(); ++y) {
      for (int x = 0; x < maze.getWidth(); ++x) {
        if (maze.getCell(x, y) == Maze::CELL_WALL) {
          DrawPixel(x, y, Color{100, 100, 100, 255});
        } else {
          // If it's the shifting zone, make the minimap red here!
          if (maze.isShiftingZone(x, y)) {
            DrawPixel(x, y, Color{255, 100, 100, 255});
          } else {
            DrawPixel(x, y, Color{30, 30, 35, 255});
          }
        }
      }
    }
    EndTextureMode();
  };

  // Generate initial minimap
  generateMinimap();

  bool minimapDirty = false;

  // 2. Main Game Loop
  while (!WindowShouldClose()) { // Detect window close button or ESC key

    // --- Update Logic ---
    player.update(maze);
    maze.updateFOV(player.getPosition(), player.getAreaState());

    if (minimapDirty) {
      generateMinimap();
      minimapDirty = false;
    }

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

    // Draw the maze based on context
    maze.render(camera, player.getAreaState());

    // Draw the player ON TOP
    player.render();

    // --- END 2D CAMERA MATRIX ---
    // Stop applying the matrix so our UI can be drawn in standard Screen Space
    // (locked to the window)
    EndMode2D();

    // Draw interaction prompt
    if (player.canUseDoor(maze)) {
      const char *msg = "Press 'K' to use door";
      int textWidth = MeasureText(msg, 30);
      DrawText(msg, (screenWidth - textWidth) / 2, screenHeight - 100, 30,
               WHITE);
    }

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
    ImGui::Text("Corridor Coverage: %.1f%%", corridorPercent);

    ImGui::Separator();
    if (ImGui::Button("Regenerate Tic-Tac-Toe Zones")) {
      std::vector<Maze::Room> zones = {
          {55, 0, 14, 150},   // V-Left
          {180, 0, 14, 150},  // V-Right
          {0, 30, 55, 14},    // H-Top-Left
          {69, 30, 111, 14},  // H-Top-Mid
          {194, 30, 56, 14},  // H-Top-Right
          {0, 105, 55, 14},   // H-Bot-Left
          {69, 105, 111, 14}, // H-Bot-Mid
          {194, 105, 56, 14}  // H-Bot-Right
      };

      maze.clearShiftingZones();
      for (const auto &z : zones) {
        maze.addShiftingZone(z.x, z.y, z.width, z.height);
        maze.eraseZone(z.x, z.y, z.width, z.height);
      }

      std::mt19937 rng(time(0));
      BSPGenerator bsp;
      PrimsGenerator prims;
      LoopGenerator loops;

      for (const auto &z : zones) {
        bsp.generateZone(maze, rng, z.x, z.y, z.width, z.height);
      }
      for (const auto &z : zones) {
        prims.generateZone(maze, rng, z.x, z.y, z.width, z.height);
      }

      for (const auto &z : zones) {
        loops.generateZone(maze, rng, z.x, z.y, z.width, z.height);
      }

      TunnelBorer borer;
      borer.ensureConnectivity(maze);

      // Post-Prims cleanup for the regenerated maze
      // Moved to the very end to catch artifacts from LoopGenerator and
      // TunnelBorer!
      prims.pruneSmallAlcoves(maze, 5);

      minimapDirty = true;
    }

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
    // 2. Convert world pixels to maze grid coordinates
    int gridX = (int)floor(pPos.x / maze.getCellSize());
    int gridY = (int)floor(pPos.y / maze.getCellSize());

    // mathematically wrap the coordinates so the dot teleports!
    int wrappedX =
        (gridX % maze.getWidth() + maze.getWidth()) % maze.getWidth();
    int wrappedY =
        (gridY % maze.getHeight() + maze.getHeight()) % maze.getHeight();

    // 3. Convert wrapped grid coordinates to percentages (0.0 to 1.0)
    float percentX = (float)wrappedX / maze.getWidth();
    float percentY = (float)wrappedY / maze.getHeight();
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
