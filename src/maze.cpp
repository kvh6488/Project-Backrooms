#include "maze.hpp"
#include <algorithm>
#include <queue>
#include <deque>

// ============================================================================
// Constructor
// ============================================================================
Maze::Maze(int width, int height, int cellSize, unsigned int seed)
    : m_width(width), m_height(height), m_cellSize(cellSize), m_nonWallCount(0),
      m_corridorCount(0), m_grid(width * height, CELL_WALL),
      m_visible(width * height, false), m_lightLevel(width * height, 0.0f),
      m_radiationMap(width * height, 0) {}

Maze::~Maze() {}

// ============================================================================
// getIndex — The heart of 1D-to-2D Mapping
// ============================================================================
int Maze::getIndex(int x, int y) const {
  // --- TOROIDAL MATH (MODULO WRAPPING) ---
  // In C++, the '%' operator is a remainder, not a true modulo for negative
  // numbers. For example, -1 % 250 = -1. We want -1 to wrap to 249. Formula:
  // (value % MAX + MAX) % MAX guarantees a positive wrapped index.
  int wrappedX = (x % m_width + m_width) % m_width;
  int wrappedY = (y % m_height + m_height) % m_height;

  return wrappedY * m_width + wrappedX;
}

// ============================================================================
// getCell — Read a Cell
// ============================================================================
int Maze::getCell(int x, int y) const { return m_grid[getIndex(x, y)]; }

// ============================================================================
// setCell — Write a Cell
// ============================================================================
void Maze::setCell(int x, int y, int cellType) {
  int index = getIndex(x, y);
  int oldType = m_grid[index];

  if (oldType == CELL_WALL && cellType != CELL_WALL) {
    m_nonWallCount++;
  } else if (oldType != CELL_WALL && cellType == CELL_WALL) {
    m_nonWallCount--;
  }

  if (oldType == CELL_CORRIDOR && cellType != CELL_CORRIDOR) {
    m_corridorCount--;
  } else if (oldType != CELL_CORRIDOR && cellType == CELL_CORRIDOR) {
    m_corridorCount++;
  }

  m_grid[index] = cellType;
}

// ============================================================================
// RECURSIVE SHADOWCASTING (Replaces old BFS Flood Fill)
// ============================================================================
// Algorithm Reference: Björn Bergström's recursive shadowcasting,
void Maze::updateVisibility(int playerX, int playerY, AreaState state) {
  // Clear visibility and light levels every frame
  std::fill(m_visible.begin(), m_visible.end(), false);
  std::fill(m_lightLevel.begin(), m_lightLevel.end(), 0.0f);

  if (state == AreaState::ROOM) {
    // --- ROOMS ARE ALWAYS FULLY LIT ---
    // Simple BFS flood fill through CELL_ROOM tiles only.
    std::queue<std::pair<int, int>> q;
    q.push({playerX, playerY});
    int startIdx = getIndex(playerX, playerY);
    m_visible[startIdx] = true;
    m_lightLevel[startIdx] = 1.0f;

    int dx[] = {0, 0, -1, 1, -1, 1, -1, 1};
    int dy[] = {-1, 1, 0, 0, -1, -1, 1, 1};

    while (!q.empty()) {
      auto [x, y] = q.front();
      q.pop();

      for (int i = 0; i < 8; ++i) {
        int nx = x + dx[i];
        int ny = y + dy[i];
        int nIndex = getIndex(nx, ny);

        if (!m_visible[nIndex]) {
          m_visible[nIndex] = true;
          m_lightLevel[nIndex] = 1.0f;

          // Only flood through room tiles (walls become visible but stop the
          // flood)
          if (getCell(nx, ny) == CELL_ROOM) {
            q.push({nx, ny});
          }
        }
      }
    }
  }
  // For corridors, we don't need to compute visibility, because the renderer
  // now completely ignores the FOV check and draws all corridor tiles,
  // relying entirely on the screen-space darkness overlay to hide unseen areas.
}

// ============================================================================
// castOctant — The Recursive Octant Scanner
// ============================================================================
// This function scans a single octant row-by-row (or column-by-column,
// depending on the octant transform). It tracks which angular range
// is still visible via [startSlope, endSlope].
//
// The recursion happens when we detect a transition from wall → floor:
//   we've found the edge of a shadow, so we recurse to continue scanning
//   the newly revealed arc.
//

bool Maze::isVisible(int x, int y) const { return m_visible[getIndex(x, y)]; }

float Maze::getLightLevel(int x, int y) const {
  return m_lightLevel[getIndex(x, y)];
}

// ============================================================================
// PHASE 2.3: RUBIK'S TORUS
// ============================================================================

void Maze::clearShiftingZones() { m_shiftingZones.clear(); }

void Maze::addShiftingZone(int x, int y, int w, int h) {
  m_shiftingZones.push_back({x, y, w, h});
}

bool Maze::isShiftingZone(int x, int y) const {
  int wrappedX = (x % m_width + m_width) % m_width;
  int wrappedY = (y % m_height + m_height) % m_height;
  for (const auto &zone : m_shiftingZones) {
    if (wrappedX >= zone.x && wrappedX < zone.x + zone.width &&
        wrappedY >= zone.y && wrappedY < zone.y + zone.height) {
      return true;
    }
  }
  return false;
}

void Maze::eraseZone(int startX, int startY, int width, int height) {
  // Overwrite cells with solid rock
  for (int y = startY; y < startY + height; ++y) {
    for (int x = startX; x < startX + width; ++x) {
      setCell(x, y, CELL_WALL);
    }
  }

  // Remove any rooms that intersect this 2D zone
  auto it = std::remove_if(
      m_rooms.begin(), m_rooms.end(),
      [startX, startY, width, height](const Room &r) {
        bool outsideX = (r.x + r.width <= startX) || (r.x >= startX + width);
        bool outsideY = (r.y + r.height <= startY) || (r.y >= startY + height);
        return !(outsideX ||
                 outsideY); // It intersects if it is NOT completely outside
      });
  m_rooms.erase(it, m_rooms.end());
}

bool Maze::hasDiagonalLeak(int x, int y) const {
  const int ddx[] = {-1, 1, -1, 1};
  const int ddy[] = {-1, -1, 1, 1};
  for (int i = 0; i < 4; ++i) {
    int nx = x + ddx[i];
    int ny = y + ddy[i];
    int tDiag = getCell(nx, ny);
    if (tDiag == Maze::CELL_CORRIDOR || tDiag == Maze::CELL_ROOM) {
      int t1 = getCell(nx, y);
      int t2 = getCell(x, ny);
      if (t1 == Maze::CELL_WALL && t2 == Maze::CELL_WALL) {
        return true;
      }
    }
  }
  return false;
}

// ============================================================================
// isValidDoorPlacement - 1-to-1 Door Isolation
// ============================================================================
bool Maze::isValidDoorPlacement(int x, int y) const {
  int roomNeighbors = 0;
  const int dx[] = {1, -1, 0, 0};
  const int dy[] = {0, 0, 1, -1};

  for (int d = 0; d < 4; ++d) {
    int nx = x + dx[d];
    int ny = y + dy[d];
    if (nx >= 0 && nx < m_width && ny >= 0 && ny < m_height) {
      if (getCell(nx, ny) == CELL_ROOM) {
        roomNeighbors++;
      }
    }
  }
  // Rule 1: A corridor cannot border > 1 room cell (prevents inner corner
  // doors)
  if (roomNeighbors > 1)
    return false;

  // Rule 2: A room cell cannot border > 1 corridor cell (prevents adjacent
  // doors)
  for (int d = 0; d < 4; ++d) {
    int nx = x + dx[d];
    int ny = y + dy[d];
    if (nx >= 0 && nx < m_width && ny >= 0 && ny < m_height) {
      if (getCell(nx, ny) == CELL_ROOM) {
        int currentCorridorNeighbors = 0;
        for (int dd = 0; dd < 4; ++dd) {
          int nnx = nx + dx[dd];
          int nny = ny + dy[dd];
          if (nnx >= 0 && nnx < m_width && nny >= 0 && nny < m_height) {
            if (getCell(nnx, nny) == CELL_CORRIDOR) {
              currentCorridorNeighbors++;
            }
          }
        }
        if (currentCorridorNeighbors >= 1)
          return false;
      }
    }
  }
  return true;
}

// ============================================================================
// PHASE 3: RADIATION MECHANICS
// ============================================================================

void Maze::spawnRadiationBarrels(int count) {
  if (m_rooms.empty()) return;
  
  // Basic random spawning
  for (int i = 0; i < count; ++i) {
    int roomIdx = std::rand() % m_rooms.size();
    const Room& r = m_rooms[roomIdx];
    
    // Pick a random cell in the room (avoiding edges, just to be safe)
    int rx = r.x + 1 + (std::rand() % (std::max(1, r.width - 2)));
    int ry = r.y + 1 + (std::rand() % (std::max(1, r.height - 2)));
    
    // Make sure it's a room cell and no barrel exists
    if (getCell(rx, ry) == CELL_ROOM && !hasBarrel(rx, ry)) {
      m_barrels.push_back({m_nextBarrelId++, rx, ry});
    }
  }
  calculateRadiationZones();
}

void Maze::calculateRadiationZones() {
  std::fill(m_radiationMap.begin(), m_radiationMap.end(), 0);
  
  for (const auto& barrel : m_barrels) {
    // 0-1 BFS for each barrel up to depth 15.
    // Moving ROOM -> ROOM costs 0, effectively infecting the whole room instantly.
    std::deque<std::pair<int, int>> q;
    std::vector<int> distances(m_width * m_height, -1);
    
    int startIdx = getIndex(barrel.x, barrel.y);
    q.push_back({barrel.x, barrel.y});
    distances[startIdx] = 0;
    
    const int dx[] = {1, -1, 0, 0};
    const int dy[] = {0, 0, 1, -1};
    
    while (!q.empty()) {
      auto [x, y] = q.front();
      q.pop_front();
      
      int currentIdx = getIndex(x, y);
      int d = distances[currentIdx];
      m_radiationMap[currentIdx] = 1; // Mark as radiated
      
      if (d >= 10) continue; // Max radius (reduced to 10)
      
      int currentType = m_grid[currentIdx];
      
      for (int i = 0; i < 4; ++i) {
        int nx = (x + dx[i]) % m_width;
        if (nx < 0) nx += m_width;
        int ny = (y + dy[i]) % m_height;
        if (ny < 0) ny += m_height;
        
        int nIdx = getIndex(nx, ny);
        int nextType = m_grid[nIdx];
        
        // Fluid flow: Only pass through open spaces (ROOM or CORRIDOR). Walls block.
        if (nextType == CELL_ROOM || nextType == CELL_CORRIDOR) {
          // If we are in a room and moving to another room tile, cost is 0.
          // This causes the entire room to instantly flood at the same depth.
          int cost = (currentType == CELL_ROOM && nextType == CELL_ROOM) ? 0 : 1;
          
          if (distances[nIdx] == -1 || distances[nIdx] > d + cost) {
            distances[nIdx] = d + cost;
            if (cost == 0) {
              q.push_front({nx, ny});
            } else {
              q.push_back({nx, ny});
            }
          }
        }
      }
    }
  }

  // Post-pass: Update the isRadiated flag on all Room structs
  // This provides an easy check for specialised spawns/items.
  for (auto& room : m_rooms) {
    room.isRadiated = false;
    for (int ry = room.y; ry < room.y + room.height; ++ry) {
      for (int rx = room.x; rx < room.x + room.width; ++rx) {
        // Toroidal coordinates
        int mx = rx % m_width;
        if (mx < 0) mx += m_width;
        int my = ry % m_height;
        if (my < 0) my += m_height;
        
        int idx = getIndex(mx, my);
        if (m_grid[idx] == CELL_ROOM && m_radiationMap[idx] > 0) {
          room.isRadiated = true;
          break; // One radiated tile means the whole room is radiated
        }
      }
      if (room.isRadiated) break;
    }
  }
}

void Maze::destroyBarrelNear(int x, int y, int radius) {
  auto it = std::remove_if(m_barrels.begin(), m_barrels.end(),
    [x, y, radius, this](const RadiationBarrel& b) {
      int dx = std::abs(b.x - x);
      int dy = std::abs(b.y - y);
      // Handle toroidal wrapping for distance checking
      dx = std::min(dx, m_width - dx);
      dy = std::min(dy, m_height - dy);
      return dx <= radius && dy <= radius;
    });
    
  if (it != m_barrels.end()) {
    m_barrels.erase(it, m_barrels.end());
    // Recalculate instantly vanishing radiation
    calculateRadiationZones();
  }
}

int Maze::getRadiationLevel(int x, int y) const {
  return m_radiationMap[getIndex(x, y)];
}

bool Maze::hasBarrel(int x, int y) const {
  for (const auto& b : m_barrels) {
    if (b.x == x && b.y == y) return true;
  }
  return false;
}

bool Maze::isBarrelNear(int x, int y, int radius) const {
  for (const auto& b : m_barrels) {
    int dx = std::abs(b.x - x);
    int dy = std::abs(b.y - y);
    dx = std::min(dx, m_width - dx);
    dy = std::min(dy, m_height - dy);
    if (dx <= radius && dy <= radius) {
      return true;
    }
  }
  return false;
}

// ============================================================================
// Zone-Aware Barrel Management
// ============================================================================
// These methods handle the interaction between the maze regeneration system
// and radiation barrels. When a shifting zone regenerates, any barrels inside
// it would become orphaned (pointing at wall tiles). We remove them first,
// then spawn replacements in the freshly carved rooms.
//
// This is a classic example of maintaining INVARIANTS across subsystem
// boundaries — the regeneration system (Phase 2) doesn't know about radiation
// (Phase 3), so we bridge them here. In Gang of Four terms, this is the
// Mediator pattern: the Maze class mediates between the two subsystems.
// ============================================================================

int Maze::removeBarrelsInZone(int zx, int zy, int zw, int zh) {
  // Erase-Remove idiom: O(n) scan, O(n) erase — same pattern used in eraseZone
  // for room pruning. We count how many were removed so the caller knows how
  // many replacements to spawn.
  int removedCount = 0;
  auto it = std::remove_if(m_barrels.begin(), m_barrels.end(),
    [zx, zy, zw, zh, &removedCount](const RadiationBarrel& b) {
      bool inside = (b.x >= zx && b.x < zx + zw &&
                     b.y >= zy && b.y < zy + zh);
      if (inside) removedCount++;
      return inside;
    });
  m_barrels.erase(it, m_barrels.end());
  return removedCount;
}

void Maze::spawnBarrelInZone(int zx, int zy, int zw, int zh) {
  // Collect rooms that overlap with the regenerated zone
  std::vector<const Room*> candidates;
  for (const auto& r : m_rooms) {
    // Check if the room's bounding box intersects the zone
    bool outsideX = (r.x + r.width <= zx) || (r.x >= zx + zw);
    bool outsideY = (r.y + r.height <= zy) || (r.y >= zy + zh);
    if (!(outsideX || outsideY)) {
      candidates.push_back(&r);
    }
  }
  
  if (candidates.empty()) return;
  
  // Try several times to find a valid floor tile (room cell, no existing barrel)
  for (int attempt = 0; attempt < 20; ++attempt) {
    const Room& r = *candidates[std::rand() % candidates.size()];
    int rx = r.x + 1 + (std::rand() % (std::max(1, r.width - 2)));
    int ry = r.y + 1 + (std::rand() % (std::max(1, r.height - 2)));
    
    if (getCell(rx, ry) == CELL_ROOM && !hasBarrel(rx, ry)) {
      m_barrels.push_back({m_nextBarrelId++, rx, ry});
      return;
    }
  }
}

