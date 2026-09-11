#pragma once

#include <raylib.h>

// ============================================================================
// grid - the one pixel density
// ============================================================================
// Every sheet in assets/ is authored at SOURCE_TILE art pixels per world
// cell and drawn at WORLD_SCALE, so one art pixel is a WORLD_SCALE-square
// block on the canvas and a cell is CELL canvas pixels. Nothing in the draw
// path may scale by any other factor: a sprite that is N source tiles wide
// covers exactly N cells, and a hand-measured rectangle that is not a whole
// number of tiles is a bug, not a special case.
//
// srcTile() addresses a sheet in tiles and cellRect() the world in cells.
// destFor() is the invariant made mechanical: a destination rectangle is only
// ever DERIVED from its source rectangle, so no draw site can carry a scale
// of its own.
// ============================================================================
namespace grid {

inline constexpr int SOURCE_TILE = 16;                   // art px per cell
inline constexpr int WORLD_SCALE = 2;                    // canvas px per art px
inline constexpr int CELL = SOURCE_TILE * WORLD_SCALE;   // canvas px per cell

// Source rectangle for a w x h tile block at tile (col, row) of a sheet.
inline constexpr Rectangle srcTile(int col, int row, int w = 1, int h = 1) {
  return {(float)(col * SOURCE_TILE), (float)(row * SOURCE_TILE),
          (float)(w * SOURCE_TILE), (float)(h * SOURCE_TILE)};
}

// World rectangle for a w x h cell block whose top-left cell is (x, y).
inline constexpr Rectangle cellRect(int x, int y, int w = 1, int h = 1) {
  return {(float)(x * CELL), (float)(y * CELL), (float)(w * CELL),
          (float)(h * CELL)};
}

// Destination for `src` drawn at WORLD_SCALE with its top-left at a world
// point. Width and height come from the source, never from the caller.
inline constexpr Rectangle destFor(Rectangle src, float worldX, float worldY) {
  return {worldX, worldY, src.width * WORLD_SCALE, src.height * WORLD_SCALE};
}

// Destination for `src` standing on cell (x, y): furniture sits on its floor
// tile and grows upward, centred across `cellsWide` cells ending at x.
inline constexpr Rectangle standingOn(Rectangle src, int x, int y,
                                      int cellsWide = 1) {
  Rectangle d = destFor(src, 0, 0);
  d.x = (x + 1) * CELL - (cellsWide * CELL + d.width) / 2.0f;
  d.y = (y + 1) * CELL - d.height;
  return d;
}

} // namespace grid
