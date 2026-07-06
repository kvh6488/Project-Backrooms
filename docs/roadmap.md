# Project Backrooms — Roadmap & Reference

> **Living document** — updated as decisions are made. Last updated: **06-07-2026**.

---

## 1. Project Vision

A **2D top-down psychological horror maze game** (visual style similar to *The Escapists*) built as a **portfolio-grade engineering showcase**. Every gameplay mechanic maps to a specific CS/math concept.

**Inspired by:**
- **The Backrooms** (film) — the unsettling, liminal atmosphere of endless, procedurally shifting corridors with flickering yellow lights and no clear exit.
- **The Maze Runner** — the idea of a hostile, ever-changing maze that must be mapped and conquered to escape, with the environment itself as the primary antagonist.
- **Plague Inc. / CoD Zombies** — population growth modelling for radiation spread, combined with wave-based escalating survival where the player must endure an ever-growing threat.

**The core loop:** The primary goal is to survive the increasing waves and difficulty of the maze as days go by, seeing how far you can get before you die. The game operates on a **psychological horror loop** where the day count is directly tied to the player's mind and performance: the day count directly influences the aggressiveness of maze shape-shifts, increases layout confusion, radiation strength/spread, and boosts mob spawn rates. Instead of a static environment, the maze fights back harder as the days progress. If you die, your entire game resets to the beginning.

**Gameplay pillars:**
- **Navigation under pressure** — limited visibility, corner-peeking, and player-developed maps. The maze cannot be paused while exploring.
- **Escalating Survival (Wave-based)** — players must survive increasing waves of mobs and difficulty as days go by. The goal is to see how far you can get.
- **Harsh Permadeath** — any death results in a complete reset of the run back to Day 0.
- **Radiation Expansion** — hostile radiation spreads in a real-time BFS expansion driven by differential equations, progressively making the maze more hostile.

**Dual purpose:**
- **Portfolio piece** (primary) — demonstrate systems engineering, algorithm design, and performance optimization to recruiters/senior engineers.
- **Real game** (secondary) — a genuinely fun and atmospheric horror experience.

**Developer context:** Built by a 2nd-year CS student as a first major project. All decisions should be well-documented for learning purposes.

---

## 2. Tech Stack

```
┌─────────────────────────────────────────────┐
│           The Compute Core (C++20)          │
│                                             │
│  Engine Logic ─── Maze Gen ─── Entities     │
│       │              │             │        │
│       └──────────────┴─────────────┘        │
│                      │                      │
│              Raylib (Rendering)             │
│              Dear ImGui (Debug UI)          │
│              rlImGui (Raylib↔ImGui bridge)  │
│                      │                      │
│              CSV Telemetry Export           │
├─────────────────────────────────────────────┤
│       The Data Science Workspace            │
│                                             │
│  Python ─── NumPy ─── Matplotlib            │
│  Jupyter Notebook (parse + plot CSV logs)   │
├─────────────────────────────────────────────┤
│            Build & Version Control          │
│                                             │
│  CMake (build system)                       │
│  MSVC cl.exe (compiler, via Visual Studio)  │
│  Git + GitHub (version control)             │
│  FetchContent (Raylib, ImGui, rlImGui)      │
└─────────────────────────────────────────────┘
```

---

## 3. Full Phased Roadmap

**Target:** All core phases complete within **~85 days**.
> **Note:** Day estimates are rough guides, not hard deadlines. We'll adjust as we go.

---

### Phase 0 (COMPLETE) — Project Scaffold *(Days 1–3)*
Get the project compiling, a window open, and the debug overlay working.
- CMake project with FetchContent (Raylib, ImGui, rlImGui)
- Window opens, renders a colored background
- Basic game loop: input → update → render at 60 FPS
- Dear ImGui debug overlay showing FPS + frame time
- Git repo initialized with `.gitignore`, pushed to GitHub

### Phase 1 (COMPLETE) — Procedural Maze & Player *(Days 4–12)*
Generate a maze, render it, and let the player walk through it.
- **Maze generation algorithm** — Hybrid BSP + Prim's Algorithm + Random Loops + Connected Component Pruning.
- Maze data structure (cell/wall representation in a flat array).
- Render maze as colored tiles (placeholder).
- Player entity: position, WASD movement, wall collision.
- **Dynamic 2D Camera** — Camera follows the player through a massively expanded maze grid.
- **Room vs. Corridor Separation** — Rooms and corridors are strictly separated via pre-spawned doors. This constrains rendering complexity and establishes choke points for gameplay.

### Phase 2 (COMPLETE) — Dynamic Maze & Textures *(Days 13–22)*
Make the maze visually distinct and physically wrap around.
- Toroidal wrapping (modulo addressing for "infinite" feel).
- Maze mutation/regeneration sections.
- Load proper tile textures for rooms and corridors.
- Add door textures.
- Animate player movement.
- **Fog-of-war / visibility system:** 
  - Corridors are inherently dark. First entering yields a message: *"Damn it's dark in these corridors"*, with heavily restricted sight distance (raycasting).
  - Rooms are always fully lit.
  - Equipping a torch extends corridor visibility distance (implmented in later phase)
- Possibly resize game window, and gameview to fit screen etc, add temporary application icon.

### Phase 3 — Radiation & Base Game development *(Days 23–34)*
The mathematical heart — radiation systems spreading across the grid.
- **Radiation Sources (Barrels):**
  - Random barrels spawn in random rooms across the map.
  - Emit a radiation zone around them (starting at ~15 tiles radius).
  - Radiated tiles turn green, spreading calculated using Breadth-First Search (BFS).
- **Debug & UI:**
  - Add an ImGui toggle to visualize radiation barrels and their BFS spread on the minimap.
- **Domain Separation:** Standard doors do not block radiation spread (it flows like a fluid through empty corridors and rooms). Players can upgrade doors to block radiation later.
- BFS for radiation territory expansion.
- Radiation causes lighting flickering.
- **Radiation Spread rules:**
  - If any part of a room gets radiated, the whole room does.
  - Corridors act as the linking arteries that spread radiation between rooms.
- **Resource Spawning:**
  - Special items and resources (like Mushrooms) only spawn in radiated rooms.
  - Two types of mushrooms (magic and normal)
- **Inventory System**
  - Allows player to pick up and hold items, 20 slots in total, 5 active slots.

- **Magic Mushrooms & Map**
  - Visual distoation when player eats mushrooms screen fades between colours, and wavy effect applied over screen
  - Maze regenerates at its shifting zone every minute *even if player is awake*. Need to figure out what happens if player is caught in the shifting zone
  - Eating magic mushroom allows player to find magic map randomly in maze draws/cabinents (may have to add random funiture spawns here)
  - Magic Map UI (Could include raidation points and regen zones not sure)
  - Magic map maybe also only found near spawn, (still rare to find), and is instantly set to show just up to the regen zone square border no further.
- **Add pens and paper spawns in draw and basic crafting system for classic map spawn now to, shows the maze as it is now for a given rectangle radius when opened, permenatly fixed, at that spot.
  - **Furniture spawns**: Searching desks, chairs, filing cabinets for raw materials and lore.
  - **Fundamental materials**: Scrap metal, wires, wood, cloth, chemicals, electronic components.
  - **Classic Map**: Crafted with pen and paper. Upgradeable (one time only) to increase its range. *Conflict note: May remain a fullscreen overlay (leaving player vulnerable) OR change to a minimap.*

### Phase 4 — Mobs, Death, Progression & Main menu *(Days 35–52)*
- **Main menu:** Allows player to start game and see stats like longest day surived (no saves)
- **Death mechanics:**
  - Harsh permadeath: If you die from any hazard, the entire game resets to Day 0.
  - The core loop is built around seeing how far you can get before this inevitable reset.
  - Add player damage and death animations here (also need to think about potential player level mechanic).
- Add player health bar (radiation slows down player heal regen), also setup and use ablity/disabilty icon HUD system.
- Stats screen on death (days survived, % mapped, kill counts).
- Stat summary upon dying. 

- Need to add sleep meachnic here so days can be passed.
- **Base placement**: Must be placed within a room. Balance entrances/exits (too few = cornered by mobs, too many = hard to defend), if attempted to place in regeneration zone pop up saying "There is something off with room appears" before you are forced to try place again to confirm, base cannot be made in radition zones.
- **Bed + sleep mechanic**: 
  - Allows sleeping. Pauses game temporarily (time limit). 
  - Sleeping produces maze shuffling, in regeneration region. 
  - Mandatory — failing to sleep causes sluggishness, hallucinations, passing out (lost/respawned).
- Discretised Lotka-Volterra population model (radiation vs. cure), or for mobs and escalting mob waves.

- **Wave-based Progression:** Days act as a rounds mechanic (CoD zombies style escalating mob counts). Every day it gets harder to survive, so the player must keep upgrading equipment and preparing to survive for as long as possible.

- **Navigation & Ecosystem:**
  - Dijkstra flow-field generation.
  - Mob spawning tied to Lotka-Volterra population caps and variable spawn rates.
  - Scent trail system (per-cell decay value)? - *May remove if too complex.*
  - Mob state machine: idle → patrol → track-scent → chase → attack.
  - Basic melee combat (hit detection, health, death), will need to add one weapon here to start.
  - List of Deadly mobs (those that attack you), to be determined later; each mob will have unique characteristics and abilities.
  - Flora & fauna ecosystem:
    - Cockroaches, moss, rats.
    - Potential bird/bat swarms.
    - Simulates a living ecological food chain with territorial fighting/conquering.
- **Mob Spawning Rules:** Also need to figure out if there will be mobs in corridors. Currently thinking a single jumpscare mob type with bird's eye view textures in corridors, and all other mobs randomly spawn inside rooms.

**Cure meachnic (may be removed)**
- Cure spreader placement and expansion:
  - Cure acts like expanding BFS safe-zone radar.
  - Slightly reduces dangers like mobs, radtion, and maze shifiting.
  - Potential chemical lab upgrades to the cure that stop radiation spread, or maze regenration
- Potential maze regeneration zone size changes (shifting zone size starts at 14, increases/decreases every night depending on cure, radiation, or something else maybe just as the days goes on more of the maze shifts/regenerates each night).
- Potentail spreading radiation drives maze chaos — shifting walls, increasing mob spawns.

### Phase 5 — Full Arsenal & Crafting  *(Days 53–72)*
- **Full Crafting system**: inventory → recipe → item, potential crafting table vs no crafting, all required items now have a crafting recipe.
- Barricades/Door upgrades (modify BFS map and radition spread).
- Potential Cure Spreaders (Chemical Dispersers): Alters differential equation variables in a sector, slowing/halting enemy spawn and radtion spread. Upgradeable. Strategy: few heavily upgraded (large radar, reduced mob spawns) vs. dozens of weak ones ( easily destroyed).
- **Weapons & Gear**:
  - Batteries (found or crafted).
  - Torch/Lantern + battery drain
  - Rope mechanic (physical link, severable by a single mob hit).
  - Melee weapons (Baseball bat, nailed bat, Sword/Machete, Throwing Spear).
  - Armour (increases protection, reduces speed).
  - Tripwire Flashbangs *(May be removed)*.
  - Geiger Counter *(Likely to be removed)*.
  - Gas Mask / Hazmat Suit (merged item: protects from infection/gases, uses gas mask icon, turns player into gas suit man, upgradble).

### Phase 6 — Advanced Mechanics & The Level 2 Descent *(Days 73–85)*
- **Level Transitions**: Screwdriver (found) required to access Level 2 vents.
- **Level 2 Architecture**:
  - Located underneath the *inner, non-shifting* sections of Level 1.
  - **[TBD] Open Question:** What happens if the virus shifting zone expands inward and swallows a corner vent?
  - Accessed via corner vents.
  - Structurally, Level 2 is a 1D circular ring. Visually plays as a side-scrolling, never-ending long corridor (no floor tiles rendered, only walls/doors).
  - Equally spaced doors with lights above them lead to equally sized "lucky dip" rooms containing unique features, items, or lore.
  - **[TBD] Room Shuffling:** When sleeping, room orders are mathematically shuffled. *Idea: Base this shuffling on mathematical series patterns (like Permutation Groups or Linear Congruential Generators) to showcase a CS concept.*
- **Advanced Level 2 Items**:
  - Night Vision Goggles (fully lights up all sections of corridors; found in L2).
  - Pulsar Radar Gun (tracks base, frontal shield; built using L2 crystals).
  - Glitched VHS Tape (circular ring buffer, O(1) state rewind; found in L2. *Possible mechanic*).
  - Potential toxic waste destoyer?
  - Potential Computer/Computer map - current full debug map, advanced item only at highest level.
- **Advanced Chemical Lab Crafting**:
  - Speed drug (faster run, paranoid hallucinations / fake mobs).
  - Parasitic Syringe (crafted from rat + cockroach serum. Injected into friendly dispersers for exponential/faster and wider cure spread. Severe decay rate if unmaintained).
- **Advanced Systems**:
  - Dog companion (Pitbull vs standard).
  - Hydroponic farming (items like moss, mushrooms etc can be farmed so you can resupply easy instead of waitng for spawns).
  - Symbiotic Infection (voluntary mutation for buffs like built-in night vision at the cost of emitting biological hum). *(Potential feature)*.

> **Archive Note (Old Level 3 Concepts):** 
> Originally, the game planned a "Level 3" with 6:1 spatial compression (like Minecraft's Nether), heavy darkness, no map/radar reliability, and ropes as the only lifeline. To constrain project scope, Level 3 was scrapped. Its unique items (Night Vision, VHS Tape, Pulsar Crystals) were moved into the Level 2 "lucky dip" rooms, and level 2 rooms changed from narrow vents when player forced to crawl to the new level 2.

### Phase 7 — GUI & Graphical Polish *(Days 86–90)*
*(Finalisation - most should be completed already)*
Get Level 1 and Level 2 looking perfect.
- Final tile textures and sprite art (Player, Mobs, Items, UI icons), maybe add carpet bitmasking.
- HUD design (health, infection meter, inventory bar, day counter).
- Clean up leftover testing framework into a dedicated testing mode (hidden during normal gameplay).
- Screen distortion effects (VHS aesthetic).
- Audio polish: ambient buzz, flickering lights, distrusting messages, mechanical maze shifting sounds, hallucination triggers, cryptic clues.
- Micro-animations and visual feedback (damage, pickups, cure spreading).
- Menu screens (title, death stats, level transition), finalise full screen and window resizing.
- General atmosphere: flickering yellow lights, office rooms.

### Phase 8 — Telemetry & Python Analytics *(Stretch — Days 91–97)*
Data science showcase.
- CSV logger: frame time, entity count, infection %, cure %, maze entropy.
- Export triggers (death, level complete, manual).
- Jupyter notebook: parse CSV, plot time series.
- Data on algorithm run time (for Big O analysis in report).
- Discrete derivatives (infection rate of change).
- Riemann sum (cumulative exposure integral).

### Phase 9 — README, Report & Portfolio Polish *(Days 98–105)*
Presentation layer.
- README.md with GIF hook, architecture diagrams, 3-min summary.
- Academic report: Big O proofs & analysis, Python charts, algorithm analysis.
- Code cleanup, commenting (finalise comments & clean them up), final profiling pass.
- **Thorough quizzing and interview practice/test:** Making sure I understand all code and concepts generated by the agent, and have read and understood clearly every line.

---

## 4. Full Items Reference

All items organized by category.

### Navigation Items
| Item | Description | Level |
|---|---|---|
| Torch/Lantern | Light source. Requires batteries. Upgradeable range (starts at circle size 3.5, then can go to 4.5, 6) (potentially blinds mobs - flash bang effect for a few seconds at max level) | L1+ |
| Classic Map | Crafted with pen & paper. Upgradeable (one time only) to increase its range. May be a fullscreen overlay or a minimap. Unreliable due to maze shifting. | L1 |
| Magic Map | Dynamically updates to reflect maze changes at night. Rare — only found when under the effects of Magic Mushrooms. Fixed region. | L1 (rare) |
| Infected Map | (Potential) Shows radiation zones on map. | L1 |
| Rope | Physical link between locations. Can be severed by direct mob attacks. | L1+ |
| Pulsar Radar Gun | Tracks base signals through shifting walls. Pings enemies from afar. Frontal shield. Built using crystals. | L2 (crafted) |
| Geiger Counter | Passive belt item. Clicks near un-destroyed Radiators. *(Likely to be removed)*. | L1 |

### Weapons & Defence
| Item | Description | Level |
|---|---|---|
| Baseball Bat | Basic melee weapon. | L1 |
| Bat with Nails | Upgraded melee weapon. | L1 |
| Sword / Machete | Melee weapon. | L1 |
| Throwing Spear / Trident Fork | Ranged melee / throwing weapon. | L1 |
| Armour | Increases protection, reduces movement speed. | L1 |
| Gas Mask / Hazmat Suit | Protects player from infection/gases. Uses gas mask icon, turns player texture into gas suit man. | L1 |
| Tripwire Flashbangs | Defensive traps. *(May be removed)*. | L1 |
| Barricades | Wood / reinforced wood. Blocks enemy BFS pathfinding. Upgradeable. | L1 (crafted) |
| Doors | Wood / Metal / Reinforced. Placed in room exits to secure bases. Opened/closed by player. Upgradeable to block radiation. | L1 (crafted) |

### Unique Gear & Companions
| Item | Description | Level |
|---|---|---|
| Dog | Increases noise/scent. Alerts to flanking enemies. Tracks enemy scents. (Pitbull variant actively attacks). | L1 |
| Screwdriver | Required to access Level 2 vents. Must be found on L1. | L1 (use in L2) |
| Night Vision Goggles | Fully lights up all sections of corridors. | L2 (found) |
| Glitched VHS Tape | Consumable. 30s–10m recording. Rewinds player state to recording start. O(1) ring buffer. *(Possible mechanic)*. | L2 (found) |

### Chemicals, Drugs & Bio-Boosters (Crafted in Base Lab)
| Item | Description | Level |
|---|---|---|
| Speed | Faster run speed. Mobs slightly faster. Increases paranoid hallucinations (fake mob spawns). | L1 (crafted) |
| Magic Mushrooms | Maze regenerates at shifting zone every 1.5 mins even if awake. Required state to find the Magic Map. | L1 (found/farmed) |
| Parasitic Syringe | Crafted from rat + cockroach serum. Injected into friendly dispersers for exponential/faster and wider cure spread. Severe decay rate if unmaintained. | L1 (crafted) |
| Potential LSD |

### Base Structures
| Item | Description | Level |
|---|---|---|
| Bed | Allows sleeping. Pauses game temporarily (time limit). Sleeping produces maze shuffling, in regeneration region. Mandatory — failing to sleep causes sluggishness, hallucinations, passing out (lost/respawned). | L1 (crafted) |
| Cure Spreaders (Chemical Dispersers) | Alters differential equation variables in a sector, slowing/halting radiation spread and enemy spawns. Upgradeable. Strategy: few heavily upgraded (large radar, reduced mob spawns) vs. dozens of weak ones (stops merging, easily destroyed). | L1 (crafted) |
| Hydroponic Farm | Grows food. Uses moss that spreads like viruses. | L1 (crafted) |

---

## 5. Algorithmic Ideas & Theoretical Showcase

These are **starting points, not final decisions** — each will be researched and evaluated before implementation. Algorithms are grouped by the system they serve.

### Memory & Data Architecture
| Concept | Approach | Used For |
|---|---|---|
| **Implicit Grid & DOD** | Store maze state as an implicit graph in a 1D flat array (`std::vector<int>`). Edges are inferred mathematically (`y * width + x`) rather than pointers, maximizing CPU cache locality. Entity pools use SoA. | Maze grid, entity pools |
| **Toroidal Wrapping (Modulo Math)** | Statically pre-allocate fixed-size 1D array. Use modulo arithmetic `(x % width + width) % width` to wrap coordinates. Mathematically transforms the flat grid into a continuous torus graph. | Toroidal "infinite" grid, Level 2 circular buffer |
| **Zero-Allocation Ring Buffer** | Fixed-size circular buffer continuously tracks minimal struct of player state (position, health, inventory flags). Instant rollback in O(1). | VHS Tape time-rewind mechanic |

### Maze Generation & Topology
| Concept | Approach | Used For |
|---|---|---|
| **Hybrid Procedural Generation** | BSP ($O(R \log R)$) for office rooms + Prim's ($O(V \log V)$) for highly-branched spanning tree corridors + Loop Injection + Connected Component Pruning (identifying and stripping dead-end alcoves). | Initial maze layout |
| **Doorway Node Graphs** | Pre-spawned doors naturally isolate rooms and corridors, allowing systems to abstract the 32x32 cellular grid into a highly optimized, high-level graph of connected sectors rather than individual cells. | Faster pathfinding, localized radiation spread |
| **Mathematical Shuffling** | (TBD) Using abstract algebra (Permutation Groups) or discrete number theory (Linear Congruential Generators) to predictably shuffle the Level 2 room order based on the current Day index. | Level 2 room logic |

### Macro Simulations & Territory
| Concept | Approach | Used For |
|---|---|---|
| **Mathematical Population Modelling** | Discretised system of non-linear Lotka-Volterra (Predator-Prey) differential equations. Calculates global population metrics, growth, decay, and territorial balance every frame. | Radiation vs. cure ecosystem balance, mob spawn rates |
| **Frontier Expansion Search** | Bidirectional Breadth-First Search (BFS) as spatial execution engine. Decoupled Macro/Micro pipeline for deterministic execution under load. Maps intersection boundary of competing forces across 2D grid, modifying local cost-maps. | Radiation/cure territory expansion |

### Agent Routing & Navigation
| Concept | Approach | Used For |
|---|---|---|
| **Deterministic Flow-Field Routing** | Centralized discrete vector field generated via Dijkstra's algorithm. Agent updates via discrete approximations (Euler's method). Each agent gets optimal movement vector via O(1) matrix lookup. | Mob navigation, entity pathfinding |
| **Dynamic Shortest Path Routing** | Real-time graph traversal and edge-weight evaluation in a continuously mutating network. Vector magnitude calculations and dot products for relative positioning and alignment. | Pulsar radar gun, base tracking through shifting walls |

### Telemetry & Analysis
| Concept | Approach | Used For |
|---|---|---|
| **Discrete Derivative** | Calculate infection growth rate (first derivative) and acceleration rate (second derivative) from CSV time-series data. | Runtime performance analysis |
| **Riemann Sum Integration** | Area under the curve representing total cumulative radiation/infection exposure over a run. | Cumulative risk evaluation |
| **Heuristic vs. AI Comparison** | Empirically compare hand-coded search algorithms against trained Neural Network pathing models. | Portfolio showcase (future) |

### Mathematical Foundation Summary
The engine is driven by the intersection of **discrete mathematics**, **applied linear algebra**, and **numerical calculus**:
- Graph theory and modulo arithmetic form the structural foundation
- Discretised non-linear differential equations (Lotka-Volterra) and centralized discrete vector fields govern real-time behaviour
- Continuous mathematical models bridged to discrete execution via Euler's method
- Deterministic probability theory (PRNG state spaces) underpins state generation
- Discrete derivatives and numerical integration (Riemann sums) evaluate runtime telemetry

---

## 6. Future Features

Features planned for after the core game is complete. Not in the current scope.

| Feature | Notes |
|---|---|
| **Controller support** | Gamepad input alongside keyboard |
| **Cross-platform (Linux/Mac)** | CMake already supports this; needs testing and CI setup |
| **Advanced AI opponents** | Reinforcement learning for dynamic difficulty |
| **LAN multiplayer** | Two-player co-op and versus modes |
| **Concurrency & parallelism** | Multi-threading, mutex design, parallel computation for performance |
| **ONNX / CUDA AI integration** | Neural network decision logic offloaded to GPU |
| **Heuristic vs. AI comparison** | Empirical comparison of hand-written pathfinding vs. trained NN models |
