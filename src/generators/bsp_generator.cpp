#include "bsp_generator.hpp"
#include <algorithm>
#include <queue>
#include <vector>

// ============================================================================
// split — Recursively divides the maze space into smaller rectangles (leaves).
// ============================================================================
bool BSPGenerator::BSPLeaf::split(std::mt19937 &rng) {
  // If we already split this leaf, we don't need to do it again.
  if (leftChild || rightChild)
    return false;

  // Randomly pick a direction to slice the rectangle: Horizontal or Vertical
  bool splitH = std::uniform_int_distribution<>(0, 1)(rng) == 0;

  // Aspect Ratio Enforcement:
  // If a room is significantly wider than it is tall, we FORCE a vertical
  // split. If it's taller than it is wide, we FORCE a horizontal split. This
  // prevents generating extremely long, skinny "hallway" leaves.
  if (width > height && (float)width / height >= 1.25f)
    splitH = false;
  else if (height > width && (float)height / width >= 1.25f)
    splitH = true;

  // We don't want leaves getting smaller than a certain threshold, otherwise
  // rooms can't fit inside them.
  const int MIN_LEAF_SIZE = 6;
  int max = (splitH ? height : width) - MIN_LEAF_SIZE;
  if (max <= MIN_LEAF_SIZE)
    return false;

  // Pick a random line to drop the slice across the rectangle
  int splitPoint = std::uniform_int_distribution<>(MIN_LEAF_SIZE, max)(rng);

  // Actually create the two child rectangles based on the split point
  if (splitH) {
    leftChild = std::make_shared<BSPLeaf>(x, y, width, splitPoint);
    rightChild = std::make_shared<BSPLeaf>(x, y + splitPoint, width,
                                           height - splitPoint);
  } else {
    leftChild = std::make_shared<BSPLeaf>(x, y, splitPoint, height);
    rightChild = std::make_shared<BSPLeaf>(x + splitPoint, y,
                                           width - splitPoint, height);
  }

  return true;
}

// ============================================================================
// createRooms — Traverses the BSP tree to the smallest leaves and carves rooms.
// ============================================================================
void BSPGenerator::BSPLeaf::createRooms(Maze &maze, std::mt19937 &rng) {
  // If this leaf has children, it's not a "bottom level" leaf. We must
  // recursively go deeper until we hit the bottom.
  if (leftChild || rightChild) {
    if (leftChild)
      leftChild->createRooms(maze, rng);
    if (rightChild)
      rightChild->createRooms(maze, rng);
    return;
  }

  roomWidth = std::uniform_int_distribution<>(4, width - 2)(rng);
  roomHeight = std::uniform_int_distribution<>(4, height - 2)(rng);

  roomX = x + std::uniform_int_distribution<>(1, width - roomWidth - 1)(rng);
  roomY = y + std::uniform_int_distribution<>(1, height - roomHeight - 1)(rng);

  for (int iy = roomY; iy < roomY + roomHeight; ++iy) {
    for (int ix = roomX; ix < roomX + roomWidth; ++ix) {
      maze.setCell(ix, iy, Maze::CELL_ROOM);
    }
  }

  hasRoom = true;
  maze.addRoom({roomX, roomY, roomWidth, roomHeight});
}

// ============================================================================
// generate — Core BSP Generation logic.
// ============================================================================
void BSPGenerator::generate(Maze &maze, std::mt19937 &rng) {
  // 1. Start with one giant leaf covering the entire maze (minus a border).
  m_rootLeaf = std::make_shared<BSPLeaf>(1, 1, maze.getWidth() - 2,
                                         maze.getHeight() - 2);
  m_leaves.push_back(m_rootLeaf);

  bool didSplit = true;
  // Keep iterating through the list of leaves until no more splits occur.
  while (didSplit) {
    didSplit = false;
    int currentSize = m_leaves.size();
    for (int i = 0; i < currentSize; i++) {
      auto leaf = m_leaves[i];
      // Only attempt to split if it's currently a bottom-level leaf
      if (leaf->leftChild == nullptr && leaf->rightChild == nullptr) {

        // If the leaf is very large (> 20), FORCE it to split.
        // If it's small but valid, give it a 75% chance to split.
        // This creates variance in room density.
        if (leaf->width > 20 || leaf->height > 20 ||
            std::uniform_int_distribution<>(0, 99)(rng) > 25) {
          if (leaf->split(rng)) {
            m_leaves.push_back(leaf->leftChild);
            m_leaves.push_back(leaf->rightChild);
            didSplit = true;
          }
        }
      }
    }
  }

  // 3. Convert leaves to rooms
  m_rootLeaf->createRooms(maze, rng);

  // Calculate middle room index for spawning
  float middleX = maze.getWidth() / 2.0f;
  float middleY = maze.getHeight() / 2.0f;
  float minDistance = 999999.0f;

  const auto &rooms = maze.getRooms();
  for (int i = 0; i < rooms.size(); ++i) {
    float roomCenterX = rooms[i].x + rooms[i].width / 2.0f;
    float roomCenterY = rooms[i].y + rooms[i].height / 2.0f;

    // distance squared
    float dx = roomCenterX - middleX;
    float dy = roomCenterY - middleY;
    float distSq = dx * dx + dy * dy;

    if (distSq < minDistance) {
      minDistance = distSq;
      m_middleRoomIndex = i;
    }
  }

  mergeAdjacentRooms(maze, rng);
  removeIsolatedWallPillars(maze);
  absorbThinPeninsulas(maze);
}

// ============================================================================
// generateZone — Runs BSP Generation but only inside a specific 2D slice
// ============================================================================
void BSPGenerator::generateZone(Maze &maze, std::mt19937 &rng, int startX,
                                int startY, int width, int height) {
  // Clear any old generation state
  m_leaves.clear();
  m_rootLeaf = nullptr;

  // 1. Start with one leaf covering ONLY the bounding box
  m_rootLeaf = std::make_shared<BSPLeaf>(startX, startY, width, height);
  m_leaves.push_back(m_rootLeaf);

  bool didSplit = true;
  while (didSplit) {
    didSplit = false;
    int currentSize = m_leaves.size();
    for (int i = 0; i < currentSize; i++) {
      auto leaf = m_leaves[i];
      if (leaf->leftChild == nullptr && leaf->rightChild == nullptr) {
        if (leaf->width > 20 || leaf->height > 20 ||
            std::uniform_int_distribution<>(0, 99)(rng) > 25) {
          if (leaf->split(rng)) {
            m_leaves.push_back(leaf->leftChild);
            m_leaves.push_back(leaf->rightChild);
            didSplit = true;
          }
        }
      }
    }
  }

  // 3. Convert leaves to rooms
  m_rootLeaf->createRooms(maze, rng);
}

// ============================================================================
// mergeAdjacentRooms — Optionally connects rooms that ended up very close.
// ============================================================================
void BSPGenerator::mergeAdjacentRooms(Maze &maze, std::mt19937 &rng) {
  // Compare every leaf against every other leaf.
  for (size_t i = 0; i < m_leaves.size(); ++i) {
    auto &a = m_leaves[i];
    if (!a->hasRoom)
      continue;

    for (size_t j = i + 1; j < m_leaves.size(); ++j) {
      auto &b = m_leaves[j];
      if (!b->hasRoom)
        continue;

      // There's only a 25% chance to merge even if they are close.
      // We don't want to merge everything, just create occasional large
      // composite rooms.
      if (std::uniform_int_distribution<>(0, 99)(rng) >= 25)
        continue;

      // Check if room B is strictly to the right of room A, with a small gap of
      // 1-3 cells
      int gapAB = b->roomX - (a->roomX + a->roomWidth);
      if (gapAB >= 1 && gapAB <= 3) {
        // Calculate vertical overlap
        int yStart = std::max(a->roomY, b->roomY);
        int yEnd = std::min(a->roomY + a->roomHeight, b->roomY + b->roomHeight);
        if (yEnd > yStart) {
          for (int y = yStart; y < yEnd; ++y) {
            for (int x = a->roomX + a->roomWidth; x < b->roomX; ++x) {
              maze.setCell(x, y, Maze::CELL_ROOM);
            }
          }
          continue;
        }
      }

      int gapBA = a->roomX - (b->roomX + b->roomWidth);
      if (gapBA >= 1 && gapBA <= 3) {
        int yStart = std::max(a->roomY, b->roomY);
        int yEnd = std::min(a->roomY + a->roomHeight, b->roomY + b->roomHeight);
        if (yEnd > yStart) {
          for (int y = yStart; y < yEnd; ++y) {
            for (int x = b->roomX + b->roomWidth; x < a->roomX; ++x) {
              maze.setCell(x, y, Maze::CELL_ROOM);
            }
          }
          continue;
        }
      }

      int gapAbove = b->roomY - (a->roomY + a->roomHeight);
      if (gapAbove >= 1 && gapAbove <= 3) {
        int xStart = std::max(a->roomX, b->roomX);
        int xEnd = std::min(a->roomX + a->roomWidth, b->roomX + b->roomWidth);
        if (xEnd > xStart) {
          for (int x = xStart; x < xEnd; ++x) {
            for (int y = a->roomY + a->roomHeight; y < b->roomY; ++y) {
              maze.setCell(x, y, Maze::CELL_ROOM);
            }
          }
          continue;
        }
      }

      int gapBelow = a->roomY - (b->roomY + b->roomHeight);
      if (gapBelow >= 1 && gapBelow <= 3) {
        int xStart = std::max(a->roomX, b->roomX);
        int xEnd = std::min(a->roomX + a->roomWidth, b->roomX + b->roomWidth);
        if (xEnd > xStart) {
          for (int x = xStart; x < xEnd; ++x) {
            for (int y = b->roomY + b->roomHeight; y < a->roomY; ++y) {
              maze.setCell(x, y, Maze::CELL_ROOM);
            }
          }
          continue;
        }
      }
    }
  }
}

// ============================================================================
// removeIsolatedWallPillars — Removes small artifacts of walls inside rooms.
// ============================================================================
void BSPGenerator::removeIsolatedWallPillars(Maze &maze) {
  int width = maze.getWidth();
  int height = maze.getHeight();
  std::vector<bool> visited(width * height, false);
  const int dx[] = {1, -1, 0, 0};
  const int dy[] = {0, 0, 1, -1};

  for (int y = 0; y < height; ++y) {
    for (int x = 0; x < width; ++x) {
      int idx = maze.getIndex(x, y);
      if (visited[idx] || maze.getCell(x, y) != Maze::CELL_WALL)
        continue;

      std::vector<int> wallComponent;
      std::queue<std::pair<int, int>> q;
      q.push({x, y});
      visited[idx] = true;

      while (!q.empty()) {
        auto [cx, cy] = q.front();
        q.pop();
        wallComponent.push_back(maze.getIndex(cx, cy));

        for (int d = 0; d < 4; ++d) {
          int nx = cx + dx[d];
          int ny = cy + dy[d];
          if (nx < 0 || nx >= width || ny < 0 || ny >= height)
            continue;

          int nIdx = maze.getIndex(nx, ny);
          if (maze.getCell(nx, ny) == Maze::CELL_WALL && !visited[nIdx]) {
            visited[nIdx] = true;
            q.push({nx, ny});
          }
        }
      }

      // If the contiguous wall block is very small (e.g. < 50), it is an
      // isolated pillar inside a room
      if (wallComponent.size() < 50) {
        for (int tileIdx : wallComponent) {
          int tx = tileIdx % width;
          int ty = tileIdx / width;
          maze.setCell(tx, ty, Maze::CELL_ROOM);
        }
      }
    }
  }
}

// ============================================================================
// absorbThinPeninsulas — Finds C-shaped cuts (horizontal peninsulas) inside
// rooms and absorbs them into the room ONLY IF their height is < 3 cells.
// This preserves thick structures while perfectly fixing Zelda wall projection.
// ============================================================================
void BSPGenerator::absorbThinPeninsulas(Maze &maze) {
  int width = maze.getWidth();
  int height = maze.getHeight();
  std::vector<bool> visited(width * height, false);

  for (int y = 0; y < height; ++y) {
    for (int x = 0; x < width; ++x) {
      int idx = maze.getIndex(x, y);
      if (visited[idx] || maze.getCell(x, y) != Maze::CELL_ROOM) continue;

      // Find the bounding box of this distinct room component
      std::vector<int> component;
      std::queue<std::pair<int, int>> q;
      q.push({x, y});
      visited[idx] = true;

      int minX = x, maxX = x;
      int minY = y, maxY = y;

      while (!q.empty()) {
        auto [cx, cy] = q.front();
        q.pop();
        
        component.push_back(maze.getIndex(cx, cy));
        if (cx < minX) minX = cx;
        if (cx > maxX) maxX = cx;
        if (cy < minY) minY = cy;
        if (cy > maxY) maxY = cy;

        const int dx[] = {1, -1, 0, 0};
        const int dy[] = {0, 0, 1, -1};
        for (int d = 0; d < 4; ++d) {
          int nx = cx + dx[d];
          int ny = cy + dy[d];
          if (nx < 0 || nx >= width || ny < 0 || ny >= height) continue;
          
          int nIdx = maze.getIndex(nx, ny);
          if (maze.getCell(nx, ny) == Maze::CELL_ROOM && !visited[nIdx]) {
            visited[nIdx] = true;
            q.push({nx, ny});
          }
        }
      }

      int compW = maxX - minX + 1;
      int compH = maxY - minY + 1;
      std::vector<bool> inComp(compW * compH, false);
      for (int cIdx : component) {
         int cx = cIdx % width;
         int cy = cIdx / width;
         inComp[(cy - minY) * compW + (cx - minX)] = true;
      }

      // Evaluate the Vertical Convexity of the room
      for (int cx = 0; cx < compW; ++cx) {
        bool seenRoom = false;
        int runStart = -1;

        for (int cy = 0; cy < compH; ++cy) {
          if (inComp[cy * compW + cx]) {
            // Re-entered room! Check if the gap we just jumped was small enough to absorb
            if (seenRoom && runStart != -1) {
              int runLength = cy - runStart;
              // Only absorb if the gap is less than 3 cells high
              if (runLength < 3) {
                for (int wy = runStart; wy < cy; ++wy) {
                  maze.setCell(minX + cx, minY + wy, Maze::CELL_ROOM);
                }
              }
            }
            seenRoom = true;
            runStart = -1; // Reset gap tracking
          } else {
            // Wall gap started
            if (seenRoom && runStart == -1) {
              runStart = cy;
            }
          }
        }
      }
    }
  }
}
