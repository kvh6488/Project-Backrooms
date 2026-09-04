# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project

Project Backrooms is a 2D top-down psychological horror maze game with a custom C++20 engine on Raylib 5.0 + Dear ImGui (via rlImGui). Single-player, no networking, no scripting layer — everything is C++ compiled into one executable.

Design intent lives in `docs/`: [roadmap.md](docs/roadmap.md) is the authoritative phased plan (Phases 0–3 complete, Phase 4 next) and [the_wilderness_update.md](docs/the_wilderness_update.md) is a newer design doc layering an overworld survival hub on top of the maze. Read the roadmap before adding a gameplay system — most features are already specced there.

## Build & Test

The `build/` directory is configured for **MinGW Makefiles** with g++ at `C:/ProgramData/mingw64/mingw64/bin/g++.exe` (Debug). All dependencies are pulled by `FetchContent` at configure time, so the first configure needs network access.

**Always name the generator and the toolchain explicitly.** The `cmake` first on `PATH` is often Visual Studio's own (`Common7/IDE/CommonExtensions/.../cmake.exe`), which defaults to the Visual Studio generator and silently corrupts the build tree — see "Generator corruption" below.

```bash
cmake -S . -B build -G "MinGW Makefiles" -DCMAKE_MAKE_PROGRAM=C:/ProgramData/mingw64/mingw64/bin/mingw32-make.exe -DCMAKE_C_COMPILER=C:/ProgramData/mingw64/mingw64/bin/gcc.exe -DCMAKE_CXX_COMPILER=C:/ProgramData/mingw64/mingw64/bin/g++.exe -DCMAKE_BUILD_TYPE=Debug
```

The short form `cmake -S . -B build -G "MinGW Makefiles"` only works when a MinGW-aware `cmake` resolves first.

```bash
cmake --build build -j
```

Run the game — **must run with `build/` as the working directory**, because assets are loaded via relative paths like `assets/magic_trip.fs`:

```bash
cd build && ./Backrooms.exe
```

Tests (Google Test, registered with CTest via `gtest_discover_tests`):

```bash
./build/BackroomsTests.exe
```

A single test or suite:

```bash
./build/BackroomsTests.exe --gtest_filter=MazeTest.ToroidalWrapping
```

### Build gotchas

- **Generator corruption (the most common breakage).** There is no `CMakePresets.json` and no `.vscode/settings.json`, so nothing pins the generator. When VS Code's CMake Tools (or Visual Studio, or any bare `cmake -S . -B build`) reconfigures using the VS-bundled cmake, it rewrites the top-level `build/CMakeCache.txt` to `Visual Studio <N>` while every FetchContent sub-build under `build/_deps/*-subbuild/` keeps its `MinGW Makefiles` cache. A generator is immutable once written to a cache, so the nested raylib configure aborts with *"Does not match the generator used previously"*, the whole configure dies before emitting any `.vcxproj`, and the next build fails with `MSBUILD : error MSB1009: Project file does not exist. Switch: ALL_BUILD.vcxproj`. Recover by deleting **only** the top-level cache and re-running the full configure command above:
  ```bash
  rm -rf "build/CMakeCache.txt" "build/CMakeFiles"
  ```
  Leave `build/_deps/` in place — its sub-build caches are already MinGW and its downloaded sources (~150 MB of raylib/ImGui/rlImGui/googletest) are reused, so nothing re-downloads.
- **Game sources are listed once, in the `BACKROOMS_GAME_SOURCES` variable**, and shared by both `add_executable` calls (`Backrooms` adds `main.cpp`, `BackroomsTests` adds the `tests/*.cpp`). Adding a new `.cpp` is one edit to that variable. Header-only additions need no edit at all — which is why `debug_log.hpp` and `asset_load.hpp` are headers.
- **Assets are copied at configure time** (`file(COPY assets DESTINATION ${CMAKE_CURRENT_BINARY_DIR})`), not at build time. After editing or adding anything in `assets/`, re-run the cmake configure step — a plain `cmake --build` will not refresh `build/assets/`.
- `target_compile_definitions(... IsTextureValid=IsTextureReady)` exists because rlImGui's `main` branch expects a Raylib API name newer than the pinned 5.0. Do not remove it while raylib stays at 5.0.
- rlImGui is fetched from `main` and ImGui from the `docking` branch head — neither is pinned, so an upstream change can break the build without any local edit.

## Architecture

### Ownership chain

`main.cpp` → `Application` (owns the Raylib window, the `UIManager`, and a `unique_ptr<GameState>`; its `run()` is the frame loop) → `PlayingState` (the only concrete `GameState` today) which owns the `Maze`, `Player`, `Camera2D`, all three renderers, the `ItemSpawner`, the seed and the shared `std::mt19937`.

`GameState` (`src/states/game_state.hpp`) is the extension point for future states (main menu, death screen). `Application` has no game logic — everything gameplay-side belongs in a state.

### Maze: one flat array, toroidal, with parallel layers

`Maze` (`src/world/maze.hpp`) is an implicit grid graph in a 1D `std::vector<int>`; neighbours are implied by geometry, never stored.

**`getIndex(x, y)` wraps toroidally** — `(x % w + w) % w`. There is no bounds checking anywhere, and out-of-range coordinates silently wrap to the far edge rather than returning a wall. Code that walks off the grid produces wrong-but-valid results, not crashes. Keep this in mind for any neighbour scan near an edge.

Several vectors are indexed identically and must stay the same length: `m_grid` (cell type), `m_visible`, `m_lightLevel`, `m_radiationMap`, `m_items`. Sparser per-cell data uses `std::map<int, T>` keyed by the same 1D index (`m_itemStates`, `m_cupboardInventories`).

`m_items` is the **single source of truth for placed items** — there are no per-item-type lists. `setCell` maintains `m_nonWallCount` / `m_corridorCount` incrementally, so the stat getters are O(1); write cells through `setCell`, not by touching `m_grid`.

Cell types are `CELL_WALL` / `CELL_CORRIDOR` / `CELL_ROOM`, and the corridor/room split drives real behaviour: `AreaState::ROOM` means fully lit (BFS flood), `AreaState::CORRIDOR` means the renderer draws everything and hides it with a screen-space light mask instead of an FOV check. The player switches between them through explicit door transitions (`K`/`L`), not by walking.

### Generation pipeline

Run in this order in `PlayingState::onEnter()`, all sharing one seeded `std::mt19937` so a seed fully determines the world:

1. `BSPGenerator` — recursive space partition, carves rooms, merges adjacent ones, returns a middle-room index used as both the Prim's start and the player spawn.
2. `PrimsGenerator` — grows the corridor web through the space between rooms.
3. `LoopGenerator` — smashes walls to break the perfect maze into loops.
4. `TunnelBorer` — BFS-based connectivity guarantee; bores tunnels to any room Prim's missed.
5. `PrimsGenerator::pruneSmallAlcoves`.
6. `ItemSpawner::spawnInitialItems`.

Each generator exposes both `generate()` (whole maze) and `generateZone(startX, startY, w, h)` (a rectangle). The zone variants exist for the "Tic-Tac-Toe" shifting-zone regeneration in `PlayingState::regenerateTicTacToeZones()`, which clears eight strips of the maze, re-runs the pipeline inside them, and asks the `ItemSpawner` to replenish exactly what `Maze::clearItemsInZone` reported destroyed. **Those zone rectangles are hardcoded to the 250×150 maze size** set in the `PlayingState` constructor; changing maze dimensions requires updating them.

Doorway punching is shared in `GeneratorUtils::punchDoorways` and enforces one door per room (`Maze::isValidDoorPlacement`).

### Rendering

Data and presentation are strictly separated: `MazeRenderer` (terrain), `PlayerRenderer` (sprite + animation timer), `ItemRenderer` (world items *and* inventory icons). Renderers own their textures; `loadTextures()` is called from `PlayingState::onEnter`.

`PlayingState::render()` has a fixed, order-sensitive pipeline:

1. `buildLightMask(...)` **before** `BeginTextureMode` — it uses its own render texture.
2. Scene into `m_screenTarget`: maze → **player → items**. Items draw after the player deliberately, so tall furniture occludes the sprite.
3. `drawLightMask()` (corridors only), then the radiation darkness rectangle, still inside the render texture.
4. `EndTextureMode`, then blit `m_screenTarget` to the screen, wrapped in `m_tripShader` when `Player::getMushroomEffectStrength() > 0`.
5. The magic-book overlay and all `UIManager` output draw *after* `EndShaderMode` — they are intentionally exempt from the trip distortion. `DebugOverlay::render` goes last of all, because ImGui must own the final draw of the frame.

`m_screenTarget` is reallocated whenever the window size changes; anything else caching screen-sized textures needs the same check.

### Items, crafting, spawning

`ItemType` (`src/items/item.hpp`) is one unified enum for both grid interactables and inventory items. Behaviour is data, not switch statements: `ItemDatabase` is a static registry of `ItemDefinition` (`isPlaceable` / `isPickable` / `isConsumable`, stack size, UI sprite rect) — query `ItemDatabase::getDef(type)` rather than special-casing an enum value. `CraftingSystem` is the equivalent static registry of `Recipe`.

Both need `ItemDatabase::init()` and `CraftingSystem::init()` before use; these are called in the `Application` constructor, so any test or tool that does not build an `Application` must call them itself.

`ItemSpawner` owns *all* placement rules (barrels avoid doorways, mushrooms grow in clumps in radiated room corners, cupboards hug walls, tables need two adjacent room tiles). Adding an item type means adding a spawn method there — `Maze` and the renderers only need to recognise the new enum value.

### Player ↔ UI communication

`Player` never touches the UI. It sets one-shot boolean flags which `PlayingState` drains each frame via the `pollEventX()` methods (`pollEventMushroomConsumed`, `pollEventMapCrafted`, …) — each poll returns the flag and clears it — and translates them into `UIManager::showPopup(text, PopupType, duration)` calls.

`UIManager` is a state holder and mailbox, not a caller: it owns inventory/cupboard/map-overlay open state and the popup queue.

Its one write path into the game is `handleInventoryInput(Player&, Maze&)`, called from `PlayingState::handleInput` — the input phase, before any drawing. `UIManager::render` and `renderInventory` are read-only with respect to the `Player` and the `Maze`; they only write hover bookkeeping for the tooltip. Both passes take their geometry from `InventoryLayout::compute`, so a click is hit-tested against exactly the rectangle that gets drawn. Keep that direction: new UI widgets resolve their clicks in `handleInventoryInput` and add their rectangle to `InventoryLayout`, never mid-draw.

`DebugOverlay` (`src/core/debug_overlay.hpp`) is the development panel, split out of `UIManager` and owned by `Application` so it survives future state switches. It is a *view*: presentation values the game needs regardless (torch on/off, camera zoom, the three light-cone numbers, show-zones) live in `RenderSettings`, which `PlayingState` owns and the overlay edits by reference; only debug-only state (the god-view minimap texture, magic-book and trip forcing, status strings) belongs to the overlay. It uses the same mailbox convention — `PlayingState` reads and clears `triggerTicTacToeRegen`, `triggerMagicBookSpawn`, `triggerForceTrip`, `triggerEndTrip`. Debug buttons must call the same public entry points the real systems will use, so they keep exercising the shipping path.

Gating is runtime only: `Backrooms.exe --dev` arms the panel (`src/core/dev_mode.hpp`) and `F1` shows/hides it. The code still ships inside the binary — a release build should add a compile-time flag around `debug_overlay.cpp` as well.

Cached render textures (`DebugOverlay::m_mapTexture`, `UIManager::m_magicBookMapTexture`, per-instance drawn maps) are regenerated only when marked dirty. Any code that changes maze layout must call **both** `DebugOverlay::markMapDirty()` and `UIManager::markMagicBookMapDirty()`.

### Input

All keybindings are read directly with Raylib polling — there is no input abstraction. Movement and door/pickup keys are in `Player::update`; everything else is in `PlayingState::handleInput`.

WASD/arrows move · `K`/`L` door 1 / door 2 · `P` pick up · `I` inventory · `O` open focused cupboard · `U` use/consume (or close fullscreen map) · `Q` enter placement mode, then left-click a visible floor tile · `1`–`5` hotbar · `F11` fullscreen · `F1` debug panel (only with `--dev`).

Command line: `--seed <name|number>` pins the world (`src/core/debug_seeds.hpp`), `--dev` arms the debug panel.

## Tests

`tests/test_maze.cpp` is currently the only test file — it covers maze indexing, toroidal wrapping, and generator invariants (rooms carved, connectivity) using fixed seeds. The test target links the whole game including Raylib and ImGui, so tests can construct real game objects, but must not open a window.

## Conventions

- Members are `m_camelCase`; classes are `PascalCase`; files are `snake_case.cpp/.hpp`.
- Includes are project-root relative (`#include "world/maze.hpp"`) — `src` is on the include path.
- Headers carry banner comments (`// ==== ClassName ====`) explaining the algorithm and its complexity. This is a portfolio project; new systems are expected to document the CS concept behind them the same way.
