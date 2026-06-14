#include "player.hpp"
#include <cmath>
#include <algorithm>

// ============================================================================
// Constructor
// ============================================================================
Player::Player(Vector2 startPosition) 
    : m_position(startPosition), m_speed(250.0f), m_radius(10.0f) 
{}

// ============================================================================
// Update - Kinematics and Input
// ============================================================================
void Player::update(const Maze& maze) {
  // 1. FRAMERATE INDEPENDENCE (Delta Time)
  // GetFrameTime() returns the seconds elapsed since the last frame (e.g., 0.016s for 60 FPS).
  // Multiplying our speed by this ensures we move exactly 250 pixels per second in real life,
  // regardless of how fast the computer is running.
  float dt = GetFrameTime();

  // Create a velocity vector to store how much we WANT to move this frame.
  Vector2 velocity = { 0.0f, 0.0f };

  // WASD and Arrow Key Input
  if (IsKeyDown(KEY_W) || IsKeyDown(KEY_UP))    velocity.y -= m_speed;
  if (IsKeyDown(KEY_S) || IsKeyDown(KEY_DOWN))  velocity.y += m_speed;
  if (IsKeyDown(KEY_A) || IsKeyDown(KEY_LEFT))  velocity.x -= m_speed;
  if (IsKeyDown(KEY_D) || IsKeyDown(KEY_RIGHT)) velocity.x += m_speed;

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
void Player::resolveCollision(const Maze& maze) {
  // To avoid checking every wall in the giant maze (O(V)), we only check the 
  // grid cells immediately surrounding the player's current bounding box.
  
  int cellSize = maze.getCellSize();

  // Calculate the player's bounding box in pixel coordinates
  float minX = m_position.x - m_radius;
  float maxX = m_position.x + m_radius;
  float minY = m_position.y - m_radius;
  float maxY = m_position.y + m_radius;

  // Convert pixel coordinates to Maze Grid Coordinates
  // We use floor() to safely handle negative values if the player goes out of bounds.
  int startGridX = static_cast<int>(std::floor(minX / cellSize));
  int endGridX   = static_cast<int>(std::floor(maxX / cellSize));
  int startGridY = static_cast<int>(std::floor(minY / cellSize));
  int endGridY   = static_cast<int>(std::floor(maxY / cellSize));

  // Loop through these nearby grid cells
  for (int y = startGridY; y <= endGridY; ++y) {
    for (int x = startGridX; x <= endGridX; ++x) {
      
      // If the cell is not a floor/room, it's a solid wall.
      if (maze.getCell(x, y) == Maze::CELL_WALL) {
        
        // Define the AABB (Axis-Aligned Bounding Box) for this specific wall cell
        Rectangle wallRect = {
          static_cast<float>(x * cellSize), 
          static_cast<float>(y * cellSize), 
          static_cast<float>(cellSize), 
          static_cast<float>(cellSize)
        };

        // 3. CIRCLE-TO-AABB MATH (Radial Push)
        // Find the closest point on the wall's rectangle to the player's center
        float closestX = std::clamp(m_position.x, wallRect.x, wallRect.x + wallRect.width);
        float closestY = std::clamp(m_position.y, wallRect.y, wallRect.y + wallRect.height);

        // Calculate the distance vector from the closest point to the player's center
        float dx = m_position.x - closestX;
        float dy = m_position.y - closestY;
        
        // Use squared distance to avoid expensive sqrt() unless necessary
        float distanceSq = dx * dx + dy * dy;

        // If the squared distance is less than the squared radius, we have a collision!
        if (distanceSq > 0.0f && distanceSq < m_radius * m_radius) {
          float distance = std::sqrt(distanceSq);
          float overlap = m_radius - distance;
          
          // Push the player out exactly along the penetration vector
          m_position.x += (dx / distance) * overlap;
          m_position.y += (dy / distance) * overlap;
        }
        else if (distanceSq == 0.0f) {
           // Extremely rare edge case: player center is perfectly inside the wall.
           // Just push them up out of it to avoid a divide-by-zero.
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
