#include "core/application.hpp"
#include "core/debug_log.hpp"
#include "core/debug_seeds.hpp"
#include "core/dev_mode.hpp"

int main(int argc, char **argv) {
  // Before anything that can log — seed resolution warns about a bad --seed.
  debuglog::enableAnsiColors();

  // A seed fully determines the world, so resolving it here (rather than
  // hardcoding one in PlayingState) turns "reproduce that bug" into a command
  // line flag instead of a recompile. See core/debug_seeds.hpp.
  const char *note = nullptr;
  unsigned int seed = debugseeds::resolveFromArgs(argc, argv, &note);

  // Without --dev the debug overlay is unreachable, so the game plays exactly
  // as a player would meet it. See core/dev_mode.hpp.
  bool devMode = devmode::enabledFromArgs(argc, argv);

  Application app(seed, note, devMode);
  app.run();
  return 0;
}
