# Phase 1: Procedural Maze & Player Implementation Plan

This document outlines the design and implementation steps for Phase 1 of Project Backrooms.

## 1. Maze Generation Algorithm: The Hybrid Approach

To achieve the non-Euclidean looping geometry and a mix of wide office rooms and narrow corridors characteristic of the Backrooms, we will implement a **Hybrid BSP + Prim's Algorithm**.

### Live Visualizer (Debug Mode)
To help us test, visualize, and learn the algorithms, the maze generation will not happen instantly behind the scenes. We will build a **Step-by-Step Visualizer** using the Dear ImGui overlay from Phase 0.
- We will add UI buttons to trigger each step manually: "Step 1: BSP", "Step 2: Prim's", "Step 3: Loops".
- The algorithms will be written to execute progressively (e.g., carving one Prim's wall per frame) so you can watch the maze organically grow and carve itself out live on the screen.
- *Note: This visualizer GUI will be moved to a dedicated "Testing Mode" and hidden during normal gameplay in Phase 9.*

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

## 4. Game State and Camera

#### [MODIFY] `src/main.cpp`
- Instantiate `Maze` and `Player`.
- Setup a `Camera2D` (Raylib struct) to track the player's position.
- Implement the basic game loop: input -> update -> begin drawing -> begin camera -> render maze -> render player -> end camera -> end drawing.
- *Note: We will test the maze with standard hard boundaries at this stage to ensure generation and movement work perfectly before adding infinite wrapping.*

## 5. Toroidal Wrapping (The "Infinite" Illusion)

*To be implemented ONLY AFTER the standard maze, player movement, and camera are fully tested and working.*

To create the illusion of an infinitely generating maze without consuming infinite memory, we will implement Toroidal Wrapping on our fixed-size grid. 

- **Mathematical Implementation:** Position calculations will use the modulo operator (`%`), which returns the remainder of a division. 
  - **How it works:** If our maze `width = 100`, valid X coordinates are 0 to 99. If the player moves right from `x = 99`, their new X becomes `100`. We calculate `100 % 100`, which equals `0`. The player instantly teleports to the left edge. 
  - **Negative Wrapping:** If moving left from `x = 0`, X becomes `-1`. In C++, we use `(x % width + width) % width` to wrap negative numbers. `(-1 % 100 + 100) % 100 = 99`. The player teleports to the right edge!
- **Graph Topology:** This transforms our flat grid into a continuous torus (donut). The graph has no boundary walls, and every cell has exactly 4 neighbors. This allows pathfinding algorithms (like BFS) to seamlessly wrap around the map.

---

## Verification Plan
### Manual Verification
- Compile and run the game.
- Use the ImGui Debugger to step-by-step execute BSP, Prim's, and Loop injection. Verify each stage draws correctly.
- Verify the player can move smoothly using WASD.
- Verify the player slides against walls instead of getting stuck on corners.
- Verify the camera follows the player.
- **Final Step:** Enable Toroidal mapping and verify walking past the map edge seamlessly wraps to the other side visually and physically.
