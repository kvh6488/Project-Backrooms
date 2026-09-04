#pragma once

#include <memory>

// Forward declaration if we ever need the Application to be passed down
class Application;

// ============================================================================
// GameState — one screen's worth of game logic
// ============================================================================
// Application owns exactly one GameState at a time and drives it through
// update/render. PlayingState is the only concrete state today; the main menu
// and death screen are Phase 4.
//
// TRANSITIONS ARE A MAILBOX, NOT A CALL.
// A state never swaps itself out. It raises a request (requestState /
// requestQuit) and Application honours it *between* frames, once render() has
// returned. Swapping inside update() would destroy the object still executing
// on the stack — the classic use-after-free in a naive state machine.
// ============================================================================
class GameState {
public:
    virtual ~GameState() = default;

    // Called once when the state is pushed/switched to
    virtual void onEnter() {}

    // Called once when the state is removed/switched away from
    virtual void onExit() {}

    // Called every frame to update game logic
    virtual void update(float dt) = 0;

    // Called every frame to render graphics
    virtual void render() = 0;

    // --- Transition mailbox, read by Application after render() ---

    bool hasPendingTransition() const {
      return m_nextState != nullptr || m_wantsQuit;
    }
    bool wantsQuit() const { return m_wantsQuit; }

    // Hands ownership of the successor to the caller, clearing the request.
    std::unique_ptr<GameState> takeNextState() { return std::move(m_nextState); }

protected:
    // Ask Application to switch to `next` after this frame.
    void requestState(std::unique_ptr<GameState> next) {
      m_nextState = std::move(next);
    }

    // Ask Application to leave the main loop after this frame.
    void requestQuit() { m_wantsQuit = true; }

private:
    std::unique_ptr<GameState> m_nextState;
    bool m_wantsQuit = false;
};
