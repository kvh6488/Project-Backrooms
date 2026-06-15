# Project Backrooms — Roadmap & Reference

> **Living document** — updated as decisions are made. Last updated: **2026-06-12 20:50 AEST**.

---

## 1. Project Vision

A **2D top-down psychological horror maze game** (visual style similar to *The Escapists*) built as a **portfolio-grade engineering showcase**. Every gameplay mechanic maps to a specific CS/math concept.

**Inspired by:**
- **The Backrooms** (film) — the unsettling, liminal atmosphere of endless, procedurally shifting corridors with flickering yellow lights and no clear exit.
- **The Maze Runner** — the idea of a hostile, ever-changing maze that must be mapped and conquered to escape, with the environment itself as the primary antagonist.
- **Plague Inc.** — the virus/cure territory mechanics, where infection spreads across a map using population growth modelling and the player must combat it with their own competing system.

**The core loop:** A spreading **virus** drives maze chaos — shifting walls, increasing mob spawns, and deepening confusion. The player spreads a **cure** to stabilize the maze, map it, and trigger an escape. The game operates on a **psychological horror loop** where the virus is directly tied to the player's mind and performance: the enemy virus's strength directly influences the aggressiveness of maze shape-shifts, increases layout confusion, and boosts mob spawn rates. Instead of a static environment, the maze fights back harder the worse you're doing.

**Gameplay pillars:**
- **Navigation under pressure** — limited visibility, corner-peeking, and player-developed maps. The maze cannot be paused while exploring.
- **Survival over time** — players must survive as many days as possible, manage mandatory sleep, and balance risk vs. exploration.
- **Territory warfare** — the player's "clean" cure virus competes against the hostile corruption in a real-time BFS territory battle driven by differential equations.
- **Escalating difficulty** — escaping one maze transitions to a harder one, infinitely. The game never truly ends.

**Dual purpose:**
- **Portfolio piece** (primary) — demonstrate systems engineering, algorithm design, and performance optimization to recruiters/senior engineers.
- **Real game** (secondary) — a genuinely fun and atmospheric horror experience.

**Developer context:** Built by a 2nd-year CS student as a first major project. All decisions should be well-documented for learning purposes.

---

## 2. Design Decisions

These are locked in based on our discussions. Changes require explicit discussion.

| Decision | Choice | Notes |
|---|---|---|
| **Perspective** | 2D top-down | Pixelated style similar to *The Escapists* |
| **Cell size** | 32×32 pixels (starting point) | Will adjust after first visual |
| **Input** | Keyboard only | Controller support is a future feature |
| **Audio** | Raylib built-in audio | Sufficient for project needs |
| **Target platform** | Windows only (initially) | Other OS support is a future feature |
| **Pause behaviour** | No pause menu while exploring | Game loop runs continuously during exploration |
| **Tile aesthetics (Phase 1)** | Placeholder solid colors | Proper textures loaded at end of Phase 1 |
| **Algorithms** | Hybrid BSP + Prim's + Loops | Evaluated DFS vs Prim's. Selected Prim's for highly branching corridors, BSP for office rooms. |
| **Maze Data Structure** | 1D Flat Array (Implicit Grid) | Maximizes CPU cache locality. Maps 2D via index = y * width + x. |
| **Maze "infinity"** | Fixed-size grid with Toroidal Wrapping | Uses modulo arithmetic `(x % width + width) % width` to wrap coordinates. |
| **Escape trigger** | When cure conquers enough of the maze + a certain generation threshold, guide the player to exit | Detailed mechanics TBD in Phases 3–4 |
| **Permadeath** | On completing a round, difficulty increases and player re-enters the same maze | Full reset only on Level 3 death (future phase) |

---

## 3. Tech Stack

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

## 4. Full Phased Roadmap

**Target:** All core phases (0–9) complete within **~85 days**.
**Full game (including optional phases 10–12):** 4–6 months.
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
- **Maze generation algorithm** — Hybrid BSP + Prim's Algorithm + Random Loops
- Maze data structure (cell/wall representation in a flat array)
- Render maze as colored tiles (placeholder)
- Player entity: position, WASD movement, wall collision
- **Dynamic 2D Camera** — Camera follows the player through a massively expanded maze grid

### Phase 2 — Dynamic Maze & Non-Euclidean Topology *(Days 13–22)*
Make the maze shift and break physical rules.
- Toroidal wrapping (modulo addressing for "infinite" feel)
- Maze mutation: reconnect edges on timer or distance trigger
- "Look-away" detection: re-roll rooms outside the player's view
- Edge pointer-swapping for non-Euclidean room flipping , or rotating ring
- Noclip hidden exits (wall cells secretly passable)
- Fog-of-war / limited visibility
- Load proper tile textures (end of phase)

### Phase 3 — Virus & BFS Territory *(Days 23–34)*
The mathematical heart — competing virus systems spreading across the grid.
- Discretised Lotka-Volterra population model (infection vs. cure)
- Bidirectional BFS for virus territory expansion
- Infection meter (time-in-zone accumulator)
  - Triggers auditory hallucinations, screen distortion
  - Forces the maze to reroute more frequently
  - Gases/viruses can physically weaken player, forcing crawl
- Visual feedback: infected cells change color
- Virsus Radiator/emitter spawner nodes (destroyable — yields useful parts, removes blockages for cure spread)
- Cure spreader placement and expansion
  - Cure acts like expanding BFS gas radar
  - Slightly reduces dangers, stops maze from merging
  - Permanently fixes cured region on the map
- Maze mutation rate linked to infection level

### Phase 4 — Death, Escape & Progression *(Days 35–42)*
Test the full cure → escape loop early, so we can verify the core game mechanic works.
- Death mechanics:
  - Low-level hazards (upper depths): kidnap/knock player out, wake up in disorienting new location, day count intact
  - High-level hazards (lower depths): harsh permadeath, reset run to Day 0
  - Maps and resources can be permanently lost upon death
- Stats screen on death (days survived, % mapped, kill counts)
- Live stat updates upon successfully finishing a level
- Escape trigger: cure threshold → complexity reduction → maze stops confusing player → cure crystal radar guides to long hallway exit
- Level progression: escaping transitions to a brand-new, significantly harder level (infinite scaling)
- Exploration gate mechanics: early game requires strict 100% maze exploration metric (of the fixed/capped region)

### Phase 5 — Mobs & Flow-Field Navigation *(Days 43–52)*
Entities that move, hunt, and fight using the flow-field.
- Dijkstra flow-field generation
- Mob spawning tied to Lotka-Volterra population caps and variable spawn rates
- Scent trail system (per-cell decay value)
- Mob state machine: idle → patrol → track-scent → chase → attack
- Basic melee combat (hit detection, health, death)
- Territorial mob interactions and fighting between mobs
- List of Deadily mobs (those that attack you), to be determined later each mob will have unique characteristics and abilites
- Flora & fauna ecosystem:
  - Cockroaches, moss, rats, mushrooms — plague-like infectious reproduction
  - Potential bird/bat swarms
  - Simulates a living ecological food chain with territorial fighting
- Scent trail system:
  - Mobs and player leave data-driven scent trails on the maze grid that fade over time
  - Heavily infected player leaves a much stronger trail that predatory mobs track

### Phase 6 — Base Building, Crafting & Essential Items *(Days 53–64)*
Player agency, progression, and core items needed for fundamental gameplay.
- **Furniture spawns**: Like the Backrooms film, odd furniture pieces (desks, chairs, filing cabinets, shelves) are occasionally placed throughout the maze. These are where found items (items that cannot be crafted) spawn. Players search furniture to discover items, raw materials, and lore. Mechanic to be expanded further.
- **Fundamental crafting materials**: Raw materials found throughout the maze (e.g., scrap metal, wires, wood, cloth, chemicals, electronic components) that aren't useful on their own but are combined in the crafting system to build useful items. Exact material list TBD — we will figure out what these are later as we design the crafting recipes.
- Base placement - bases must be placed within a room, you can choose any room to build you base in but you must balance the number of exits too few if mobs break through your door, you can get cornererd easily as no other direction, too many and you are forced
to maintain a large amount of doors meaning one of them is more likely to get broken down by mobs. Size is also a consideration smaller rooms often have less entrances/exits meaning there easier to defend, but have less space for building your base equipment
like farming, chemical lab etc, where as larger rooms often have a great amount of room for equipment but many more entrances/exits.
- Bed + sleep mechanic:
  - Allows sleeping, temporarily pauses game (time limit)
  - Sleeping is mandatory — failing to sleep causes sluggishness, increased hallucinations, can lead to passing out (lost/respawned)
  - Sleeping accelerates maze shuffling in outer regions (distant map scrambled when you wake up)
- Crafting system: inventory → recipe → item
- Barricade & Door placement (modify BFS cost map / block enemy BFS pathfinding)
  - Doors can be placed in room exits. They can be opened/closed by the player but must be broken down by mobs.
- Cure disperser building + upgrade tiers
  - Strategy choice: few heavily upgraded dispersers large radar (severely reduced mob spawns) vs. dozens of weak ones (stops maze merging but easily destroyed by mobs)
- Item pickup / inventory system
- Key Early Game Items:
  - Batteries (can be found or crafted in chemical lab)
  - Torch + battery drain (upgradeable range, can blind mobs when upgraded)
  - Rope mechanic (physical link, severable by direct mob attacks in L1)
  - Classic Map (limited region, full-screen overlay, unreliable due to maze shifts)
  - Spray Can (mark walls, invalidated by maze shifts)
  - Barricades (wood, reinforced wood — block BFS pathfinding)
  - Doors (wood, metal, reinforced — placed in room entrances/exits to secure a base, upgradeable health)
  - Melee weapons (baseball bat, bat with nails)
- Navigation risk: opening map takes up entire screen, leaving player blind and vulnerable

### Phase 7 — Full Level 1 Items *(Days 65–72)*
All remaining items that can be found or crafted in Level 1.
- Sword / Machete
- Throwing Spear / Trident Fork
- Armour (increases protection, reduces speed)
- Tripwire Flashbangs (defensive traps)
- Screwdriver (required for Level 2 access — item exists now, use comes later, cannot be crafted must be found on L1)
- Geiger Counter (passive belt item, clicks faster near un-destroyed Radiators or toxic virus/infection zones)
- Gas Mask (protects player from gases only)
- Hazmat Suit (protects from negative infection and gases)

### Phase 8 — Extra Gameplay Features *(Days 73–78)*
Fun gameplay systems that enrich Level 1 but aren't core to the loop.
- Dog companion (increases noise/scent, alerts to flanking enemies, tracks scents)
  - Pitbull variant (less sniffing, actively attacks mobs)
- Farming (done in base, hydroponic food, moss spreading like viruses)
- Chemical Lab crafting:
  - Speed drug (faster run, mobs faster too, paranoid hallucinations — fake mob spawns)
  - LSD/Shrooms (reveals hidden exits, aggressively increases maze shuffling, decreases maze emptiness and item spawn, increases item hallucinations — fake items, chance to find magic map, extremely dangerous in L3 — causes mass paranoid hallucinations)
  - Ketamine/Dissociative (instant floating/teleporting between maze levels without finding physical entrances)
  - Parasitic Syringe (crafted from rat + cockroach serum, injected into friendly dispersers for exponential hybrid virus spread, severe decay rate collapses disperser if not maintained. Other potential boosters use moss and mushrooms)
  - Cure will be developed in the chemical lab
  - Batteries, wires, scrap metal, and chemicals
- Symbiotic Infection (voluntary infection with mutated strains for buffs like Night Vision — eliminates torch reliance — at cost of emitting biological hum that attracts predators)
- Gases fill rooms and force players to crawl (deadly in level 2 vents)
- Potentially add rounds (will be days) were the number of mobs increases each round (like in cod zombies), meaning the game gets harder and each day and your chance of surviving decreases, although could affect overall gameplay goal and cure/virus dynamic. Not sure if I like this mechanic yet.
- Extra Items:
  - Magic Map (dynamically updates, rare find, limited region, only found when on LSD)
  - Pulsar Radar Gun (tracks base through shifting walls, pings enemies, frontal shield, built using crystals found in level 3)

### Phase 9 — GUI & Graphical Polish *(Days 79–85)*
Get Level 1 and everything in it looking perfect.
- Final tile textures and sprite art
- Player character sprite and animations
- Mob sprites and animations
- Item sprites and UI icons
- HUD design (health, infection meter, inventory bar, day counter)
- Clean up any left over testing framework shown in the GUI, or code, and move it into a dedicated testing mode (in normal gameplay mode, testing GUI element will not appear)
- Screen distortion effects (VHS aesthetic)
- Audio polish: ambient buzz, flickering lights, distrusting messages, mechanical maze shifting sounds, hallucination triggers, cryptic clues
- Micro-animations and visual feedback (damage, pickups, cure spreading)
- Menu screens (title, death stats, level transition)
- Level 1 atmosphere: flickering yellow lights, office rooms (torch not always required)

---

### Optional / Late-Stage Phases

### Phase 10 — Levels 2 & 3 *(Stretch — Days 86–96)*
Vertical depth — introduces new environments and items.
- Level transitions (screwdriver for L2, hidden exits for L3)
- L2: narrow vents, static geometry, crawl movement, long corridors with few turns make it easy for mobs to follow. May be the only level that never changes — reliable navigation anchor against looping geometries.
- L3: 6:1 spatial compression (like Minecraft's Nether), heavy darkness, occasional red emergency/alarm lights. Access via rope tied to vent cover in L2 (vent can occasionally snap shut, trapping you; rope can be cut by stalking mobs), or enter without a rope. Bases cannot be built here. Players cannot sleep. Map changes too frequently to draw.
- Radar/map suppression in L3 (radar guns fail due to localized crystal interference)
- Rope in L3: only lifeline, can be cut mid-way by stalking mobs
- Level 2 & 3 specific items:
  - Night Vision Goggles (see in all light conditions, requires visiting depths of L3 to get somehow unsure yet)
  - Glitched VHS Tape (circular ring buffer, O(1) state rewind, 30s–10m recording, consumable found in deep levels)
  - Pulsar Radar Gun crystals (found in L3)
  - L3 provides rare resources for powerful items

### Phase 11 — Telemetry & Python Analytics *(Stretch — Days 97–103)*
Data science showcase.
- CSV logger: frame time, entity count, infection %, cure %, maze entropy
- Export triggers (death, level complete, manual)
- Jupyter notebook: parse CSV, plot time series
- Data on aglorithm run time (for Big O analysis in report)
- Discrete derivatives (infection rate of change)
- Riemann sum (cumulative exposure integral)

### Phase 12 — README, Report & Portfolio Polish *(Days 104–110)*
Presentation layer.
- README.md with GIF hook, architecture diagrams, 3-min summary
- Academic report: Big O proofs & analysis, Python charts, algorithm analysis
- Code cleanup, commenting (finalise comments & clean them up), final profiling pass

---

## 5. Full Items Reference

All items from the design document, organized by category.

### Navigation Items
| Item | Description | Level |
|---|---|---|
| Spray Can | Marks walls. Unreliable if maze shifts. | L1 |
| Torch | Light source. Requires batteries. Upgradeable range. Can blind mobs when upgraded. | L1+ |
| Classic Map | Maps a limited region at a point in time. Unreliable due to maze shifting. Full-screen overlay (leaves player blind/vulnerable). | L1 |
| Magic Map | Dynamically updates to reflect maze changes. Rare — can only be found when on LSD. Limited region. Full-screen overlay. | L1 (rare) |
| Rope | Physical link between locations. Can be severed by direct mob attacks (L1) or cut mid-way by stalking mobs (L3). | L1+ |
| Pulsar Radar Gun | Tracks base signals through shifting walls. Pings enemies from afar. Frontal shield (speedy enemies can still flank). Requires batteries. Best tether. Built using crystals found in L3. | L3 (crafted) |
| Geiger Counter | Passive belt item. Clicks faster near un-destroyed Radiators or toxic virus/infection zones. | L1 |

### Weapons & Defence
| Item | Description | Level |
|---|---|---|
| Baseball Bat | Basic melee weapon. | L1 |
| Bat with Nails | Upgraded melee weapon. | L1 |
| Sword / Machete | Melee weapon. | L1 |
| Throwing Spear / Trident Fork | Ranged melee / throwing weapon. | L1 |
| Armour | Increases protection, reduces movement speed. | L1 |
| Gas Mask | Protects player from gases only. | L1 |
| Hazmat Suit | Protects from negative infection and gases. | L1 |
| Tripwire Flashbangs | Defensive traps. | L1 |
| Barricades | Wood / reinforced wood. Blocks enemy BFS pathfinding. Upgradeable. | L1 (crafted) |
| Doors | Wood / Metal / Reinforced. Placed in room exits to secure bases. Opened/closed by player. Upgradeable. | L1 (crafted) |

### Unique Gear & Companions
| Item | Description | Level |
|---|---|---|
| Dog | Increases noise/scent (attracts mobs). Alerts to flanking enemies. Tracks enemy scents. | L1 |
| Pitbull Dog | Less sniffing ability. Actively attacks/defends against mobs. | L1 |
| Screwdriver | Required to access Level 2 (vents), cannot be crafted must be found on L1. | L1 (use in L2) |
| Night Vision Goggles | Allows player to see in all light conditions. Requires visiting depths of L3 in someway to get them, unsure how yet. | L3 (found) |
| Glitched VHS Tape | Consumable. 30s–10m recording. Rewinds player state (position, health, inventory) to recording start. O(1) ring buffer. Stopping/starting uses tape time. Only rewind to start of most recent recording. Once rewound, tape disappears. One use per tape. | L3 (found) |

### Chemicals, Drugs & Bio-Boosters (Crafted in Base Lab)
| Item | Description | Level |
|---|---|---|
| Speed | Faster run speed for player. Mobs slightly faster. Increases paranoid hallucinations (fake mob spawns). | L1 (crafted) |
| LSD / Shrooms | Reveals hidden wall escapes. Aggressively increases maze shuffling. Decreases maze emptiness and item spawns. Increases item hallucinations (fake items). Chance to find magic map. Extremely dangerous in L3 (mass paranoid hallucinations). | L1 (crafted) |
| Ketamine / Dissociative | Instant floating/teleporting between the three vertical maze levels without finding physical entrances. | L1 (crafted) |
| Parasitic Syringe | Crafted from rat + cockroach serum. Injected into friendly dispersers for exponential hybrid virus spread, but severe decay rate collapses disperser if not maintained. Other boosters possible with moss/mushrooms. | L1 (crafted) |

### Base Structures
| Item | Description | Level |
|---|---|---|
| Bed | Allows sleeping. Pauses game temporarily (time limit). Sleeping accelerates distant maze shuffling. Mandatory — failing to sleep causes sluggishness, hallucinations, passing out (lost/respawned). | L1 (crafted) |
| Cure Spreaders (Chemical Dispersers) | Alters differential equation variables in a sector, slowing/halting enemy virus spread. Upgradeable. Strategy: few heavily upgraded (large radar, reduced mob spawns) vs. dozens of weak ones (stops merging, easily destroyed). | L1 (crafted) |
| Hydroponic Farm | Grows food. Uses moss that spreads like viruses. | L1 (crafted) |

---

## 6. Open Decisions Tracker

Items still needing discussion or research before we can proceed.

| # | Topic | Status | Notes |
|---|---|---|---|
| 1 | **Maze generation algorithm** | 🟢 Decided | Hybrid BSP + Prim's algorithm with loop injection |
| 2 | **Grid size for Level 1** | 🔴 Not decided | Needs to feel "infinite" but fit within a memory cap. 64×64? 128×128? Chunk-based? |
| 3 | **Cell size in pixels** | 🟢 Decided (32×32) | Starting point, will adjust after first visual |
| 4 | **Maze data structure** | 🟢 Decided | Implicit 2D grid graph stored as a contiguous 1D array for cache locality |
| 5 | **Camera system details** | 🟡 Basic plan | Follow-player with viewport clipping. Fog-of-war / view cone specifics TBD |
| 6 | **"Look-away" detection** | 🔴 Not decided | How do we determine which cells the player "isn't looking at" in 2D top-down? |
| 7 | **Escape / memory cap threshold** | 🔴 Not decided | What % of maze cured triggers the escape? What's the fixed size? |

---

## 7. Algorithmic Ideas & Theoretical Showcase

All algorithms and theoretical approaches from the original design document. These are **starting points, not final decisions** — each will be researched and evaluated before implementation. Algorithms are grouped by the system they serve.

> **Note:** This section exists so future conversations can reference all algorithmic ideas without needing to read the original .docx.

### Memory & Data Architecture
| Concept | Approach | Used For |
|---|---|---|
| **Implicit Grid & DOD** | Store maze state as an implicit graph in a 1D flat array (`std::vector<int>`). Edges are inferred mathematically (`y * width + x`) rather than stored as pointers, maximizing CPU cache locality. Entity pools will use SoA. | Maze grid, entity pools |
| **Toroidal Wrapping (Modulo Math)** | Statically pre-allocate fixed-size 1D array. Use modulo arithmetic `(x % width + width) % width` to wrap coordinates. Mathematically transforms the flat grid into a continuous torus graph with no boundary walls. | Toroidal "infinite" grid |
| **Zero-Allocation Ring Buffer** | Fixed-size circular buffer continuously tracks minimal struct of player state (position, health, inventory flags). Instant rollback in O(1). | VHS Tape time-rewind mechanic |

### Maze Generation & Topology
| Concept | Approach | Used For |
|---|---|---|
| **Hybrid Procedural Generation** | BSP ($O(R \log R)$) for office rooms + Prim's ($O(V \log V)$) for highly-branched spanning tree corridors + Loop Injection pass ($O(V)$) to break the tree and add cycles. | Initial maze layout |
| **Non-Euclidean Spatial Topology** | Edge pointer-swapping to physically connect disjointed maze sectors in O(1) without moving rendered assets. | Room flipping, looping edges, impossible geometry |

### Macro Simulations & Territory
| Concept | Approach | Used For |
|---|---|---|
| **Mathematical Population Modelling** | Discretised system of non-linear Lotka-Volterra (Predator-Prey) differential equations. Calculates global population metrics, growth, decay, and territorial balance every frame. | Virus vs. cure ecosystem balance, mob spawn rates |
| **Frontier Expansion Search** | Bidirectional Breadth-First Search (BFS) as spatial execution engine. Decoupled Macro/Micro pipeline for deterministic execution under load. Maps intersection boundary of competing forces across 2D grid, modifying local cost-maps. | Virus/cure territory expansion |

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

## 8. Future Features

Features planned for after the core game is complete (post-Phase 12). Not in the current scope.

| Feature | Notes |
|---|---|
| **Controller support** | Gamepad input alongside keyboard |
| **Cross-platform (Linux/Mac)** | CMake already supports this; needs testing and CI setup |
| **Advanced AI opponents** | Reinforcement learning for dynamic difficulty |
| **LAN multiplayer** | Two-player co-op and versus modes |
| **Concurrency & parallelism** | Multi-threading, mutex design, parallel computation for performance |
| **ONNX / CUDA AI integration** | Neural network decision logic offloaded to GPU |
| **Heuristic vs. AI comparison** | Empirical comparison of hand-written pathfinding vs. trained NN models |

---

## 9. Working Agreements

- **No code changes without explicit user approval.**
- **All algorithm choices are explored together** — I present options, we discuss, you decide.
- **This document is updated** whenever a decision is made or a phase is completed.
- **User is in the loop at every step** — no surprise implementations.
- **Explain before doing** — new concepts, tools, and workflows are explained before they're used.
