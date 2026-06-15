# Phase 2: Dynamic Maze & Non-Euclidean Topology

Our goal in this phase is to break the player's grasp of physical reality. We will transform our standard 2D grid into an impossible, infinite-feeling space using advanced coordinate math and dynamic data structures.

## The Mathematics of Phase 2: Topology
In this phase, we are moving away from standard flat Euclidean geometry and diving into **Topology**—the mathematical study of properties that are preserved through continuous deformations, twistings, and stretchings of objects. 
Specifically, we will conceptually transform our flat 2D grid into a **Torus** (a donut shape). A flat 2D grid maps perfectly to a Torus if its top edge connects to its bottom edge, and its left edge connects to its right edge. By applying modulo arithmetic (`%`) to our grid access functions, we warp our mathematical space so that moving past the edge inherently wraps back around infinitely.

---

## Phase 2 Breakdown

### Phase 2.1: Toroidal Wrapping (The Math)
Currently, if the player hits the edge of the map, they hit a wall. In a **Toroidal** (donut-shaped) space, walking off the right edge seamlessly teleports you to the left edge.
- **Concept:** Modulo Arithmetic.
- **Implementation:** We will update `Maze::getIndex(x, y)` and `Maze::isInBounds(x, y)`. Instead of rejecting coordinates outside `[0, width)`, we will wrap them using `(x % width + width) % width`.
- **Generation:** Because the grid wraps natively, algorithms like `PrimsGenerator` and `TunnelBorer` will naturally wrap their paths across the edges of the map, organically connecting the far left to the far right.

### Phase 2.2: Seamless Rendering (Frustum Culling)
To render this infinite Toroidal space, we cannot simply draw the whole maze 9 times (which would take $337,500$ operations per frame and lag the game).
- **Concept:** Frustum Culling & Viewport Chunking.
- **Implementation:** We calculate the exact bounds of the camera's viewport in world space (the "Frustum"). We only iterate over the specific tiles that fall within that rectangle (e.g., a $40 \times 25$ chunk = $1000$ operations). If a coordinate falls outside the $250 \times 150$ bounds (e.g., $X=255$), we use modulo math to look up the data at cell 5, but we visually draw it at world coordinate 255.
- **Minimap Representation:** The Minimap will remain a static $250 \times 150$ render. However, because the math wraps, when the player walks off the right edge of the screen, the red tracking dot on the Minimap will instantly teleport to the far-left edge. This provides excellent visual confirmation to the player of the Toroidal topology!
- **Result:** $O(W \times H)$ time complexity per frame, scaling perfectly while creating a flawless infinite loop!

### Phase 2.3: The Rubik's Torus (Sleep Mutation)
Instead of mutating the maze when the player isn't looking, we turn the Backrooms into a giant, shifting puzzle!
- **Concept:** Localized Procedural Regeneration (Tic-Tac-Toe Slices).
- **Implementation:** When the player sleeps (or regenerates a zone), we calculate a "Tic-Tac-Toe" pattern composed of intersecting vertical and horizontal slices (currently 14 cells wide). To avoid the algorithms overwriting and corrupting each other at the intersections, we mathematically divide this pattern into 8 independent rectangular bounding boxes. We erase the contents of these zones, and strictly run our BSP, Prim's, and Loop algorithms inside each box. Finally, we run the Tunnel Borer (Breadth-First Search) to perfectly stitch these 8 new disconnected islands back into the main Torus graph seamlessly without producing illegal diagonal connections.
- **Time Complexity:** 
  - **Erasing & Carving:** $O(A \log A)$, where $A$ is the total combined area of the bounding boxes. Prim's algorithm dictates this upper bound due to its priority queue/frontier expansion.
  - **Stitching (Tunnel Borer):** $O(N)$ worst-case, where $N = W \times H$. The BFS must traverse the maze to verify connectivity and blast tunnels, which scales with the total area of the global maze.
  - **Overall:** The precise time complexity of a single mutation shift is **$O(N + A \log A)$**. Because we do not enforce how the regeneration area ($A$) scales relative to the total maze size ($N$), neither term strictly dominates. If the shifting zones cover a large percentage of the map, the $O(A \log A)$ carving dominates. If the shifting zones are kept small while the maze expands to infinity, the $O(N)$ BFS stitching dominates. In practice on a $250 \times 150$ grid, both execute nearly instantaneously.
- **Gameplay Integration:** The size and aggression of these shifting zones will be linked to the player's infection level. If the virus dominates, the shifting zones will expand; if the cure dominates, they will shrink.
- **Minimap Representation:** The `RenderTexture2D` used for the minimap is flagged as "dirty" whenever a zone regenerates. The game does a single-frame redraw of the Minimap texture to reflect the newly shifted chunks, painting the active regeneration zones red for visual clarity.

### Phase 2.4: Fog of War & Textures
To enhance the horror, players should only see a limited distance.
- **Concept:** Raycasting or Bresenham's Line Algorithm.
- **Implementation:** Cast rays from the player's position. If a ray hits a wall, the cells behind it are drawn black (or very dark). We will finally swap our solid colored squares for actual textures (carpet, wallpaper) using Raylib's `Texture2D` and add an updated player icon.

---
