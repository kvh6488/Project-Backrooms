#pragma once

// ============================================================================
// ScriptedInput — a Scenario played back as one InputState per tick
// ============================================================================
// The InputSource the headless harness plugs into Application in place of
// the keyboard. The command list is compiled once, up front, into a flat
// timeline of frames (an InputState plus an optional checkpoint name), so
// sample(tick) is an array read and the whole run is known before the first
// tick - which is what lets the harness create output directories, and a
// test assert the timeline, without a window.
//
// The key -> field mapping below mirrors pollHardwareInput exactly, held and
// pressed included: a held key sets its "held" fields every tick and its
// "pressed" fields on the first tick only, the way IsKeyDown/IsKeyPressed
// behave. Arrow keys feed both movement (held) and nav (pressed), as on
// hardware. Memory is one frame per tick: ~60 bytes, so an hour of game time
// is ~13 MB and a typical scenario is kilobytes.
// ============================================================================

#include "core/input_state.hpp"
#include "dev/scenario.hpp"

#include <string>
#include <vector>

class ScriptedInput : public InputSource {
public:
  struct Frame {
    InputState in;
    std::string checkpoint; // empty on an ordinary tick
  };

  explicit ScriptedInput(const scenario::Scenario &sc) { compile(sc); }

  InputState sample(int tick) override {
    if (tick < 0 || tick >= (int)m_frames.size())
      return InputState{};
    return m_frames[tick].in;
  }

  bool finished(int tick) const override {
    return tick >= (int)m_frames.size();
  }

  // Name of the checkpoint recorded on `tick`, or null.
  const std::string *checkpointAt(int tick) const {
    if (tick < 0 || tick >= (int)m_frames.size() ||
        m_frames[tick].checkpoint.empty())
      return nullptr;
    return &m_frames[tick].checkpoint;
  }

  int tickCount() const { return (int)m_frames.size(); }

  // Every checkpoint, in tick order - the harness lists them in run.json.
  std::vector<std::string> checkpointNames() const {
    std::vector<std::string> names;
    for (const Frame &f : m_frames)
      if (!f.checkpoint.empty())
        names.push_back(f.checkpoint);
    return names;
  }

private:
  std::vector<Frame> m_frames;

  // Mouse position is state on hardware, so it is state here too.
  Vector2 m_cursor = {0.0f, 0.0f};

  void pushIdle(int n, const std::string &checkpoint = "") {
    for (int i = 0; i < n; ++i) {
      Frame f;
      f.in.mouse = m_cursor;
      f.checkpoint = checkpoint;
      m_frames.push_back(f);
    }
  }

  static void applyKey(InputState &in, const std::string &key, bool first) {
    // --- held ---
    if (key == "W" || key == "UP") in.moveUp = true;
    if (key == "S" || key == "DOWN") in.moveDown = true;
    if (key == "A" || key == "LEFT") in.moveLeft = true;
    if (key == "D" || key == "RIGHT") in.moveRight = true;
    if (!first)
      return;
    // --- pressed, first tick only ---
    if (key == "K") in.door1 = true;
    if (key == "L") in.door2 = true;
    if (key == "P") in.pickup = true;
    if (key == "I") in.toggleInventory = true;
    if (key == "O") in.openCupboard = true;
    if (key == "U") in.use = true;
    if (key == "Q") in.place = true;
    if (key == "UP") in.navUp = true;
    if (key == "DOWN") in.navDown = true;
    if (key == "LEFT") in.navLeft = true;
    if (key == "RIGHT") in.navRight = true;
    if (key.size() == 1 && key[0] >= '1' && key[0] <= '5')
      in.hotbar[key[0] - '1'] = true;
  }

  void compile(const scenario::Scenario &sc) {
    using K = scenario::Command::Kind;
    for (const scenario::Command &c : sc.commands) {
      switch (c.kind) {
      case K::Wait:
        pushIdle(c.ticks);
        break;
      case K::Hold:
        for (int i = 0; i < c.ticks; ++i) {
          Frame f;
          f.in.mouse = m_cursor;
          applyKey(f.in, c.key, i == 0);
          m_frames.push_back(f);
        }
        break;
      case K::Mouse:
        m_cursor = {(float)c.x, (float)c.y};
        break; // consumes no tick; the next frame carries the new position
      case K::Click:
      case K::RClick: {
        m_cursor = {(float)c.x, (float)c.y};
        Frame f;
        f.in.mouse = m_cursor;
        (c.kind == K::Click ? f.in.mouseLeftPressed : f.in.mouseRightPressed) =
            true;
        m_frames.push_back(f);
        break;
      }
      case K::Checkpoint:
        pushIdle(1, c.name);
        break;
      case K::End:
        return;
      }
    }
  }
};
