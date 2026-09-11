# Closed-Loop Autonomous Dev Workflow for Project Backrooms

## Plain-English primer (read this first)

Six ideas. None are as clever as the rest of this document makes them sound.

**1. Input virtualization — "stop reading a global, pass a value."**
Today 32 places ask the keyboard directly (`if (IsKeyDown(KEY_W))` at `player.cpp:96`).
You cannot lie to hardware, so no test can say "pretend W is held". Instead, one
function reads the keyboard once per frame and fills in a form (`struct InputState`),
and that form is handed to everything. `player.cpp:96` becomes `if (in.moveUp)`. To
test, hand the code a form you filled in yourself. This is the same pattern as the
existing `RenderSettings`: a plain struct, owned by one thing, passed by reference.

**2. Fixed dt — the game stops caring how fast the computer is.**
`dt` is "seconds since the last frame"; everything that moves multiplies by it. This is
already done correctly — one `GetFrameTime()` at `application.cpp:66`, passed down. The
only problem is that it is a stopwatch tied to real time: run flat out with no window and
frames take 0.0002s, so dt goes tiny and "walk forward 1000 frames" moves 6 pixels
instead of 500. Fix: declare `dt = 1/60` instead of measuring it. Then 1000 frames is
always 16.7 seconds of game time no matter how long they really took. THAT is what makes
fast-forward possible — it is not a speed limit.

**3. Headless mode — play the game with nobody watching.**
Open the window invisible (`FLAG_WINDOW_HIDDEN`), drive it from a text file
(`hold W 45`, `press P`, `checkpoint after_pickup`), and at each checkpoint save a
screenshot plus a small JSON of facts (player position, cell, inventory). An agent reads
the JSON to check the numbers and looks at the PNG to check it looks right.

**4. Fixed scale, variable canvas — never scale by a fraction, and reward a big monitor.**
Today `camera.zoom = 1.2 x window scale`, so a 32px tile is drawn 38.4px wide. One art
pixel cannot cover 1.2 screen pixels, so the GPU blurs and guesses: some pixels come out
fat, some thin, and it shimmers when you move. Fix: draw the world onto a canvas at zoom
1.0 (32px tile = exactly 32px), then stamp that canvas onto the window at a WHOLE number
(1x, 2x, 3x). Never 1.5x. The rule is:

    canvas = window / scale       (scale is a whole number, chosen by the player)

Sprites are therefore always the same size on screen - the view never "zooms out" - while
a bigger window means a bigger canvas, which fits MORE tiles. There are never black bars,
because the canvas is defined as exactly the window divided by the scale. The canvas
already exists (`m_screenTarget`) and already reallocates on resize; it just allocates
`window / scale` instead of `window`.

**5. Pixel density — two things that sound the same.**
  - *World size*: how many tiles does the table cover? (2 tiles)
  - *Pixel density*: how many art pixels sit inside those 2 tiles? (63, or 32?)

  A floor tile is 16 art px drawn 32px, so one art pixel is a 2x2 block. A table is 63
  art px drawn 64px, so one art pixel is 1 screen pixel. Both are "2 tiles wide", but the
  table packs 4x the detail into the same space — that is the visible difference between
  the furniture and everything else. The ruling is to go chunky everywhere: furniture art
  shrinks 63px -> 32px and then draws at 2x. Same size on screen, just chunkier.

**6. Palette — one shared list of colours.**
The sheets came from different artists with different greens and greys. Pick ~46 colours
and remap every sprite to the nearest one, so it reads as one place.

**What is NOT touched:** maze generation (BSP, Prim's, loops, tunnel borer), items,
crafting, inventory rules, UI layout, the light mask, the trip shader. No gameplay moves.
The only genuinely new concept is the input form; everything else is either fixing a
number that is already wrong (the 1.2x zoom, the 1.067x cupboard, `time(0)` in zone
regen) or moving art around.

---

## Context

Every feature and asset in this project is verified by you playing the game by hand. That is the bottleneck: it does not scale, it is not repeatable, and it cannot be delegated to an agent. The goal is a loop where a change can be built, driven through a scripted scenario, and checked against both a screenshot and a machine-readable state dump — with no human at the keyboard.

Three things block that today, and one is a live breakage:

1. **The build tree is corrupted right now.** `build/CMakeCache.txt` says `Visual Studio 18 2026` with no compiler entries and no `.vcxproj` files, while a GNU `Makefile` and MinGW-built `.exe`s from Sep 4 sit beside it. The next `cmake --build build` fails with `MSB1009`. Nothing else can start until this is fixed.
2. **Input and time are read from global hardware state.** 32 raylib input calls across 4 files, and no way to inject a scripted sequence.
3. **There is no output surface.** Zero file I/O in the entire codebase — no screenshots, no telemetry, no JSON. All output is `printf` to stdout.

The audit also surfaced a determinism hole and a render-scale incoherence that would silently poison any harness built on top of them. Both are fixed early, before the harness depends on them.

### Decisions taken (yours, recorded)

| Call | Ruling |
|---|---|
| Input abstraction | POD `InputState` passed to `update()` **and** `render()`; mirrors the `RenderSettings` precedent |
| Tick loop | Fixed 1/60 everywhere; one code path for play and test |
| Camera scaling | **Fixed integer scale, variable canvas**: `canvas = window / scale`. Sprites never change size; a bigger monitor shows more world; no letterboxing ever |
| Base pixel density | **Unify down to 16px/cell** — all art authored at 16px density, drawn at 2× |
| Palette | One shared ramp set; overworld and underworld split by value window + temperature bias |
| PixelLab | **MCP server is live and authenticated** — trial plan, 40 generations, $0 credits. Use the MCP tools directly; no Python SDK, no key handling |

---

## Grid audit (delivered)

### Source density is uniform; draw scale is not

All nine sheets in `assets/` measure as native 1:1 pixel art — 44–83% single-pixel horizontal runs, so none is an upscale of finer art. Every pack you own is authored on a 16px atlas grid (`PostApoc_Workshop/Info.txt` states it outright; Pixel Crawler's tilesets are exactly 25×25 cells of 16px, with characters on 32×32 frames over that same grid). The `16 vs 32` conflict in `docs/the_wilderness_update.md` §19 is therefore **benign**: 16px source → 32px world cell is a clean integer 2×.

The real defect is downstream — props are drawn at roughly half the pixel density of the world around them:

| Sprite | Source rect | Draw scale | Site |
|---|---|---|---|
| Maze tiles | 16×16 | **2×** ✓ | `maze_renderer.cpp:142` |
| Player | 16×16 | **2×** ✓ | `player_renderer.cpp:73` (via hardcoded `32.0f`) |
| Door | 16×29 | **2×** ✓ | `maze_renderer.cpp:218` (`cellSize / 16.0f`) |
| Cupboard | 30×57 / 31×64 | **1.067×** ✗ | `item_renderer.cpp:196` (`cellSize / cupW`) |
| Table | 63×45 / 61×41 / 31×64 / 29×62 | **1.016×** ✗ | `item_renderer.cpp:290,298` |
| Barrel (TOXIC_WASTE) | 22×30 | **1×** ✗ | `item_renderer.cpp:75` (absolute px dest) |
| Radiation doodad | 16×16 | **1×** ✗ | `item_renderer.cpp:83` |
| Mushroom | 16×16 | **1×** ✗ | `item_renderer.cpp:107` (`cellSize / 2.0f`) |
| Magic book | 16×16 | **1.5×** ✗ | `item_renderer.cpp:354` |

World *geometry* is fine — a 63px table drawn 64px wide is 2 cells, the player is 1 cell. It is the pixel density that diverges, which is exactly the "quality difference" you noticed: a furniture pixel is one screen pixel while a terrain pixel is a 2×2 block.

### Ruling: 16px density everywhere, drawn at 2×

**Every asset is authored at 16px per world cell and drawn at exactly 2×.** No non-integer scale survives anywhere in the pipeline.

Fix per asset:

| Asset | Fix | Cost |
|---|---|---|
| Backrooms Lvl1 floor/wall/prop sets | None — already compliant | Zero |
| `inspector_spritesheet.png` (player) | None — already compliant | Zero |
| `mushrooms_pixel_asset.png` | Re-cut dest to 2× (32px, one full cell) | Rect change only |
| `doodads_spritesheet.png` | Re-cut dest to 2× | Rect change only |
| `Spritesheet_TheDarkRitual_BigWander.png` | Drop the 1.5× to 2×; fix `bookSrc.y = 97` → `96` (off-grid, straddles rows 6/7; duplicated at `item_database.cpp:93`) | Rect change only |
| `PostApoc_Workshop_WithShadow.png` | **Downsample 544×448 → 272×224, then touch up.** Re-cut all 8 hand-measured furniture rects to 16px multiples (`63×45`→`32×24`, `30×57`→`16×32`, `22×30`→`16×16`) | **Real work** — pixel-art downsample loses half the detail; needs a manual pass |
| `PostApoc_Workshop_Icons.png` | Downsample 224×224 → 112×112; `PAPER {96,339,14,10}` and `PENCIL {114,305,13,12}` re-cut to 16px cells | Same pass |
| ↳ **fallback if the downsample looks bad** | **Replace rather than downsample.** A 2:1 downsample discards half the pixels; on dense, high-detail furniture it can turn readable objects to mush, and no touch-up recovers detail that was never in the smaller image. Two escape routes, in order of preference: **(a)** re-source natively — `Backrooms Items` has 48 native 16×16 sprites and Pixel Crawler ships `Furniture.png` (800×864), `Dungeon_Props` and `Interior_Props_01`, all 16px-grid; **(b)** regenerate via `/generate-asset` at 16px with the master palette forced and the surviving Backrooms tileset as style reference. Decide per object, not per sheet — a locker may survive the downsample while a tool bench does not | Judgement call at Phase 5; budget for it |
| `FullSpritesheet.png` (Ability Icons) | **Retire from `assets/`** — 72 KB, referenced by nothing. Re-add from the pack when Phase 4's ability HUD lands | Delete |
| `guard_yellow_spritesheet.png` | Keep (window icon only), route through `assets::loadTexture` | Trivial |
| Pixel Crawler (overworld) | None — natively 16px | Zero |
| `Mobs/` pack | **Flag: 32-based sheets (160×192, 320×320).** Cannot be used without a downsample or a redraw. Not currently in `assets/` | Deferred |

**Nothing requires a redraw from scratch.** The workshop sheets need a downsample-and-touch-up, which is the single genuine art cost. Alternative worth considering at Phase 5: source replacement furniture from `Backrooms Items` (48 native 16×16 sprites) and Pixel Crawler `Furniture.png` / `Dungeon_Props` rather than downsampling.

### Render-path sites that must become grid-relative

Introduce `core/grid.hpp`: `SOURCE_TILE = 16`, `WORLD_SCALE = 2`, `CELL = 32`, `PIXEL_SCALE` (player setting, default 1).

- `item_renderer.cpp:74-87` — barrel/doodad absolute dest rects and the `-11.0f` / `-8.0f` hardcoded half-widths
- `item_renderer.cpp:107` — `cellSize / 2.0f`
- `item_renderer.cpp:139-162` — four cupboard rects (note `:139` says y=197 while its own comment says 198; `:160` uses a *different* rect for the same sprite)
- `item_renderer.cpp:266-302` — eight table rects, none on a 16px boundary
- `item_renderer.cpp:354` — `(cellSize / 32.0f) * 1.5f`
- `player_renderer.cpp:73-81` — dest `32.0f` and origin `{16.0f, 16.0f}`
- `maze_renderer.cpp:394` — `3.0f * 32.0f` (uses literal 32, not `cellSize`)
- `maze_renderer.cpp:429` — `28.0f * camera.zoom`
- `playing_state.cpp:171` — `1280` / `720` restated as literals instead of read from `UIManager::getUIScale()`

### Bugs found in passing (fix during Phase 5)

- **`maze_renderer.cpp:196` samples outside its texture.** `wallX` reaches 13 on a 192px-wide (12-column) wall sheet, so `:200` and `:209` read x = 208–224. Live out-of-bounds on the door-in-room wallpaper path.
- **Two light-gradient diameters** — `512` at `maze_renderer.cpp:29`, `250` at `:352`. Touching any flashlight slider silently swaps to the lower-res gradient.
- **Three copies of the flashlight defaults** — `maze_renderer.hpp:49-51` (235/2.0/3.125) vs `render_settings.hpp:18-20` (235/1.5/3.5).
- **Map geometry quadruples duplicated** — `ui_manager.cpp:112-115` vs `:710-713`; `:121-122` vs `:740-748`.

---

## Architecture

### Pillar 1 — `InputState` and the fixed tick

Three new header-only files (header-only is deliberate — the project's stated convention, since a `.cpp` means editing `BACKROOMS_GAME_SOURCES`, shared by both executables):

- **`src/core/input_state.hpp`** — POD struct. Named action booleans (`moveUp`, `door1`, `pickup`, `toggleInventory`, `hotbar[5]`, …) split into `held` and `pressed`, plus `mouse` position, `mouseLeftPressed`, `mouseRightPressed`, and `screenW`/`screenH`. Plus `InputState pollHardware()` — the only place raylib input functions are called in the shipping build.
- **`src/dev/scripted_input.hpp`** — parses a plain-text scenario and yields one `InputState` per tick.
- **`src/dev/scenario.hpp`** — the scenario grammar.

**Write JSON, read plain text.** Telemetry goes out as JSON (trivial to emit by hand); scenarios come in as line-oriented text, so no JSON *parser* is needed and no fourth `FetchContent` dependency is added. Scenario grammar:

```
# comment
seed 1788480606
wait 30
hold W 45
press P
click 640 360
checkpoint after_pickup
end
```

**Signature churn** (mechanical, ~35 call sites):

```cpp
virtual void GameState::update(float dt, const InputState& in) = 0;
virtual void GameState::render(const InputState& in) = 0;
void Player::update(Maze&, float dt, bool canMove, const InputState& in);
void UIManager::handleInventoryInput(Player&, Maze&, const InputState& in);
void UIManager::render(Player&, Maze&, ItemRenderer&, bool, float, const InputState& in);
void PlayingState::handleInput(const InputState& in);
```

`UIManager::render` reading `in` does **not** violate the documented boundary (`ui_manager.hpp:82-83`) — that rule is about not mutating `Player` or `Maze`, which still holds. The six render-phase `GetMousePosition()` calls become field reads.

**Fixed dt is what enables fast-forward — it is not a speed limit.** Two separate clocks:

- *Fixed dt* — each tick advances the **simulation** by exactly 1/60 s of game time.
- *Fast-forward* — running those ticks back-to-back with no **wall-clock** wait.

Today `GetFrameTime()` welds them together: run flat out and dt collapses toward zero, so 1000 ticks advance the sim by ~3 s instead of 16.7 s, and movement, timers and the radiation flicker all behave differently than they do at 60 FPS. Decoupling them lets the harness burn 10,000 ticks as fast as the GPU allows while the simulation sees exactly 166 s of game time — bit-identical to a real playthrough. Headless simply omits `SetTargetFPS`, so nothing throttles the loop.

**`Application::run()` becomes source-driven:**

```cpp
struct RunConfig {
  bool headless = false;
  int maxTicks = -1;              // -1 = until WindowShouldClose
  const char* scenarioPath = nullptr;
  const char* outDir = nullptr;
};

for (int tick = 0; !shouldStop(tick); ++tick) {
  InputState in = m_inputSource->sample(tick);
  if (!m_headless && IsKeyPressed(KEY_F1)) m_debugOverlay.toggle();
  m_currentState->update(kFixedDt, in);   // kFixedDt = 1.0f / 60.0f
  m_currentState->render(in);
  if (m_harness) m_harness->onTick(tick, *m_currentState);
  // …existing transition handling, unchanged
}
```

**Wall-clock leaks that must die** (they would desync under replay):
- `item_renderer.cpp:362` `GetTime()` → pass `totalTime` into `ItemRenderer::render`
- `ui_manager.cpp:376, 599, 600` `GetTime()` → a `m_simTime` accumulator in the existing `UIManager::update(dt)`
- **`playing_state.cpp:679` `std::mt19937 regenRng(time(0))`** → derive from `m_seed` + a regen counter. This is a straight determinism bug today: zone regeneration is not seed-reproducible, so "same seed = same world" is already false after the first regen.

### Pillar 2 — Headless harness

New CLI: `Backrooms.exe --headless <scenario> --out <dir> [--ticks N]`. Parsed by a new header-only `src/dev/headless_mode.hpp`, following the exact pattern of `dev_mode.hpp` and `debug_seeds.hpp`.

Changes in `Application`:
- `SetConfigFlags(FLAG_WINDOW_RESIZABLE | FLAG_WINDOW_HIDDEN)` when headless
- Skip `SetTargetFPS(60)` when headless, so ticks run flat out with no wall-clock wait
- Skip `rlImGuiSetup` when headless (`DebugOverlay::render` already early-returns when hidden, so ImGui never runs — one less nondeterminism source)

**Checkpoint capture** — two images per checkpoint. Once Phase 3 lands the canvas is `window / PIXEL_SCALE`, so a scenario **must pin the window size and scale** (default 1280×720 at ×1) for screenshots to stay byte-diffable across runs:
- `scene.png` — `LoadImageFromTexture(m_screenTarget.texture)` + `ImageFlipVertical` immediately after `EndTextureMode()` (`playing_state.cpp:558`). Pre-shader, pre-UI, pure gameplay.
- `frame.png` — `LoadImageFromScreen()` immediately before `EndDrawing()` (`playing_state.cpp:615`). Includes trip shader, UI and overlays.

**`telemetry.json`** per checkpoint, emitted by a new header-only `src/dev/json_writer.hpp` (~60 lines; no new dependency — raylib's `SaveFileText` is already linked):

```json
{ "tick": 145, "checkpoint": "after_pickup", "seed": 1788480606,
  "player": { "worldPos": [2416.0, 1552.0], "cell": [75, 48],
              "areaState": "ROOM", "facing": "DOWN",
              "mushroomEffect": 0.0 },
  "camera":  { "target": [2416.0, 1552.0], "zoom": 1.0,
               "rect": [1776, 1192, 1280, 720] },   // canvas-space
  "inventory": [ {"slot":0,"type":"MUSHROOM","count":1} ],
  "visibleItems": [ {"cell":[74,48],"type":"TABLE"} ],
  "maze": { "nonWallCount": 18422, "corridorCount": 9106 } }
```

Artifacts land in `artifacts/<scenario>/<checkpoint>/` (add to `.gitignore`). **Scenarios live in `scenarios/` at repo root, not in `assets/`** — they are not loaded by the game at runtime and must not go through the configure-time asset copy.

### Pillar 3 — Fixed scale, variable canvas

The scene renders to `m_screenTarget` at `camera.zoom = 1.0`, so a 32px cell is exactly
32 texels. The target is sized **`window / scale`**, where `scale` is a whole number the
player picks. The blit then covers the window exactly:

```
PIXEL_SCALE   = 1, 2 or 3   (player setting, default 1)
canvasW       = screenW / PIXEL_SCALE     // integer division
canvasH       = screenH / PIXEL_SCALE
blit dest     = { 0, 0, canvasW * PIXEL_SCALE, canvasH * PIXEL_SCALE }
```

Consequences, all intended:

- **Sprites never change size.** At a given scale, a tile is always `32 * scale` screen
  pixels. Resizing the window does not zoom.
- **A bigger monitor shows more world.** Canvas grows with the window, so more tiles fit.
- **No letterboxing, ever.** The canvas is defined as the window divided by the scale, so
  the blit fills it by construction.
- **No fractional scaling, ever.** `scale` is an integer and `zoom` is pinned at 1.0.

Tiles visible across, by monitor and scale:

| Window | ×1 (32px tiles) | ×2 (64px tiles) | ×3 (96px tiles) |
|---|---|---|---|
| 1280×720 (default) | **40** | 20 | 13 |
| 1920×1080 | 60 | **30** | 20 |
| 2560×1440 | 80 | 40 | 26 |
| 3440×1440 (ultrawide) | 107 | 53 | 35 |

For reference the game today shows ~33 tiles across at any window size. `×1` on the
default window (40 tiles) is the closest match and is the default.

**Integer-division slack.** An odd window dimension leaves up to `scale - 1` pixels
uncovered (e.g. 1281 wide at ×2 gives a 640-wide canvas covering 1280). Round the canvas
*up* and let one pixel column bleed off-screen; the alternative is a one-pixel seam.

**This is less work than a fixed canvas, not more.** `m_screenTarget` already reallocates
whenever the window size changes (`playing_state.cpp:526-531`); that logic stays, it just
divides by `PIXEL_SCALE` first.

**Design consequence to revisit later, not now.** An ultrawide player sees 107 tiles of
corridor at once versus 40 on the default window — materially less tension in a horror
maze, since they spot things much earlier. The standard remedies are a vignette or a
maximum-canvas clamp. Flagged, not solved.

**Migration work.** All 24 `GetScreenWidth()`/`GetScreenHeight()` reads must be triaged
into **canvas space** (the scene, `view_bounds.hpp`, `MazeRenderer::buildLightMask` and
`drawLightMask`) versus **window space** (the final blit, `UIManager::getUIScale`, all UI
layout). The light mask is the subtle one: it works in screen space today and must move
to canvas space or the cone will not line up. Harness screenshots are canvas-sized, so a
scenario must pin the window size to stay diffable across runs.

### Pillar 3 — Master palette

A sampler (`tools/palette_sample.py`) reads every PNG in `assets/` plus the four underworld packs and Pixel Crawler, weights colours by pixel count, converts to **OKLab** (perceptually uniform — RGB k-means clusters badly on ramps), and clusters into candidate ramps. Output is a proposal, not a decision.

Target shape — roughly 46 colours:

| Ramp | Steps | Role |
|---|---|---|
| Green | 8 | Vegetation, moss, radiation-adjacent organics |
| Brown | 8 | Wood, earth, leather, rust |
| Grey | 8 | Concrete, metal, stone |
| Light blue | 8 | Water, sky, cold light |
| Neutral | 8 | Near-black → near-white |
| Accents | 6 | Fire, radiation, UI alert, item highlight |

**Two worlds from one ramp set:** the overworld draws steps 2–7 of each ramp with a warm bias and a soft directional sun; the underworld draws steps 0–5 with a cool green-grey bias and hard point-light falloff. Value contrast carries readability; saturated accents stay rare.

**What that costs, honestly.** The overworld gives up its deepest shadows — no true blacks on the surface, so night has to be sold by hue shift and light radius rather than darkness. The underworld gives up its brightest highlights — a torch beam cannot blow out to white, so "bright" underground has to read through contrast against a compressed neighbourhood rather than absolute luminance. Both are survivable and both are, arguably, the correct constraint for the mood each world wants.

Deliverable: `docs/palette.md` (hex list + ramp names + per-world windows) and `artifacts/palette_swatch.png` for you to look at. **You approve the hexes before any asset is quantized.**

### Pillar 4 — The two skills

Both under `.claude/skills/<name>/SKILL.md`. The project has no `.claude/` directory at all today — this is greenfield.

**`/headless-test <feature-or-asset>`**
1. Guard: `grep CMAKE_GENERATOR build/CMakeCache.txt`. If it is not `MinGW Makefiles`, run the documented recovery (`rm -rf build/CMakeCache.txt build/CMakeFiles`, leave `build/_deps/` alone) then the full explicit configure line from `CLAUDE.md`.
2. Build: `cmake --build build -j`.
3. Select an existing scenario from `scenarios/`, or author one against the documented grammar.
4. Run: `cd build && ./Backrooms.exe --headless ../scenarios/<name>.txt --out ../artifacts/<name>/`.
5. Read `telemetry.json`; assert the feature's own invariants (exact numbers, not vibes).
6. Read `scene.png` and `frame.png` with the Read tool and inspect them visually.
7. Report pass/fail, citing the telemetry field and the image that shows it.

**`/generate-asset <sprite description>`**
1. **Search first.** Query a cached manifest of `../Asset packs` (built once by `tools/index_packs.py`: path, dimensions, inferred cell size, dominant palette). Report the top matches and stop if one fits.
2. On a genuine miss only: check `get_balance` first (40-generation trial budget), then generate at 16px via the PixelLab MCP tools — `create_topdown_tileset` for terrain, `create_map_object` for props, `create_character` for mobs — passing the master palette as the forced palette and an existing sheet as the style reference.
3. Quantize to the master palette; verify every pixel is an exact palette member.
4. Verify grid alignment — dimensions are multiples of 16, content bbox lands on cell boundaries.
5. Write to `assets/`.
6. **Remind: `cmake` reconfigure is mandatory.** `file(COPY assets ...)` runs at configure time only; `cmake --build` will not refresh `build/assets/`.

Steps 1 and 3–6 work with no API key. Step 2 needs `PIXELLAB_SECRET` in a gitignored `.env`.

---

## Phased plan

Each phase leaves the game runnable and hand-playable.

### Phase 0 — Repair and pin the build
**Changes:** Recover the corrupted cache. Add `CMakePresets.json` pinning generator, compilers and build type — the absence of this file is precisely why the VS-bundled `cmake` keeps hijacking the tree. Add `artifacts/`, `.env`, `scenarios/*.out` to `.gitignore`.
**Unlocks:** Everything. Nothing else can build.
**Risk:** Low. Worst case `build/_deps/` gets clobbered and ~150 MB re-downloads.
**Verify:** `cmake --build build -j` succeeds; `cd build && ./Backrooms.exe` plays; `./build/BackroomsTests.exe` is green.

### Phase 1 — Determinism prerequisites
**Changes:** Kill the four wall-clock reads (`item_renderer.cpp:362`, `ui_manager.cpp:376/599/600`) and fix `playing_state.cpp:679`'s `time(0)` regen seed. No input changes.
**Unlocks:** "Same seed ⇒ same world" becomes true, which every later phase assumes.
**Risk:** The craft-flash and book-pulse animations change phase slightly. Cosmetic.
**Verify:** New test — run the generation pipeline twice from one seed *including a zone regeneration*, assert identical `m_grid`. This test fails today.

### Phase 2 — `InputState` and the fixed tick
**Changes:** The three new headers; `RunConfig`-driven `run()`; signature churn across `game_state.hpp`, `playing_state`, `player`, `ui_manager`, `application`.
**Unlocks:** Scripted input; `Player::update` becomes unit-testable — `tests/test_inventory.cpp:4-7` explicitly documents that it cannot be called today because it reads the keyboard.
**Risk:** Highest-churn phase. Two ordering hazards to preserve: `handleInput()` runs before `m_player.update()` but the inventory-open gate is read after (`playing_state.cpp:103`), and arrow keys are bound in two places with different gates (`player.cpp:96-103` under `canMove`, `playing_state.cpp:370-384` under `isInventoryOpen`). Also worth fixing here: nothing checks `io.WantCaptureMouse`, so debug-panel clicks currently reach the game too.
**Verify:** Play by hand — every key behaves identically. New test drives `Player::update` with a synthetic `InputState` and asserts movement and pickup.

### Phase 3 — Virtual resolution
**Changes:** `m_screenTarget` sized `window / PIXEL_SCALE`; `camera.zoom` pinned at 1.0; blit at whole-number scale filling the window; the debug `Tile zoom` slider becomes a 1×/2×/3× `PIXEL_SCALE` selector.
**Unlocks:** Genuinely pixel-perfect rendering, and fixed-size diffable screenshots.
**Risk:** The visible-area change on the default window is mild (40 tiles vs ~33), but an ultrawide monitor now shows ~107 — flagged above as a design question for later. All 24 `GetScreenWidth()`/`GetScreenHeight()` reads must be triaged into canvas space versus window space. `MazeRenderer::buildLightMask` and `drawLightMask` are the subtle ones — they operate in screen space and must move to canvas space or the cone will not line up.
**Verify:** Screenshot at the default window and confirm exact 1:1 texels, no resampling, no bars. Resize the window and confirm sprites stay the same size while more tiles appear. Flip `PIXEL_SCALE` to 2 and confirm sprites double and tile count halves, still with no bars.

### Phase 4 — Headless harness ✅ (2026-09-11)
**Changes:** `--headless` flag; `FLAG_WINDOW_HIDDEN`; `json_writer.hpp`; checkpoint capture; a first `scenarios/` set.
**Unlocks:** The whole loop. This is the payoff phase.
**Risk:** `LoadImageFromScreen()` against a hidden window is the one thing I could not verify from here — if the back buffer reads empty, fall back to `scene.png` only, which comes from a render texture and is certain to work.
**Verify:** Run one scenario twice; both `telemetry.json` files and both PNG sets are byte-identical.

**As built.** The game side is `core/capture.hpp`: a `Telemetry` POD that `GameState::snapshot()` fills (a value out, mirroring `InputState` in — no JSON in `states/`) and a four-hook `CaptureSink` (`beginTick` / `onSceneReady` / `onFrameReady` / `endTick`). `main.cpp` builds an `AppConfig` and lends `Application` a `ScriptedInput` and a `HeadlessHarness`; nothing under `states/` includes `dev/`. Five dev headers: `scenario.hpp`, `scripted_input.hpp`, `headless_mode.hpp`, `json_writer.hpp`, `headless_harness.hpp`. Three scenarios: `smoke` (walk, inventory), `corridor` (door → light mask), `pickup` (seed 3, fixture `mushroom_room`: two pickups and a miss, every telemetry prediction in the file's comments holds). Fourteen new tests in `tests/test_harness.cpp`. Verified: `diff -r` of two runs of each scenario is empty; a 187-tick scenario runs in ~0.3 s.

**Found on the way, fixed:**
- `frame.png` from the hidden window **works** — but only after `rlDrawRenderBatchActive()` before `LoadImageFromScreen()`. rlgl batches draws and flushes at `EndDrawing`; `glReadPixels` does not flush, so the first captures had the scene blit but no UI.
- **`GetRandomValue` was a determinism hole Phase 1 missed.** Raylib seeds it from the clock at `InitWindow`; the radiation flicker and the magic-book 1-in-3 roll use it. `SetRandomSeed(m_seed)` in the `Application` constructor closes it. The proper fix later is to route both through the shared `std::mt19937`.
- Grammar deviation from the sketch above, deliberate: `checkpoint` **consumes one idle tick** rather than tagging the next command's tick, so a capture never contains half of the following command. `press KEY` is `hold KEY 1`; held keys fire their "pressed" fields on the first tick only, exactly as `IsKeyDown`/`IsKeyPressed` do. `mouse X Y` moves the persistent cursor without consuming a tick.
- The `radiation` fixture's note is stale post-Phase 1: seed 1788480606 now spawns in a plain room with no items in view. Left as-is; `mushroom_room` is the fixture for item tests.

**Still open:** the `scene.png` of a corridor at the default light settings is nearly black (a faint cone around the player). Whether that matches the windowed game or is a Phase 3 light-mask regression needs an eye on both side by side — it is a presentation question, not a harness one.

### Phase 5 — Grid fixes and prop re-cut ✅ (2026-09-11, pending your eye on the gate)
**Changes:** `core/grid.hpp`; downsample the two workshop sheets with a manual touch-up; re-cut all ~14 off-grid rects; make the 9 hardcoded render sites grid-relative; fix the out-of-bounds `wallX`, the two gradient diameters, and the duplicated flashlight defaults; retire `FullSpritesheet.png`.
**Unlocks:** A single pixel density, which §16.6 names as the prerequisite for the lighting work.
**Risk:** The downsample is the one real art cost and will need your eye. World scale is preserved throughout — sizes do not change, only density. **Gate:** review the downsampled workshop sheets before the re-cut lands; if any object reads as mush, take the re-source or regenerate route above for that object rather than shipping a bad downsample. Budget for this going either way.
**Verify:** Harness screenshots before and after. Telemetry must be **unchanged** (this is a presentation-only change); the images should show uniform 2×2 texel blocks everywhere.

**As built.** `src/core/grid.hpp` holds `SOURCE_TILE = 16`, `WORLD_SCALE = 2`, `CELL = 32` and three helpers: `srcTile(col,row,w,h)` addresses a sheet in tiles, `cellRect` the world in cells, and `destFor(src, x, y)` derives every destination rectangle from its source — the invariant made mechanical, since no draw site can carry a scale of its own any more. `standingOn(src, x, y, cellsWide)` is the one anchoring rule for furniture. All nine render sites now go through these; `ItemRenderer` no longer reads `Maze::getCellSize()` at all. Verified the plan's way: all twelve pre-existing checkpoints have byte-identical `telemetry.json` before and after, 66 tests green, every scenario byte-identical across two runs.

**The workshop downsample** is `tools/downsample_sheet.py`, reproducible from the pack source: box-average each 2×2, snap to the nearest colour already on the sheet (no invented colours), and vote alpha per block between *solid / drop-shadow / clear* — the sheet's shadows are black at a single partial alpha, and flattening them to opaque was the first attempt's mistake (every table underside became a slab). Output is `assets/PostApoc_Workshop_16px.png` (272×224); the 544×448 copy is gone from `assets/`. Two new presentation scenarios and fixtures carry the evidence: `furniture` (seed 1: both horizontal tables, two side-on cupboards) and `barrel` (seed 38: toxic barrel, vertical table). Nothing needed a manual touch-up at 1280×720 ×1.5.

**Deviations from the table above, deliberate:**
- `PostApoc_Workshop_Icons.png` is **not** downsampled. Its `Info.txt` says it outright and the sheet confirms it: 14×14 cells of 16px inventory icons, already the same density as the mushroom and ritual UI icons. The audit conflated it with the furniture sheet.
- `PAPER` and `PENCIL` were 14×10 and 13×12 prop sprites cut from the *furniture* sheet, not the icon sheet; halved they would be 7×5 blobs. Both are re-sourced to the icon sheet: paper → the torn sheet in its "Paper" row, pencil → the twig in its "Wood" row (there is no pencil icon anywhere in the packs — a one-generation `/generate-asset` candidate for Phase 7).
- The `wallX` out-of-bounds read was **unreachable**, not live: `isDoorInRoom && !isDoorWithRoomBelow` cannot hold inside `belowIsVisibleFloor`, because both conditions are "the cell below is visible room". The dead branch is deleted rather than patched.
- Flashlight defaults: `MazeRenderer` now seeds its three values from `RenderSettings{}`, which was already documented as *the* shipping copy. The rendered defaults therefore move from the renderer's private 2.0 / 3.125 to 1.5 / 3.5 — a slightly larger, softer cone, and the first time the debug sliders start on the numbers actually in use.
- `Maze` still takes a `cellSize` (default now `grid::CELL`) because ~40 test call sites pass it; whether a maze should know about pixels at all is a small follow-up, not a Phase 5 change.

**Found on the way:** Phase 4's "still open" corridor screenshot is **not** a light-mask regression. `corridor_walked` (tick 303) lands on a radiation flicker at its ~0.98 peak — the corridor there is radiated, and the mask itself, dumped mid-frame, is correct (247/255 at the player). `in_corridor` two checkpoints earlier shows a normal cone. Closed.

**Gate outcome (your review, same day):** four changes. The magic book goes back to its old 24px — the one sprite off the 2× rule, at a full cell it swallowed its table. Paper and pencil go back to the old prop cut-outs, kept at native 1:1 in a two-cell `workshop_prop_icons.png` (UI icons are slot-stretched in window space, so world density does not apply). The barrel loses its badge altogether: PixelLab at 16px and 32px both produced mush (2 generations spent) and a rasterised 9px trefoil still hid the drum, so the plain workshop barrel is sold as toxic by two additive fluorescent-green `DrawCircleGradient` halos (one behind, one tight over the drum) — the same trick as the magic book's glow. Mushrooms are regenerated smaller: a 0.7× nearest shrink of the pack sheet as the init image, one `create_image_pixflux` pass at strength 300 with the sheet's own palette forced (3 generations spent across two attempts; the higher-strength one won), then hand-trimmed to three columns — six variants per kind. Budget: 5 of my 20 used, 35 remain on the trial. `doodads_spritesheet.png` is retired with the overlay.

### Phase 6 — Master palette ✅ (2026-09-11)
**Changes:** `tools/palette_sample.py`; `docs/palette.md`; swatch sheet; then `tools/quantize.py` applied to `assets/`.
**Unlocks:** Visual coherence, and the banded-lighting and day/night work in §16.6.
**Risk:** Quantization can flatten intentional gradients. Quantize to a copy, compare, then commit.
**Gate:** You approve the hex list and the swatch sheet before anything is quantized.
**Verify:** A conformance checker reports 100% of pixels in `assets/` are exact palette members; harness screenshots before/after side by side.

**As built.** `tools/palette_sample.py` proposes; `tools/quantize.py` applies and `--check`s; `docs/palette.json` is the palette and `docs/palette.md` the reasoning and the maintenance rules. Two deviations from the table above, both from the data: the plan had no yellow ramp while ochre is a third of the shipping pixels, and 8 steps flattened the carpet weave (its shades sit 0.02–0.04 L apart against a 0.08 rung), so the palette is **7 ramps / 56 colours**, the yellow ramp at 12 steps. Ramps are straight lines through OKLab fitted per hue family and snapped to real sheet colours, so no hex is invented. Verified the plan's way: `--check assets/` at 100 %, telemetry byte-identical across all four scenarios before and after, 66 tests green. The one honest cost was named in the plan and measured here: about 12 % of shipping pixels move more than 0.10 ΔE, all of them the workshop's saturated oranges and the guards' uniforms, which is the coherence the palette exists to buy.

### Phase 7 — The two skills
**Changes:** `.claude/skills/headless-test/SKILL.md`, `.claude/skills/generate-asset/SKILL.md`, `tools/index_packs.py`, `tools/gen_asset.py`.
**Unlocks:** Repeatability by an agent with no memory of this session.
**Risk:** Only 40 trial generations exist, so the skill must be tested against a sprite that is already in the packs (exercising the search path) before it is ever allowed to generate.
**Verify:** Run `/headless-test` on a feature you know works and one you know is broken; confirm it distinguishes them. Run `/generate-asset` for a sprite that *does* exist in the packs and confirm it finds it instead of generating.

---

## Open items

- **PixelLab MCP is live and authenticated.** Confirmed via `get_balance`: **trial plan, 40 generations remaining, $0.00 credits.** 80+ tools are available, including `create_topdown_tileset`, `create_map_object`, `create_character`, `image_to_pixelart`, and `reduce_colors` (server-side palette quantizing — this can do the master-palette conformance step for us). **No Python SDK, no `.env`, no key handling: drop `tools/gen_asset.py` and have `/generate-asset` call the MCP tools directly.**
- **40 generations is a hard budget, split 20/20.** At trial there are no credits to fall back on, so a wasted generation is gone.
  - **I may spend at most 20** across all development and testing of `/generate-asset`. If I approach that ceiling I stop and report,
    rather than spending into your half.
  - **The remaining 20 are yours** to test the skill yourself once I hand it over.
  - **If it works well, you upgrade the plan** and the budget stops being a constraint. Until then it is the binding one.
  - This makes the search-first gate load-bearing rather than merely tidy: `../Asset packs` holds ~1,500 sprites across 9 packs, and
    almost every underworld prop we need is already in one of them. `/generate-asset` calls `get_balance` before every run and refuses
    to generate without first reporting the best pack matches it found.
- **Ultrawide is a feature, not a problem.** Wider monitors reveal more of the maze sides, but the torch/light mask already hides almost all of it, so the gameplay impact is small. Settled; no vignette or clamp needed.
- **`Mobs/` pack is 32-based** and cannot enter `assets/` under this ruling without a downsample. Relevant when Phase 4 of the roadmap adds mobs.
- **`docs/the_wilderness_update.md` §19 asks for a written tile spec.** Phases 3 and 5 produce exactly that; fold the result back into the doc.
