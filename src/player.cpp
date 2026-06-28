#include "player.hpp"
#include <algorithm>
#include <cmath>

// ============================================================================
// Constructor
// ============================================================================
Player::Player(Vector2 startPosition, AreaState startState)
    : m_position(startPosition), m_speed(130.0f), m_radius(10.0f),
      m_areaState(startState) {}

// ============================================================================
// Update - Kinematics and Input
// ============================================================================
void Player::update(const Maze &maze) {
  // 1. FRAMERATE INDEPENDENCE (Delta Time)
  // GetFrameTime() returns the seconds elapsed since the last frame (e.g.,
  // 0.016s for 60 FPS). Multiplying our speed by this ensures we move exactly
  // 250 pixels per second in real life, regardless of how fast the computer is
  // running.
  float dt = GetFrameTime();

  // Create a velocity vector to store how much we WANT to move this frame.
  Vector2 velocity = {0.0f, 0.0f};

  // WASD and Arrow Key Input
  if (IsKeyDown(KEY_W) || IsKeyDown(KEY_UP))
    velocity.y -= m_speed;
  if (IsKeyDown(KEY_S) || IsKeyDown(KEY_DOWN))
    velocity.y += m_speed;
  if (IsKeyDown(KEY_A) || IsKeyDown(KEY_LEFT))
    velocity.x -= m_speed;
  if (IsKeyDown(KEY_D) || IsKeyDown(KEY_RIGHT))
    velocity.x += m_speed;

  // 1.5 DOOR TRANSITIONS (KEY_K)
  if (IsKeyPressed(KEY_K)) {
    int gridX = static_cast<int>(std::floor(m_position.x / maze.getCellSize()));
    int gridY = static_cast<int>(std::floor(m_position.y / maze.getCellSize()));

    int dx[] = {0, 0, -1, 1};
    int dy[] = {-1, 1, 0, 0};

    for (int i = 0; i < 4; ++i) {
      int nx = gridX + dx[i];
      int ny = gridY + dy[i];
      int cell = maze.getCell(nx, ny);

      if (m_areaState == AreaState::CORRIDOR && cell == Maze::CELL_ROOM) {
        // We are trying to enter a room.
        // Since rendering is completely decoupled from the room array, we no
        // longer need to verify which exact room rectangle this tile belongs to
        // (which fixes bugs with bridged rooms).
        m_areaState = AreaState::ROOM;
        // Snap player over the threshold into the room
        m_position.x = nx * maze.getCellSize() + maze.getCellSize() / 2.0f;
        m_position.y = ny * maze.getCellSize() + maze.getCellSize() / 2.0f;
        break; // Only enter one door per frame
      } else if (m_areaState == AreaState::ROOM && cell == Maze::CELL_CORRIDOR) {
        // We are trying to exit a room into a corridor.
        m_areaState = AreaState::CORRIDOR;
        // Snap player over the threshold into the corridor
        m_position.x = nx * maze.getCellSize() + maze.getCellSize() / 2.0f;
        m_position.y = ny * maze.getCellSize() + maze.getCellSize() / 2.0f;
        break;
      }
    }
  }

  // 2. THE "SLIDE" COLLISION METHOD
  // Instead of updating X and Y simultaneously and getting stuck on corners,
  // we step one axis at a time.

  // Step 1: Move on the X axis, then check if we hit a wall and resolve it.
  m_position.x += velocity.x * dt;
  resolveCollision(maze);

  // Step 2: Move on the Y axis, then check if we hit a wall and resolve it.
  m_position.y += velocity.y * dt;
  resolveCollision(maze);
}

// ============================================================================
// Collision Resolution
// ============================================================================
void Player::resolveCollision(const Maze &maze) {
  // To avoid checking every wall in the giant maze (O(V)), we only check the
  // grid cells immediately surrounding the player's current bounding box.

  int cellSize = maze.getCellSize();

  // Calculate the player's bounding box in pixel coordinates
  float minX = m_position.x - m_radius;
  float maxX = m_position.x + m_radius;
  float minY = m_position.y - m_radius;
  float maxY = m_position.y + m_radius;

  // Convert pixel coordinates to Maze Grid Coordinates
  // We use floor() to safely handle negative values if the player goes out of
  // bounds.
  int startGridX = static_cast<int>(std::floor(minX / cellSize));
  int endGridX = static_cast<int>(std::floor(maxX / cellSize));
  int startGridY = static_cast<int>(std::floor(minY / cellSize));
  int endGridY = static_cast<int>(std::floor(maxY / cellSize));

  // Loop through these nearby grid cells
  for (int y = startGridY; y <= endGridY; ++y) {
    for (int x = startGridX; x <= endGridX; ++x) {

      // Check contextual solidity
      int cell = maze.getCell(x, y);
      bool isSolid = false;

      if (cell == Maze::CELL_WALL) {
        isSolid = true;
      } else if (m_areaState == AreaState::CORRIDOR &&
                 cell == Maze::CELL_ROOM) {
        isSolid = true; // Rooms are solid walls from the outside
      } else if (m_areaState == AreaState::ROOM && cell == Maze::CELL_CORRIDOR) {
        isSolid = true; // Corridors are solid walls from the inside
      }

      if (isSolid) {

        // Define the AABB (Axis-Aligned Bounding Box) for this specific wall
        // cell
        Rectangle wallRect = {
            static_cast<float>(x * cellSize), static_cast<float>(y * cellSize),
            static_cast<float>(cellSize), static_cast<float>(cellSize)};

        // 3. CIRCLE-TO-AABB MATH (Radial Push)
        // Find the closest point on the wall's rectangle to the player's center
        float closestX =
            std::clamp(m_position.x, wallRect.x, wallRect.x + wallRect.width);
        float closestY =
            std::clamp(m_position.y, wallRect.y, wallRect.y + wallRect.height);

        // Calculate the distance vector from the closest point to the player's
        // center
        float dx = m_position.x - closestX;
        float dy = m_position.y - closestY;

        // Use squared distance to avoid expensive sqrt() unless necessary
        float distanceSq = dx * dx + dy * dy;

        // If the squared distance is less than the squared radius, we have a
        // collision!
        if (distanceSq > 0.0f && distanceSq < m_radius * m_radius) {
          float distance = std::sqrt(distanceSq);
          float overlap = m_radius - distance;

          // Push the player out exactly along the penetration vector
          m_position.x += (dx / distance) * overlap;
          m_position.y += (dy / distance) * overlap;
        } else if (distanceSq == 0.0f) {
          // Extremely rare edge case: player center is perfectly inside the
          // wall. Just push them up out of it to avoid a divide-by-zero.
          m_position.y -= m_radius;
        }
      }
    }
  }
}

// ============================================================================
// Render
// ============================================================================
void Player::render() const {
  // Draw the player as a simple red circle
  DrawCircleV(m_position, m_radius, RED);
}

// ============================================================================
// Door Interaction Check
// ============================================================================
bool Player::canUseDoor(const Maze &maze) const {
  int gridX = static_cast<int>(std::floor(m_position.x / maze.getCellSize()));
  int gridY = static_cast<int>(std::floor(m_position.y / maze.getCellSize()));

  int dx[] = {0, 0, -1, 1};
  int dy[] = {-1, 1, 0, 0};

  for (int i = 0; i < 4; ++i) {
    int nx = gridX + dx[i];
    int ny = gridY + dy[i];
    int cell = maze.getCell(nx, ny);

    if (m_areaState == AreaState::CORRIDOR && cell == Maze::CELL_ROOM)
      return true;
    if (m_areaState == AreaState::ROOM && cell == Maze::CELL_CORRIDOR)
      return true;
  }
  return false;
}
