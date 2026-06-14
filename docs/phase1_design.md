# Phase 1: Procedural Maze & Player Implementation Plan

This document outlines the design and implementation steps for Phase 1 of Project Backrooms.

## 1. Maze Generation Algorithm: The Hybrid Approach

To produce a maze that contains a mix of wide office rooms and narrow, twisting corridors characteristic of the Backrooms.

### The Algorithm Steps & Complexity

1. **BSP (Binary Space Partitioning) for Rooms**
   - **How it works:** We start with the entire maze as one giant rectangular region. We pick a random axis (horizontal or vertical) and split the region into two smaller rectangles. We recursively split these smaller rectangles until they reach a minimum size threshold. Then, inside each "leaf" rectangle, we carve out a slightly smaller rectangular room. This creates non-overlapping "office rooms" distributed naturally across the map.
   - **Complexity:** O(R log R) where R is the number of rooms.

2. **Prim's Algorithm for Corridors**
   - **How it works:** We treat the empty space between the BSP rooms as solid rock. We pick a random starting cell next to a room, turn it into a floor corridor, and add all its adjacent walls to a list. We then loop: randomly pick a wall from the list. If the wall divides a floor cell and an unvisited rock cell, we break the wall (turn it to a floor) and add its unvisited rock neighbors to the list. This "grows" the maze outward organically like a mold until all the rooms are connected by a chaotic web of highly-branched, confusing corridors.
   - **Complexity:** O(V log V) where V is the number of grid cells.

3. **Loop Generation (Breaking the Tree)**
   - **How it works:** Prim's algorithm generates a "perfect maze" with zero loops (a spanning tree). To introduce confusion and cycles, we perform a final pass. We iterate over every remaining wall cell in the maze. For each wall, we give it a small random chance (e.g., 3%) to be deleted and turned into a floor. This connects dead ends together and creates circular paths.
   - **Complexity:** O(V) as we iterate over the grid once.

4. **Tunnel Borer (Ensuring 100% Connectivity)**
   - **How it works:** Even after Prim's and Loop Generation, some BSP rooms might remain completely isolated if the mold never reached them. To fix this, we use a two-step graph traversal approach. First, we run a **Flood-Fill BFS (Breadth-First Search)** starting from any open floor to map out the "main network" of connected rooms and corridors. Second, if we find any room that wasn't reached by the flood-fill, we run a **Pathfinding BFS** starting from that isolated room. Crucially, this pathfinding BFS is allowed to walk through solid rock. Once it collides with the main network, we trace the shortest path backwards and carve a straight tunnel through the rock to connect them. We repeat this process until zero isolated rooms remain.
   - **Complexity:** O(V) per isolated room, as both the Flood-Fill BFS and Pathfinding BFS traverse the grid cells (vertices) at most once per pass.

### Overall Maze Generation Complexity
Combining all four algorithms gives us the total time complexity for generating a full Backrooms level:
- **Best Case (Lower Bound): Ω(V).** If BSP splits perfectly and the Tunnel Borer finds zero isolated islands on its first scan, every step scales linearly with the number of grid cells (V). We are mathematically certain it can never run faster than this, as we must at least visit every cell once.
- **Worst Case (Upper Bound): O(V²).** In a pathological edge-case where the initial generation algorithms somehow create hundreds of completely isolated 1x1 rooms (proportional to V), the Tunnel Borer's `while` loop would execute O(V) times, running a full O(V) BFS on each iteration. We are certain it will never be worse than this quadratic time limit.
- **Average/Common Case:** In typical runs, the algorithms remain strictly linear. Because Prim's algorithm naturally creates a highly-connected web, the Tunnel Borer rarely finds more than 0 to 2 isolated rooms. Running a BFS a constant number of times means the average everyday performance heavily skews toward our lower bound, running in **O(V)** time.

## 2. Core Data Structures: Implicit Grid Graph

The maze graph will be represented implicitly on a 2D grid. We do not use explicit node objects with pointers to neighbors. Instead, the edges are implied by the geometry of the grid (e.g., cell (x, y) is connected to (x+1, y)).

#### [NEW] `src/maze.hpp` and `src/maze.cpp`
- **`std::vector<int> grid`**: The 2D implicit graph will be stored using a 1D flat array to maximize cache locality and performance.
- **Mapping Formula:** `index = y * width + x`
  - **How it works:** Imagine a grid of width 5. The first row (y=0) has indices 0, 1, 2, 3, 4. The second row (y=1) has indices 5, 6, 7, 8, 9. Every time we move down one row (y increases by 1), we skip an entire row of `width` cells. We then add `x` to move horizontally within that row. This formula mathematically translates a 2D coordinate into a single 1D memory location.

## 3. Player and Movement

#### [NEW] `src/player.hpp` and `src/player.cpp`
- **State Data:**
  - **`Vector2 position`**: A Raylib struct holding `float x` and `float y` for smooth movement.
  - **`float speed`**: Defined in units per second.
  - **`float radius`**: Used for collision bounding.

- **WASD Input & Kinematics:**
  - We will use Raylib's input functions (e.g., `IsKeyDown(KEY_W)`) to poll keyboard state every frame.
  - If a key is pressed, we create a velocity vector. For example, pressing 'W' sets `velocity.y = -speed`, and pressing 'A' sets `velocity.x = -speed`.

- **Framerate Independence (Delta Time):**
  - We multiply the velocity by Raylib's `GetFrameTime()` (Delta Time).
  - Formula: `position.x += velocity.x * deltaTime`.
  - This ensures the player moves exactly the same distance per real-world second, regardless of whether the game is running at 30 FPS or 144 FPS.

- **Collision Implementation (The "Slide" Method):**
  - We do not update the X and Y positions simultaneously.
  - First, we add the X velocity to the position and check if the player overlaps a wall (using Circle-to-AABB math). If they overlap, we push the X position back out of the wall.
  - Second, we add the Y velocity and do the same check.
  - **Why?** Handling axes separately allows the player to smoothly "slide" along a wall if they press diagonally into it, rather than getting completely stuck.

## 4. Game State and Dynamic Camera

#### [MODIFY] `src/main.cpp`
- Instantiate `Maze` and `Player`.
- Setup a **`Camera2D`** (Raylib struct) to track the player's position.
- **Massive Scale:** We will increase the maze size by 6-7x. The entire maze will no longer fit on a single screen. The camera must dynamically pan to keep the player centered, giving a true sense of scale and exploration.
- Implement the basic game loop: input -> update -> begin drawing -> begin camera -> render maze -> render player -> end camera -> end drawing.

---
