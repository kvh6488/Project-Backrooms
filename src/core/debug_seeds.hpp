#pragma once

// ============================================================================
// Debug Seeds — Named, reproducible worlds
// ============================================================================
// A single seed feeds the shared std::mt19937 through the whole generation
// pipeline, so a seed fully determines the world: layout, rooms, radiation,
// item placement. That makes a seed a complete, one-word bug report — and it
// makes "the world where X happens" a reusable test fixture.
//
// Usage:
//   Backrooms.exe                       -> random seed from the clock
//   Backrooms.exe --seed radiation      -> a named fixture below
//   Backrooms.exe --seed 1788480606     -> an explicit numeric seed
//
// Header-only so no CMakeLists.txt source-list edits are needed.
// ============================================================================

#include <cstdlib>
#include <cstring>
#include <ctime>

namespace debugseeds {

struct NamedSeed {
  const char *name;
  unsigned int seed;
  const char *note;
};

// Add fixtures here as you find worlds worth returning to.
inline constexpr NamedSeed NAMED[] = {
    {"radiation", 1788480606u,
     "player spawns inside a radiation zone (magic mushrooms nearby)"},
    {"original", 1783608201u, "the original pinned development seed"},
};

inline constexpr int NAMED_COUNT = sizeof(NAMED) / sizeof(NAMED[0]);

// Looks up a fixture by name. Returns nullptr when there is no match.
inline const NamedSeed *findByName(const char *name) {
  for (int i = 0; i < NAMED_COUNT; ++i) {
    if (std::strcmp(NAMED[i].name, name) == 0) {
      return &NAMED[i];
    }
  }
  return nullptr;
}

// Resolves the seed for this run from the command line.
// Accepts "--seed <name>" or "--seed <number>"; falls back to the clock.
// outNote receives a human-readable description for logging.
inline unsigned int resolveFromArgs(int argc, char **argv,
                                    const char **outNote) {
  static const char *noteBuf = "random (from clock)";

  for (int i = 1; i < argc - 1; ++i) {
    if (std::strcmp(argv[i], "--seed") != 0) {
      continue;
    }
    const char *value = argv[i + 1];

    if (const NamedSeed *named = findByName(value)) {
      if (outNote)
        *outNote = named->note;
      return named->seed;
    }

    // Not a known name, so try to read it as a raw number.
    char *end = nullptr;
    unsigned long parsed = std::strtoul(value, &end, 10);
    if (end != value && *end == '\0') {
      if (outNote)
        *outNote = "explicit numeric seed";
      return (unsigned int)parsed;
    }
  }

  if (outNote)
    *outNote = noteBuf;
  return (unsigned int)std::time(nullptr);
}

} // namespace debugseeds
