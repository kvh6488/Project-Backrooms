#include "core/application.hpp"
#include "core/debug_seeds.hpp"

int main(int argc, char **argv) {
  // A seed fully determines the world, so resolving it here (rather than
  // hardcoding one in PlayingState) turns "reproduce that bug" into a command
  // line flag instead of a recompile. See core/debug_seeds.hpp.
  const char *note = nullptr;
  unsigned int seed = debugseeds::resolveFromArgs(argc, argv, &note);

  Application app(seed, note);
  app.run();
  return 0;
}
