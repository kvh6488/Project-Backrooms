#include "maze.hpp"
#include "raylib.h"

// ============================================================================
// Constructor
// ============================================================================
// Creates a width x height grid where every cell starts as a solid wall.
// Think of it like starting with a massive block of solid rock before we
// carve rooms and corridors out of it.
//
// The 1D array is sized to hold width * height integers.
// For a 100x100 maze, that's 10,000 integers = ~40 KB of contiguous memory.
Maze::Maze(int width, int height, int cellSize, unsigned int seed)
    : m_width(width), m_height(height), m_cellSize(cellSize),
      m_grid(width * height, CELL_WALL) // Fill every cell with CELL_WALL (0)
      ,
      m_rng(seed) // Initialize RNG with the fixed seed
{}

// ============================================================================
// BSP ALGORITHM IMPLEMENTATION
// ============================================================================

// Slices a block of cookie dough (leaf) into two smaller blocks.
bool Maze::BSPLeaf::split(std::mt19937 &rng) {
  // If we've already split, bail out.
  if (leftChild || rightChild)
    return false;

  // Determine direction of split (Horizontal vs Vertical)
  // If the block is 25% wider than it is tall, force vertical split.
  // If it is 25% taller than it is wide, force horizontal split.
  // Otherwise, flip a 50/50 coin.
  bool splitH = std::uniform_int_distribution<>(0, 1)(rng) == 0;
  if (width > height && (float)width / height >= 1.25f)
    splitH = false;
  else if (height > width && (float)height / width >= 1.25f)
    splitH = true;

  // Minimum size of a leaf so rooms aren't microscopically tiny
  const int MIN_LEAF_SIZE = 6;

  // Determine the maximum length we can split at
  int max = (splitH ? height : width) - MIN_LEAF_SIZE;
  if (max <= MIN_LEAF_SIZE)
    return false; // Block is too small to slice anymore!

  // Pick a random line to slice on
  int splitPoint = std::uniform_int_distribution<>(MIN_LEAF_SIZE, max)(rng);

  // Create the two children (the two smaller blocks of dough)
  if (splitH) {
    leftChild = std::make_shared<BSPLeaf>(x, y, width, splitPoint);
    rightChild = std::make_shared<BSPLeaf>(x, y + splitPoint, width,
                                           height - splitPoint);
  } else {
    leftChild = std::make_shared<BSPLeaf>(x, y, splitPoint, height);
    rightChild = std::make_shared<BSPLeaf>(x + splitPoint, y,
                                           width - splitPoint, height);
  }

  return true; // Slice successful!
}

// Recursively walks down the BSP tree. When it hits the bottom (leaves), it
// carves a room.
void Maze::BSPLeaf::createRooms(Maze &maze, std::mt19937 &rng) {
  // If this leaf has children, it's not a "final" leaf. Tell the children to do
  // the work.
  if (leftChild || rightChild) {
    if (leftChild)
      leftChild->createRooms(maze, rng);
    if (rightChild)
      rightChild->createRooms(maze, rng);
    return;
  }

  // This is a final leaf! Let's carve a physical room inside it.
  // We want rooms to be randomly sized, but fit inside the leaf with a 1-cell
  // padding (walls).
  roomWidth = std::uniform_int_distribution<>(4, width - 2)(rng);
  roomHeight = std::uniform_int_distribution<>(4, height - 2)(rng);

  // Place the room randomly within the leaf, respecting the 1-cell padding
  roomX = x + std::uniform_int_distribution<>(1, width - roomWidth - 1)(rng);
  roomY = y + std::uniform_int_distribution<>(1, height - roomHeight - 1)(rng);

  // Carve the actual tiles into the 1D Array!
  for (int iy = roomY; iy < roomY + roomHeight; ++iy) {
    for (int ix = roomX; ix < roomX + roomWidth; ++ix) {
      maze.setCell(ix, iy, Maze::CELL_ROOM);
    }
  }

  hasRoom = true;
}

// The master function that runs the entire BSP phase.
void Maze::generateBSP() {
  // 1. Create the root leaf covering the entire map (with a 1-cell
  // indestructible outer border)
  m_rootLeaf = std::make_shared<BSPLeaf>(1, 1, m_width - 2, m_height - 2);
  m_leaves.push_back(m_rootLeaf);

  // 2. Loop through all leaves and split them continuously
  bool didSplit = true;
  while (didSplit) {
    didSplit = false;

    // We iterate by index because m_leaves.size() will grow as we add children!
    int currentSize = m_leaves.size();
    for (int i = 0; i < currentSize; i++) {
      auto leaf = m_leaves[i];

      // Only split leaves that haven't been split yet
      if (leaf->leftChild == nullptr && leaf->rightChild == nullptr) {
        // To prevent boring perfection, we sometimes randomly stop splitting
        // early (25% chance) Unless the block is still massive (>20), then we
        // force it to split.
        if (leaf->width > 20 || leaf->height > 20 ||
            std::uniform_int_distribution<>(0, 99)(m_rng) > 25) {
          if (leaf->split(m_rng)) {
            m_leaves.push_back(leaf->leftChild);
            m_leaves.push_back(leaf->rightChild);
            didSplit = true;
          }
        }
      }
    }
  }

  // 3. Once all slicing is completely done, ask the root leaf to carve the
  // physical rooms!
  m_rootLeaf->createRooms(*this, m_rng);

  // 4. Occasionally merge nearby rooms to create larger, L-shaped formations
  mergeAdjacentRooms();
}

// ============================================================================
// ROOM MERGING — Creating Combined Room Shapes
// ============================================================================
// After BSP carves isolated rectangular rooms, this pass looks for rooms that
// are separated by a thin wall (1-3 cells). With a ~25% chance, it smashes
// the dividing wall to merge the two rooms into a single larger shape.
//
// This creates the variety of L-shaped, T-shaped, and wide open spaces
// that make the Backrooms feel less like a grid of perfect rectangles.
//
// WHY before Prim's? If we merge rooms BEFORE corridor generation, Prim's
// algorithm will naturally grow corridors around the new combined shapes
// without any special handling.
//
// COMPLEXITY: O(R^2) where R = number of rooms (typically 10-20, so trivial)
// ============================================================================

void Maze::mergeAdjacentRooms() {
  for (size_t i = 0; i < m_leaves.size(); ++i) {
    auto &a = m_leaves[i];
    if (!a->hasRoom)
      continue;

    for (size_t j = i + 1; j < m_leaves.size(); ++j) {
      auto &b = m_leaves[j];
      if (!b->hasRoom)
        continue;

      // Only merge ~25% of adjacent room pairs for variety
      if (std::uniform_int_distribution<>(0, 99)(m_rng) >= 25)
        continue;

      // --- Check if Room A is directly LEFT of Room B ---
      // Gap = distance from A's right edge to B's left edge
      int gapAB = b->roomX - (a->roomX + a->roomWidth);
      if (gapAB >= 1 && gapAB <= 3) {
        // Find the vertical overlap between the two rooms
        int yStart = std::max(a->roomY, b->roomY);
        int yEnd = std::min(a->roomY + a->roomHeight, b->roomY + b->roomHeight);
        if (yEnd > yStart) {
          // There IS vertical overlap! Smash all wall cells in the gap.
          for (int y = yStart; y < yEnd; ++y) {
            for (int x = a->roomX + a->roomWidth; x < b->roomX; ++x) {
              setCell(x, y, CELL_ROOM);
            }
          }
          continue; // Move on to the next pair
        }
      }

      // --- Check if Room B is directly LEFT of Room A ---
      int gapBA = a->roomX - (b->roomX + b->roomWidth);
      if (gapBA >= 1 && gapBA <= 3) {
        int yStart = std::max(a->roomY, b->roomY);
        int yEnd = std::min(a->roomY + a->roomHeight, b->roomY + b->roomHeight);
        if (yEnd > yStart) {
          for (int y = yStart; y < yEnd; ++y) {
            for (int x = b->roomX + b->roomWidth; x < a->roomX; ++x) {
              setCell(x, y, CELL_ROOM);
            }
          }
          continue;
        }
      }

      // --- Check if Room A is directly ABOVE Room B ---
      int gapAbove = b->roomY - (a->roomY + a->roomHeight);
      if (gapAbove >= 1 && gapAbove <= 3) {
        int xStart = std::max(a->roomX, b->roomX);
        int xEnd = std::min(a->roomX + a->roomWidth, b->roomX + b->roomWidth);
        if (xEnd > xStart) {
          for (int x = xStart; x < xEnd; ++x) {
            for (int y = a->roomY + a->roomHeight; y < b->roomY; ++y) {
              setCell(x, y, CELL_ROOM);
            }
          }
          continue;
        }
      }

      // --- Check if Room B is directly ABOVE Room A ---
      int gapBelow = a->roomY - (b->roomY + b->roomHeight);
      if (gapBelow >= 1 && gapBelow <= 3) {
        int xStart = std::max(a->roomX, b->roomX);
        int xEnd = std::min(a->roomX + a->roomWidth, b->roomX + b->roomWidth);
        if (xEnd > xStart) {
          for (int x = xStart; x < xEnd; ++x) {
            for (int y = b->roomY + b->roomHeight; y < a->roomY; ++y) {
              setCell(x, y, CELL_ROOM);
            }
          }
          continue;
        }
      }
    }
  }
}

// ============================================================================
// PRIM'S ALGORITHM IMPLEMENTATION (Corridor Generation)
// ============================================================================
// Grows organic, branching corridors through the solid rock using a modified
// version of Randomized Prim's Algorithm.
//
// THE MOLD ANALOGY:
// Imagine dropping a spore of mold onto the edge of a BSP room. The mold
// looks at the solid rock walls around it (the "frontier"). Each frame, it
// randomly picks one of those walls and checks: "If I eat through this wall,
// will I create a loop?" If not, it eats the wall and adds the NEW rock
// neighbors to its frontier list. The mold keeps spreading until its frontier
// list is completely empty — meaning it has nowhere left to grow.
//
// TIME COMPLEXITY:  O(V) where V = total grid cells.
//   Each cell is added to the frontier at most once and removed at most once.
//   We use a swap-and-pop trick for O(1) removal from the frontier vector.
//
// SPACE COMPLEXITY: O(V) for the frontier vector in the worst case.
// ============================================================================

void Maze::generateCorridors() {
  // The frontier: a list of wall cells that are candidates for being smashed.
  // We store them as flat 1D indices into our m_grid array.
  std::vector<int> frontier;

  // --- STEP 1: Find the Seed ---
  // We need a guaranteed safe starting point. We loop through the BSP leaves
  // and find the first room. We then pick a wall cell adjacent to that room's
  // edge to serve as the mold's "spore" — the first cell it will try to eat.
  int seedX = -1, seedY = -1;
  for (const auto &leaf : m_leaves) {
    if (leaf->hasRoom) {
      // Pick the wall cell directly to the left of the room's top-left corner.
      // We subtract 1 from roomX to step outside the room into solid rock.
      seedX = leaf->roomX - 1;
      seedY = leaf->roomY;

      // Safety check: make sure the seed is actually inside the grid and is
      // actually a wall (not the outer border or another room)
      if (isInBounds(seedX, seedY) && getCell(seedX, seedY) == CELL_WALL) {
        break; // Found a valid seed!
      }

      // If the left side didn't work, try the right side of the room
      seedX = leaf->roomX + leaf->roomWidth;
      if (isInBounds(seedX, seedY) && getCell(seedX, seedY) == CELL_WALL) {
        break;
      }

      // If neither worked, try below
      seedX = leaf->roomX;
      seedY = leaf->roomY + leaf->roomHeight;
      if (isInBounds(seedX, seedY) && getCell(seedX, seedY) == CELL_WALL) {
        break;
      }

      // Reset and try the next leaf
      seedX = -1;
      seedY = -1;
    }
  }

  // If we somehow couldn't find a seed (shouldn't happen), bail out safely
  if (seedX == -1 || seedY == -1)
    return;

  // --- STEP 2: Plant the Seed ---
  // Turn the seed wall cell into a corridor floor tile.
  setCell(seedX, seedY, CELL_FLOOR);

  // The 4 cardinal directions: Right, Left, Down, Up
  // We will reuse this array constantly to check neighbors.
  const int dx[] = {1, -1, 0, 0};
  const int dy[] = {0, 0, 1, -1};

  // Add all solid-rock neighbors of the seed to the frontier.
  for (int d = 0; d < 4; ++d) {
    int nx = seedX + dx[d];
    int ny = seedY + dy[d];
    if (isInBounds(nx, ny) && getCell(nx, ny) == CELL_WALL) {
      frontier.push_back(getIndex(nx, ny));
    }
  }

  // --- STEP 3: The Growth Loop ---
  // Keep growing until the frontier is completely empty.
  while (!frontier.empty()) {
    // Pick a random wall from the frontier list.
    // We use the "swap and pop" trick for O(1) removal:
    //   1. Generate a random index into the frontier vector
    //   2. Swap that element with the LAST element
    //   3. Pop the last element off (which is O(1) for vectors)
    // This avoids the O(N) cost of erasing from the middle of a vector.
    int randIdx =
        std::uniform_int_distribution<>(0, (int)frontier.size() - 1)(m_rng);
    int wallIndex = frontier[randIdx];

    // Swap-and-pop: move our chosen wall to the back, then remove it
    frontier[randIdx] = frontier.back();
    frontier.pop_back();

    // Convert the 1D index back to 2D coordinates
    // This is the reverse of our getIndex formula: index = y * width + x
    //   x = index % width  (the remainder gives us the column)
    //   y = index / width   (integer division gives us the row)
    int wx = wallIndex % m_width;
    int wy = wallIndex / m_width;

    // Skip if this cell is no longer a wall (it may have been carved by
    // a previous iteration, or it might be part of a BSP room)
    if (getCell(wx, wy) != CELL_WALL)
      continue;

    // --- STEP 4: The Check ---
    // Count how many of this wall's 4 neighbors are already carved
    // (floor or room tiles). If exactly 1 neighbor is carved, it's safe
    // to smash this wall without creating a loop.
    int carvedNeighborCount = 0;
    for (int d = 0; d < 4; ++d) {
      int nx = wx + dx[d];
      int ny = wy + dy[d];
      if (isInBounds(nx, ny)) {
        int neighborType = getCell(nx, ny);
        if (neighborType == CELL_FLOOR || neighborType == CELL_ROOM) {
          ++carvedNeighborCount;
        }
      }
    }

    // If 2+ neighbors are already carved, smashing this wall would merge
    // two separate corridors together, creating a loop. Prim's algorithm
    // strictly forbids loops, so we discard this wall and move on.
    if (carvedNeighborCount != 1)
      continue;

    // --- STEP 4b: Diagonal Leak Check ---
    // Even if the cardinal check passed, we must also check the 4 DIAGONAL
    // neighbors. If a diagonal neighbor is carved, but the two cells sharing
    // that corner are both walls, then carving this cell would create a
    // "diagonal-only" connection:
    //
    //   FLOOR  WALL          FLOOR  WALL
    //   WALL   WALL    →     WALL   FLOOR   ← BAD! Diagonal leak!
    //
    // This looks ugly now and will be impossible to render clean wall
    // corners for later when we use proper textures and thin walls.
    bool hasDiagonalLeak = false;
    // The 4 diagonal directions: (+1,+1), (+1,-1), (-1,+1), (-1,-1)
    const int diagX[] = {1, 1, -1, -1};
    const int diagY[] = {1, -1, 1, -1};
    for (int d = 0; d < 4; ++d) {
      int dnx = wx + diagX[d]; // Diagonal neighbor X
      int dny = wy + diagY[d]; // Diagonal neighbor Y
      if (isInBounds(dnx, dny)) {
        int diagType = getCell(dnx, dny);
        if (diagType == CELL_FLOOR || diagType == CELL_ROOM) {
          // The diagonal is carved. Check the two cells sharing this corner:
          //   (wx + diagX[d], wy) and (wx, wy + diagY[d])
          // If BOTH are walls, we'd have a diagonal-only connection. Bad!
          int sharedA = getCell(wx + diagX[d], wy);
          int sharedB = getCell(wx, wy + diagY[d]);
          if (sharedA == CELL_WALL && sharedB == CELL_WALL) {
            hasDiagonalLeak = true;
            break; // No need to check the other diagonals
          }
        }
      }
    }
    if (hasDiagonalLeak)
      continue;

    // --- STEP 5: Smash the Wall! ---
    // This wall has exactly 1 carved neighbor, so it's safe to eat.
    // Turn it into a corridor floor tile.
    setCell(wx, wy, CELL_FLOOR);

    // Add all of THIS wall's solid-rock neighbors to the frontier.
    // These are the new candidates for the mold to eat next frame.
    for (int d = 0; d < 4; ++d) {
      int nx = wx + dx[d];
      int ny = wy + dy[d];
      if (isInBounds(nx, ny) && getCell(nx, ny) == CELL_WALL) {
        frontier.push_back(getIndex(nx, ny));
      }
    }
  }

  // --- STEP 6: Punch Doorways ---
  // Prim's algorithm refuses to smash any wall that has 2+ carved neighbors
  // (to prevent loops). But this means the walls BETWEEN rooms and corridors
  // never get smashed — because one side is CELL_ROOM and the other side is
  // CELL_FLOOR, giving the wall 2 carved neighbors.
  //
  // To fix this, we scan the border of every BSP room. For each room, we
  // collect all wall cells that sit between the room and a corridor, then
  // randomly pick one to punch open as a doorway.
  for (const auto &leaf : m_leaves) {
    if (!leaf->hasRoom)
      continue;

    // Collect all candidate doorway positions for this room
    std::vector<std::pair<int, int>> doorCandidates;

    // Scan the 4 borders of the room looking for wall cells adjacent to corridors
    for (int ix = leaf->roomX; ix < leaf->roomX + leaf->roomWidth; ++ix) {
      // Check the wall cell directly ABOVE the room (roomY - 1)
      int wy = leaf->roomY - 1;
      if (isInBounds(ix, wy) && getCell(ix, wy) == CELL_WALL) {
        // Is there a corridor on the OTHER side of this wall? (one more step up)
        if (isInBounds(ix, wy - 1) && getCell(ix, wy - 1) == CELL_FLOOR) {
          doorCandidates.push_back({ix, wy});
        }
      }
      // Check the wall cell directly BELOW the room (roomY + roomHeight)
      wy = leaf->roomY + leaf->roomHeight;
      if (isInBounds(ix, wy) && getCell(ix, wy) == CELL_WALL) {
        if (isInBounds(ix, wy + 1) && getCell(ix, wy + 1) == CELL_FLOOR) {
          doorCandidates.push_back({ix, wy});
        }
      }
    }
    for (int iy = leaf->roomY; iy < leaf->roomY + leaf->roomHeight; ++iy) {
      // Check the wall cell directly LEFT of the room (roomX - 1)
      int wx = leaf->roomX - 1;
      if (isInBounds(wx, iy) && getCell(wx, iy) == CELL_WALL) {
        if (isInBounds(wx - 1, iy) && getCell(wx - 1, iy) == CELL_FLOOR) {
          doorCandidates.push_back({wx, iy});
        }
      }
      // Check the wall cell directly RIGHT of the room (roomX + roomWidth)
      wx = leaf->roomX + leaf->roomWidth;
      if (isInBounds(wx, iy) && getCell(wx, iy) == CELL_WALL) {
        if (isInBounds(wx + 1, iy) && getCell(wx + 1, iy) == CELL_FLOOR) {
          doorCandidates.push_back({wx, iy});
        }
      }
    }

    // If we found any candidates, punch a random one open as a doorway
    if (!doorCandidates.empty()) {
      int pick = std::uniform_int_distribution<>(
          0, (int)doorCandidates.size() - 1)(m_rng);
      setCell(doorCandidates[pick].first, doorCandidates[pick].second,
              CELL_FLOOR);
    }
  }
}

// ============================================================================
// getIndex — The Heart of 1D-to-2D Mapping
// ============================================================================
// Converts a 2D coordinate (x, y) into a 1D array index.
//
// Imagine our grid laid out like rows of a spreadsheet:
//   Row 0 (y=0): indices [0, 1, 2, 3, 4]       (if width = 5)
//   Row 1 (y=1): indices [5, 6, 7, 8, 9]
//   Row 2 (y=2): indices [10, 11, 12, 13, 14]
//
// To get to row y, we skip y full rows of 'width' cells each: y * width
// Then we move x cells to the right within that row: + x
//
// Example: getIndex(3, 2) on a width=5 grid
//   = 2 * 5 + 3
//   = 10 + 3
//   = 13  ← That's the correct index in our 1D array!
int Maze::getIndex(int x, int y) const { return y * m_width + x; }

// ============================================================================
// getCell — Read a Cell
// ============================================================================
// Returns the cell type (CELL_WALL, CELL_FLOOR, or CELL_ROOM) at position (x,
// y). If the coordinates are out of bounds, we return CELL_WALL as a safety
// measure so anything outside the grid acts like a solid wall.
int Maze::getCell(int x, int y) const {
  if (!isInBounds(x, y)) {
    return CELL_WALL; // Out-of-bounds = treat as wall
  }
  return m_grid[getIndex(x, y)];
}

// ============================================================================
// setCell — Write a Cell
// ============================================================================
// Changes the cell type at position (x, y).
// This is how the generation algorithms will "carve" floor out of solid rock:
//   maze.setCell(5, 10, Maze::CELL_FLOOR);  // Turn cell (5,10) into walkable
//   floor
void Maze::setCell(int x, int y, int cellType) {
  if (!isInBounds(x, y)) {
    return; // Silently ignore out-of-bounds writes
  }
  m_grid[getIndex(x, y)] = cellType;
}

// ============================================================================
// isInBounds — Boundary Check
// ============================================================================
// Returns true if (x, y) is inside the grid.
// x must be in range [0, width)  and y must be in range [0, height).
// The parenthesis ')' means exclusive — e.g., for width=100, valid x is 0
// to 99.
bool Maze::isInBounds(int x, int y) const {
  return x >= 0 && x < m_width && y >= 0 && y < m_height;
}

// ============================================================================
// render — Draw the Maze
// ============================================================================
// Loops through every cell in the grid and draws a colored rectangle.
//
// PERFORMANCE NOTE: Right now, this draws EVERY cell in the maze every frame.
// For a 100x100 grid that's 10,000 rectangles per frame. This is fine for now,
// but later we will optimize this to only draw cells visible in the camera
// viewport (called "frustum culling").
//
// Color coding (placeholder):
//   CELL_WALL  → Dark grey (solid rock)
//   CELL_FLOOR → Light yellow (the classic Backrooms carpet color)
//   CELL_ROOM  → Slightly different yellow (to distinguish office rooms)
void Maze::render() const {
  for (int y = 0; y < m_height; ++y) {
    for (int x = 0; x < m_width; ++x) {
      int cellType = m_grid[getIndex(x, y)];

      Color color;
      switch (cellType) {
      case CELL_FLOOR:
        color = Color{194, 178, 128, 255}; // Sandy yellow — corridor floor
        break;
      case CELL_ROOM:
        color = Color{210, 190, 140, 255}; // Slightly lighter — office room
        break;
      case CELL_WALL:
      default:
        color = Color{40, 35, 30, 255}; // Dark brown-grey — solid wall
        break;
      }

      // Draw a filled rectangle at the correct pixel position.
      // Grid cell (x, y) maps to pixel position (x * cellSize, y * cellSize).
      DrawRectangle(x * m_cellSize, // Pixel X position
                    y * m_cellSize, // Pixel Y position
                    m_cellSize,     // Width in pixels
                    m_cellSize,     // Height in pixels
                    color           // Fill color
      );
    }
  }
}
