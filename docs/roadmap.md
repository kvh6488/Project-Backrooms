# Project Backrooms — Roadmap & Reference

> **Living document** — updated as decisions are made. Last updated: **2026-06-29**.

---

## 1. Project Vision

A **2D top-down psychological horror maze game** (visual style similar to *The Escapists*) built as a **portfolio-grade engineering showcase**. Every gameplay mechanic maps to a specific CS/math concept.

**Inspired by:**
- **The Backrooms** (film) — the unsettling, liminal atmosphere of endless, procedurally shifting corridors with flickering yellow lights and no clear exit.
- **The Maze Runner** — the idea of a hostile, ever-changing maze that must be mapped and conquered to escape, with the environment itself as the primary antagonist.
- **Plague Inc.** — the virus/cure territory mechanics, where infection spreads across a map using population growth modelling and the player must combat it with their own competing system.

**The core loop:** A spreading **virus** drives maze chaos — shifting walls, increasing mob spawns, and deepening confusion. The player spreads a **cure** to stabilize the maze, map it, and survive. The game operates on a **psychological horror loop** where the virus is directly tied to the player's mind and performance: the enemy virus's strength directly influences the aggressiveness of maze shape-shifts, increases layout confusion, and boosts mob spawn rates. Instead of a static environment, the maze fights back harder the worse you're doing.

**Gameplay pillars:**
- **Navigation under pressure** — limited visibility, corner-peeking, and player-developed maps. The maze cannot be paused while exploring.
- **Survival over time** — players must survive as many days as possible, manage mandatory sleep, and balance risk vs. exploration. The ultimate goal is currently being evaluated (escaping vs endless survival loop).
- **Territory warfare** — the player's "clean" cure virus competes against the hostile corruption in a real-time BFS territory battle driven by differential equations.

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

### Phase 2 (IN PROGRESS) — Dynamic Maze & Textures *(Days 13–22)*
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

### Phase 3 — Virus & BFS Territory *(Days 23–34)*
The mathematical heart — competing virus systems spreading across the grid.
- **Domain Separation:** We must figure out how the virus/infection spreads through the maze as rooms and corridors are separated. Currently thinking the virus will be mainly a corridor thing and mobs a room thing.
  - *Note: Pre-spawned doors from Phase 1 act as strict flow-control boundaries that prevent the corridor-based virus from flooding into the room-based mob spawns.*
- Discretised Lotka-Volterra population model (infection vs. cure).
- Bidirectional BFS for virus territory expansion.
- Infection meter (time-in-zone accumulator):
  - Triggers auditory hallucinations, screen distortion.
  - Forces the maze to reroute more frequently.
  - Gases/viruses can physically weaken player, forcing crawl.
- Visual feedback: infected cells change color.
- Virus Radiator/emitter spawner nodes (destroyable — yields useful parts, removes blockages for cure spread).
- Cure spreader placement and expansion:
  - Cure acts like expanding BFS gas radar.
  - Slightly reduces dangers, stops maze from merging.
  - Permanently fixes cured region on the map.
- Maze mutation rate linked to infection level (shifting zone size starts at 14, increases/decreases every night depending on cure vs virus domination).

### Phase 4 — Mobs, Death & Progression *(Days 35–52)*
- **Death mechanics:**
  - Low-level hazards (upper depths): kidnap/knock player out, wake up in disorienting new location, day count intact.
  - High-level hazards (lower depths): harsh permadeath, reset run to Day 0.
  - *Note: Need to rethink death mechanics!*
  - Maps and resources can be permanently lost upon death.
  - Add player damage and death animations here (also need to think about potential player level mechanic)
- Stats screen on death (days survived, % mapped, kill counts).
- Live stat updates upon successfully finishing a level or dying. *Note: Not sure if there will be game levels anymore...*
- Escape trigger: cure threshold → complexity reduction → maze stops confusing player → cure crystal radar guides to long hallway exit. *Note: Not sure if the goal will be to escape the maze anymore.*
- Exploration gate mechanics: early game requires strict 100% maze exploration metric (of the fixed/capped region).
- **Navigation & Ecosystem:**
  - Dijkstra flow-field generation.
  - Mob spawning tied to Lotka-Volterra population caps and variable spawn rates.
  - Scent trail system (per-cell decay value)? - *May remove if too complex.*
  - Mob state machine: idle → patrol → track-scent → chase → attack.
  - Basic melee combat (hit detection, health, death).
  - List of Deadly mobs (those that attack you), to be determined later; each mob will have unique characteristics and abilities.
  - Flora & fauna ecosystem:
    - Cockroaches, moss, rats, mushrooms — plague-like infectious reproduction.
    - Potential bird/bat swarms.
    - Simulates a living ecological food chain with territorial fighting/conquering.
- **Mob Spawning Rules:** Also need to figure out if there will be mobs in corridors. Currently thinking a single jumpscare mob type with bird's eye view textures in corridors, and all other mobs randomly spawn inside rooms.
- May need to also add basic menu and save mechanisms, for testing, level settings etc.

### Phase 5 — Base Building, Crafting & Full Arsenal *(Days 53–72)*
- **Furniture spawns**: Searching desks, chairs, filing cabinets for raw materials and lore.
- **Fundamental materials**: Scrap metal, wires, wood, cloth, chemicals, electronic components.
- **Base placement**: Must be placed within a room. Balance entrances/exits (too few = cornered by mobs, too many = hard to defend).
- **Bed + sleep mechanic**: 
  - Allows sleeping. Pauses game temporarily (time limit). 
  - Sleeping produces maze shuffling, in regeneration region. 
  - Mandatory — failing to sleep causes sluggishness, hallucinations, passing out (lost/respawned).
- **Crafting system**: inventory → recipe → item.
- Barricades & Door placement (modify BFS cost map).
- Cure Spreaders (Chemical Dispersers): Alters differential equation variables in a sector, slowing/halting enemy virus spread. Upgradeable. Strategy: few heavily upgraded (large radar, reduced mob spawns) vs. dozens of weak ones (stops merging, easily destroyed).
- **Weapons & Gear**:
  - Batteries (found or crafted).
  - Torch + battery drain (potentially add lantern which lights up circle 360 degrees around player)
  - Rope mechanic (physical link, severable by mob attacks).
  - **Classic Map**: Crafted with pen and paper. Upgradeable (one time only) to increase its range. *Conflict note: May remain a fullscreen overlay (leaving player vulnerable) OR change to a minimap.*
  - Spray Can (mark walls, invalidated by shifts).
  - Barricades & Doors (wood, metal, reinforced).
  - Melee weapons (Baseball bat, nailed bat, Sword/Machete, Throwing Spear).
  - Armour (increases protection, reduces speed).
  - Tripwire Flashbangs *(May be removed)*.
  - Geiger Counter *(Likely to be removed)*.
  - Gas Mask / Hazmat Suit (merged item: protects from infection/gases, uses gas mask icon, turns player into gas suit man).
  - Gases filling rooms *(May be removed)*.

### Phase 6 — Advanced Mechanics & The Level 2 Descent *(Days 73–85)*
- **Level Transitions**: Screwdriver required to access Level 2 vents.
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
  - Magic Map (dynamically updates when maze shifts at night; fixed region, rare find, only via LSD).
- **Advanced Chemical Lab Crafting**:
  - Speed drug (faster run, paranoid hallucinations / fake mobs).
  - LSD / Shrooms (maze regenerates at its shifting zone every 1.5 minutes *even if player is awake*. Need to figure out what happens if player is caught in the shifting zone).
  - Parasitic Syringe (crafted from rat + cockroach serum. Injected into friendly dispersers for exponential/faster and wider cure spread. Severe decay rate if unmaintained).
- **Advanced Systems**:
  - Dog companion (Pitbull vs standard).
  - Hydroponic farming (moss spreading).
  - Symbiotic Infection (voluntary mutation for buffs like built-in night vision at the cost of emitting biological hum). *(Potential feature)*.
  - Rounds mechanic (CoD zombies style escalating mob counts). *(Unsure)*.

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
- Menu screens (title, death stats, level transition).
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
| Spray Can | Marks walls. Unreliable if maze shifts. | L1 |
| Torch | Light source. Requires batteries. Upgradeable range. Can blind mobs when upgraded. | L1+ |
| Classic Map | Crafted with pen & paper. Upgradeable (one time only) to increase its range. May be a fullscreen overlay or a minimap. Unreliable due to maze shifting. | L1 |
| Magic Map | Dynamically updates to reflect maze changes at night. Rare — only found when on LSD. Fixed region. | L1 (rare) |
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
| Doors | Wood / Metal / Reinforced. Placed in room exits to secure bases. Opened/closed by player. Upgradeable. | L1 (crafted) |

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
| LSD / Shrooms | Maze regenerates at shifting zone every 1.5 mins even if awake. Chance to find magic map. | L1 (crafted) |
| Parasitic Syringe | Crafted from rat + cockroach serum. Injected into friendly dispersers for exponential/faster and wider cure spread. Severe decay rate if unmaintained. | L1 (crafted) |

### Base Structures
| Item | Description | Level |
|---|---|---|
| Bed | Allows sleeping. Pauses game temporarily (time limit). Sleeping produces maze shuffling, in regeneration region. Mandatory — failing to sleep causes sluggishness, hallucinations, passing out (lost/respawned). | L1 (crafted) |
| Cure Spreaders (Chemical Dispersers) | Alters differential equation variables in a sector, slowing/halting enemy virus spread. Upgradeable. Strategy: few heavily upgraded (large radar, reduced mob spawns) vs. dozens of weak ones (stops merging, easily destroyed). | L1 (crafted) |
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
| **Doorway Node Graphs** | Pre-spawned doors naturally isolate rooms and corridors, allowing systems to abstract the 32x32 cellular grid into a highly optimized, high-level graph of connected sectors rather than individual cells. | Faster pathfinding, localized virus spread |
| **Mathematical Shuffling** | (TBD) Using abstract algebra (Permutation Groups) or discrete number theory (Linear Congruential Generators) to predictably shuffle the Level 2 room order based on the current Day index. | Level 2 room logic |

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
