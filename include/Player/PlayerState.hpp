#pragma once

#include <memory>

namespace entity {

class Player;

/**
 * @brief Base abstract class representing a player's state (State Pattern).
 */
class PlayerState {
public:
    virtual ~PlayerState() = default;
    virtual void handleInput(Player& player) = 0;
    virtual void update(Player& player, float deltaTime) = 0;
};

/**
 * @brief State when the player is idle on the ground.
 */
class StandingState : public PlayerState {
public:
    void handleInput(Player& player) override;
    void update(Player& player, float deltaTime) override;
};

/**
 * @brief State when the player is walking/running horizontally on the ground.
 */
class WalkingState : public PlayerState {
public:
    void handleInput(Player& player) override;
    void update(Player& player, float deltaTime) override;
};

/**
 * @brief State when the player is airborne (jumping or falling).
 */
class JumpingState : public PlayerState {
public:
    void handleInput(Player& player) override;
    void update(Player& player, float deltaTime) override;
};

} // namespace entity