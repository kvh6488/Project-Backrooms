#pragma once

// Forward declaration if we ever need the Application to be passed down
class Application;

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
};
