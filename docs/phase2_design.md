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
Instead of mutating the maze when the player isn't looking, we will turn the entire Backrooms into a giant Toroidal Rubik's Cube!
- **Concept:** Circular Array Shifting.
- **Implementation:** We will add a blue "Bed" object taking up two tiles in the center of the map. When the player presses a button to sleep, the screen fades to black. While asleep, entire columns (slices of the X-axis) will be circularly shifted up or down the Y-axis. Later, we will expand this so entire rows (slices of the Y-axis) can shift along the X-axis. Because the grid wraps natively (Toroidal), shifting an entire column doesn't break the data structure—it just dramatically rearranges the maze layout!
- **Minimap Representation:** The `RenderTexture2D` used for the minimap must be flagged as "dirty" whenever the player sleeps. When the player wakes up, the game will do a single-frame redraw of the Minimap texture to reflect the newly shifted Rubik's chunks.

### Phase 2.4: Fog of War & Textures
To enhance the horror, players should only see a limited distance.
- **Concept:** Raycasting or Bresenham's Line Algorithm.
- **Implementation:** Cast rays from the player's position. If a ray hits a wall, the cells behind it are drawn black (or very dark). We will finally swap our solid colored squares for actual textures (carpet, wallpaper) using Raylib's `Texture2D`.

---
