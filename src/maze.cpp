#include "maze.hpp"

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
    : m_width(width)
    , m_height(height)
    , m_cellSize(cellSize)
    , m_grid(width * height, CELL_WALL)  // Fill every cell with CELL_WALL (0)
    , m_rng(seed)                        // Initialize RNG with the fixed seed
{
}

// ============================================================================
// BSP ALGORITHM IMPLEMENTATION
// ============================================================================

// Slices a block of cookie dough (leaf) into two smaller blocks.
bool Maze::BSPLeaf::split(std::mt19937& rng) {
    // If we've already split, bail out.
    if (leftChild || rightChild) return false;

    // Determine direction of split (Horizontal vs Vertical)
    // If the block is 25% wider than it is tall, force vertical split.
    // If it is 25% taller than it is wide, force horizontal split.
    // Otherwise, flip a 50/50 coin.
    bool splitH = std::uniform_int_distribution<>(0, 1)(rng) == 0;
    if (width > height && (float)width / height >= 1.25f) splitH = false;
    else if (height > width && (float)height / width >= 1.25f) splitH = true;

    // Minimum size of a leaf so rooms aren't microscopically tiny
    const int MIN_LEAF_SIZE = 6; 
    
    // Determine the maximum length we can split at
    int max = (splitH ? height : width) - MIN_LEAF_SIZE;
    if (max <= MIN_LEAF_SIZE) return false; // Block is too small to slice anymore!

    // Pick a random line to slice on
    int splitPoint = std::uniform_int_distribution<>(MIN_LEAF_SIZE, max)(rng);

    // Create the two children (the two smaller blocks of dough)
    if (splitH) {
        leftChild = std::make_shared<BSPLeaf>(x, y, width, splitPoint);
        rightChild = std::make_shared<BSPLeaf>(x, y + splitPoint, width, height - splitPoint);
    } else {
        leftChild = std::make_shared<BSPLeaf>(x, y, splitPoint, height);
        rightChild = std::make_shared<BSPLeaf>(x + splitPoint, y, width - splitPoint, height);
    }

    return true; // Slice successful!
}

// Recursively walks down the BSP tree. When it hits the bottom (leaves), it carves a room.
void Maze::BSPLeaf::createRooms(Maze& maze, std::mt19937& rng) {
    // If this leaf has children, it's not a "final" leaf. Tell the children to do the work.
    if (leftChild || rightChild) {
        if (leftChild) leftChild->createRooms(maze, rng);
        if (rightChild) rightChild->createRooms(maze, rng);
        return;
    }

    // This is a final leaf! Let's carve a physical room inside it.
    // We want rooms to be randomly sized, but fit inside the leaf with a 1-cell padding (walls).
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
    // 1. Create the root leaf covering the entire map (with a 1-cell indestructible outer border)
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
                // To prevent boring perfection, we sometimes randomly stop splitting early (25% chance)
                // Unless the block is still massive (>20), then we force it to split.
                if (leaf->width > 20 || leaf->height > 20 || std::uniform_int_distribution<>(0, 99)(m_rng) > 25) {
                    if (leaf->split(m_rng)) {
                        m_leaves.push_back(leaf->leftChild);
                        m_leaves.push_back(leaf->rightChild);
                        didSplit = true;
                    }
                }
            }
        }
    }

    // 3. Once all slicing is completely done, ask the root leaf to carve the physical rooms!
    m_rootLeaf->createRooms(*this, m_rng);
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
int Maze::getIndex(int x, int y) const {
    return y * m_width + x;
}

// ============================================================================
// getCell — Read a Cell
// ============================================================================
// Returns the cell type (CELL_WALL, CELL_FLOOR, or CELL_ROOM) at position (x, y).
// If the coordinates are out of bounds, we return CELL_WALL as a safety measure
// so anything outside the grid acts like a solid wall.
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
//   maze.setCell(5, 10, Maze::CELL_FLOOR);  // Turn cell (5,10) into walkable floor
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
// The parenthesis ')' means exclusive — e.g., for width=100, valid x is 0 to 99.
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
                    color = Color{ 194, 178, 128, 255 }; // Sandy yellow — corridor floor
                    break;
                case CELL_ROOM:
                    color = Color{ 210, 190, 140, 255 };  // Slightly lighter — office room
                    break;
                case CELL_WALL:
                default:
                    color = Color{ 40, 35, 30, 255 };     // Dark brown-grey — solid wall
                    break;
            }

            // Draw a filled rectangle at the correct pixel position.
            // Grid cell (x, y) maps to pixel position (x * cellSize, y * cellSize).
            DrawRectangle(
                x * m_cellSize,    // Pixel X position
                y * m_cellSize,    // Pixel Y position
                m_cellSize,        // Width in pixels
                m_cellSize,        // Height in pixels
                color              // Fill color
            );
        }
    }
}
