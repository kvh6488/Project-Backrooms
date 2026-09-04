// ============================================================================
// test_magic_book.cpp — Magic Book of Maps placement invariants
// ============================================================================
// The book is the hardest thing in the game to observe by playing: it needs a
// mushroom, a full trip, a 1-in-3 roll, a table in range, and it despawns the
// moment the trip decays. That made "nothing on screen" ambiguous between a
// spawn failure and a render failure.
//
// These tests remove the spawner from that ambiguity permanently. Once they
// are green, a missing book on screen is a RENDERING bug — which is exactly
// the kind of bisection a test suite is for.
//
// Nothing here opens a window: ItemSpawner and Maze are pure data.
// ============================================================================

#include "items/crafting_system.hpp"
#include "items/item_database.hpp"
#include "world/generators/bsp_generator.hpp"
#include "world/generators/loop_generator.hpp"
#include "world/generators/prims_generator.hpp"
#include "world/generators/tunnel_borer.hpp"
#include "world/item_spawner.hpp"
#include "world/maze.hpp"

#include <algorithm>
#include <cmath>
#include <gtest/gtest.h>
#include <random>
#include <utility>
#include <vector>

namespace {

// The radius baked into ItemSpawner::spawnMagicBookOfMaps.
constexpr float SEARCH_RADIUS = 20.0f;

class MagicBookSpawnTest : public ::testing::Test {
protected:
  void SetUp() override {
    // Normally done by the Application constructor. A test binary builds no
    // Application, so it must initialise the static registries itself.
    ItemDatabase::init();
    CraftingSystem::init();
  }

  // Lays down a horizontal table pair. Mirrors ItemSpawner::spawnTables:
  // state 0 is the left half, state 1 the RIGHT half and the root.
  static void placeHorizontalTable(Maze &maze, int rootX, int rootY) {
    maze.setCell(rootX - 1, rootY, Maze::CELL_ROOM);
    maze.setCell(rootX, rootY, Maze::CELL_ROOM);
    maze.setItem(rootX - 1, rootY, ItemType::TABLE);
    maze.setItemState(rootX - 1, rootY, 0);
    maze.setItem(rootX, rootY, ItemType::TABLE);
    maze.setItemState(rootX, rootY, 1);
  }

  // Vertical pair: state 2 is the top half, state 3 the BOTTOM half and root.
  static void placeVerticalTable(Maze &maze, int rootX, int rootY) {
    maze.setCell(rootX, rootY - 1, Maze::CELL_ROOM);
    maze.setCell(rootX, rootY, Maze::CELL_ROOM);
    maze.setItem(rootX, rootY - 1, ItemType::TABLE);
    maze.setItemState(rootX, rootY - 1, 2);
    maze.setItem(rootX, rootY, ItemType::TABLE);
    maze.setItemState(rootX, rootY, 3);
  }

  // The invariant every successful spawn must satisfy: the book sits on a
  // table tile, and specifically on a ROOT tile. Landing on a non-root half
  // would draw the book against the wrong rectangle.
  static void expectBookOnTableRoot(const Maze &maze) {
    ASSERT_TRUE(maze.isMagicBookSpawned());
    int bx = maze.getMagicBookX();
    int by = maze.getMagicBookY();
    EXPECT_EQ(maze.getItem(bx, by), ItemType::TABLE);
    int state = maze.getItemState(bx, by);
    EXPECT_TRUE(state == 1 || state == 3)
        << "book landed on table state " << state
        << " (expected a root tile: 1 = horizontal right, 3 = vertical bottom)";
  }
};

// --- 1. The happy path --------------------------------------------------

TEST_F(MagicBookSpawnTest, SpawnsOnHorizontalTableRoot) {
  Maze maze(60, 60, 32, 12345);
  std::mt19937 rng(12345);
  ItemSpawner spawner(rng);

  placeHorizontalTable(maze, 30, 30);

  auto result = spawner.spawnMagicBookOfMaps(maze, 28, 30);

  EXPECT_EQ(result, ItemSpawner::BookSpawnResult::SPAWNED);
  expectBookOnTableRoot(maze);
  EXPECT_EQ(maze.getMagicBookX(), 30);
  EXPECT_EQ(maze.getMagicBookY(), 30);
}

TEST_F(MagicBookSpawnTest, SpawnsOnVerticalTableRoot) {
  Maze maze(60, 60, 32, 999);
  std::mt19937 rng(999);
  ItemSpawner spawner(rng);

  placeVerticalTable(maze, 20, 25);

  auto result = spawner.spawnMagicBookOfMaps(maze, 20, 27);

  EXPECT_EQ(result, ItemSpawner::BookSpawnResult::SPAWNED);
  expectBookOnTableRoot(maze);
  EXPECT_EQ(maze.getMagicBookX(), 20);
  EXPECT_EQ(maze.getMagicBookY(), 25);
}

// --- 2. Failure is reported, not silent ---------------------------------

TEST_F(MagicBookSpawnTest, ReportsFailureWhenNoTableExists) {
  Maze maze(60, 60, 32, 7);
  std::mt19937 rng(7);
  ItemSpawner spawner(rng);

  auto result = spawner.spawnMagicBookOfMaps(maze, 30, 30);

  EXPECT_EQ(result, ItemSpawner::BookSpawnResult::NO_TABLE_IN_RANGE);
  EXPECT_FALSE(maze.isMagicBookSpawned());
}

// A failed attempt must not leave the maze half-updated.
TEST_F(MagicBookSpawnTest, FailureLeavesBookCoordinatesCleared) {
  Maze maze(60, 60, 32, 7);
  std::mt19937 rng(7);
  ItemSpawner spawner(rng);

  spawner.spawnMagicBookOfMaps(maze, 30, 30);

  EXPECT_FALSE(maze.isMagicBookSpawned());
  EXPECT_EQ(maze.getMagicBookX(), -1);
  EXPECT_EQ(maze.getMagicBookY(), -1);
}

// --- 3. The search radius actually bounds the search --------------------

TEST_F(MagicBookSpawnTest, RespectsSearchRadius) {
  Maze maze(120, 120, 32, 4242);
  std::mt19937 rng(4242);
  ItemSpawner spawner(rng);

  // Comfortably outside the 20 tile radius.
  placeHorizontalTable(maze, 90, 30);

  auto result = spawner.spawnMagicBookOfMaps(maze, 30, 30);

  EXPECT_EQ(result, ItemSpawner::BookSpawnResult::NO_TABLE_IN_RANGE);
  EXPECT_FALSE(maze.isMagicBookSpawned());
}

// The radius is EUCLIDEAN, not Chebyshev. A table at (+15, +15) is 21.2 tiles
// away and must be rejected even though both axes are individually under 20.
TEST_F(MagicBookSpawnTest, RadiusIsEuclideanNotChebyshev) {
  Maze maze(120, 120, 32, 555);
  std::mt19937 rng(555);
  ItemSpawner spawner(rng);

  placeHorizontalTable(maze, 45, 45); // dx = 15, dy = 15 -> dist ~= 21.2

  auto result = spawner.spawnMagicBookOfMaps(maze, 30, 30);

  EXPECT_EQ(result, ItemSpawner::BookSpawnResult::NO_TABLE_IN_RANGE);
}

TEST_F(MagicBookSpawnTest, SpawnedTableIsWithinRadius) {
  Maze maze(120, 120, 32, 31337);
  std::mt19937 rng(31337);
  ItemSpawner spawner(rng);

  // A spread of tables, some in range and some out.
  placeHorizontalTable(maze, 62, 60);
  placeVerticalTable(maze, 70, 55);
  placeHorizontalTable(maze, 100, 60); // out of range
  placeVerticalTable(maze, 60, 100);   // out of range

  ASSERT_EQ(spawner.spawnMagicBookOfMaps(maze, 60, 60),
            ItemSpawner::BookSpawnResult::SPAWNED);

  float dx = (float)(maze.getMagicBookX() - 60);
  float dy = (float)(maze.getMagicBookY() - 60);
  EXPECT_LE(std::sqrt(dx * dx + dy * dy), SEARCH_RADIUS);
}

// --- 4. Only root tiles are candidates ----------------------------------

// Non-root halves (states 0 and 2) must never be chosen: the renderer derives
// the table rectangle from the root tile, so a book on a non-root half would
// be positioned against a rectangle that does not exist.
TEST_F(MagicBookSpawnTest, IgnoresNonRootTableTiles) {
  Maze maze(60, 60, 32, 24680);
  std::mt19937 rng(24680);
  ItemSpawner spawner(rng);

  // Orphaned non-root halves only - no state 1 or 3 anywhere.
  maze.setCell(30, 30, Maze::CELL_ROOM);
  maze.setItem(30, 30, ItemType::TABLE);
  maze.setItemState(30, 30, 0);
  maze.setCell(32, 30, Maze::CELL_ROOM);
  maze.setItem(32, 30, ItemType::TABLE);
  maze.setItemState(32, 30, 2);

  auto result = spawner.spawnMagicBookOfMaps(maze, 30, 30);

  EXPECT_EQ(result, ItemSpawner::BookSpawnResult::NO_TABLE_IN_RANGE);
  EXPECT_FALSE(maze.isMagicBookSpawned());
}

// Across many RNG states, every chosen tile must be one of the real
// candidates. This holds whether placement is random or closest-first, so it
// survives a change to that policy.
TEST_F(MagicBookSpawnTest, AlwaysChoosesAnActualCandidate) {
  const std::vector<std::pair<int, int>> roots = {{32, 30}, {30, 34}, {36, 36}};

  for (unsigned int seed = 0; seed < 100; ++seed) {
    Maze maze(60, 60, 32, seed);
    std::mt19937 rng(seed);
    ItemSpawner spawner(rng);

    placeHorizontalTable(maze, 32, 30);
    placeVerticalTable(maze, 30, 34);
    placeHorizontalTable(maze, 36, 36);

    ASSERT_EQ(spawner.spawnMagicBookOfMaps(maze, 30, 30),
              ItemSpawner::BookSpawnResult::SPAWNED)
        << "seed " << seed;

    std::pair<int, int> got = {maze.getMagicBookX(), maze.getMagicBookY()};
    EXPECT_NE(std::find(roots.begin(), roots.end(), got), roots.end())
        << "seed " << seed << " picked (" << got.first << ", " << got.second
        << "), which is not a table root";
  }
}

// --- 5. Reproducibility --------------------------------------------------

// A seed determines the whole world, so it must determine book placement too.
// Without this, "reproduce with --seed N" would be a lie for this feature.
TEST_F(MagicBookSpawnTest, DeterministicForAFixedSeed) {
  auto run = [](unsigned int seed) {
    Maze maze(60, 60, 32, seed);
    std::mt19937 rng(seed);
    ItemSpawner spawner(rng);

    placeHorizontalTable(maze, 32, 30);
    placeVerticalTable(maze, 30, 34);
    placeHorizontalTable(maze, 36, 36);

    spawner.spawnMagicBookOfMaps(maze, 30, 30);
    return std::make_pair(maze.getMagicBookX(), maze.getMagicBookY());
  };

  EXPECT_EQ(run(8675309), run(8675309));
  EXPECT_EQ(run(1788480606), run(1788480606));
}

// --- 6. Integration with the real generation pipeline -------------------

// The unit tests above build tables by hand. This one runs the genuine
// pipeline so a change to generation or to spawnTables cannot silently leave
// the book with nowhere to go.
TEST_F(MagicBookSpawnTest, FullPipelinePlacesBookOnGeneratedTable) {
  const unsigned int seed = 1788480606; // the "radiation" debug fixture
  Maze maze(250, 150, 32, seed);
  std::mt19937 rng(seed);

  BSPGenerator bsp;
  bsp.generate(maze, rng);
  int midRoomIdx = bsp.getMiddleRoomIndex();

  PrimsGenerator prims;
  prims.generate(maze, rng, midRoomIdx);

  LoopGenerator loops;
  loops.generate(maze, rng);

  TunnelBorer borer;
  borer.ensureConnectivity(maze);

  prims.pruneSmallAlcoves(maze, 5);

  ItemSpawner spawner(rng);
  spawner.spawnInitialItems(maze);

  // Generation must produce at least one table, or the book is unreachable
  // by design rather than by accident.
  int tableX = -1, tableY = -1;
  for (int y = 0; y < maze.getHeight() && tableX < 0; ++y) {
    for (int x = 0; x < maze.getWidth(); ++x) {
      if (maze.getItem(x, y) == ItemType::TABLE) {
        int state = maze.getItemState(x, y);
        if (state == 1 || state == 3) {
          tableX = x;
          tableY = y;
          break;
        }
      }
    }
  }
  ASSERT_GE(tableX, 0) << "generation produced no table root tiles, so the "
                          "magic book could never spawn";

  ASSERT_EQ(spawner.spawnMagicBookOfMaps(maze, tableX, tableY),
            ItemSpawner::BookSpawnResult::SPAWNED);
  expectBookOnTableRoot(maze);
}

} // namespace
