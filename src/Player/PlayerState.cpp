#include "Player/PlayerState.hpp"
#include "Player/Player.hpp"

#include <cmath>

namespace entity {

// --- StandingState ---

void StandingState::handleInput(Player& player) {
    const auto& input = player.getInput();

    if (input.jumpHeld && player.getPhysicsBody().isGrounded()) {
        player.changeState(std::make_unique<JumpingState>());
        return;
    }

    if (std::abs(input.moveAxis) > 0.01f) {
        player.changeState(std::make_unique<WalkingState>());
        return;
    }
}

void StandingState::update(Player& player, float deltaTime) {
    (void)deltaTime;
    // Decelerate or stay standing
}

// --- WalkingState ---

void WalkingState::handleInput(Player& player) {
    const auto& input = player.getInput();

    if (input.jumpHeld && player.getPhysicsBody().isGrounded()) {
        player.changeState(std::make_unique<JumpingState>());
        return;
    }

    if (std::abs(input.moveAxis) <= 0.01f &&
        std::abs(player.getPhysicsBody().getVelocity().x) < 1.0f) {
        player.changeState(std::make_unique<StandingState>());
        return;
    }
}

void WalkingState::update(Player& player, float deltaTime) {
    (void)deltaTime;
}

// --- JumpingState ---

void JumpingState::handleInput(Player& player) {
    const auto& input = player.getInput();

    if (player.getPhysicsBody().isGrounded()) {
        if (std::abs(input.moveAxis) > 0.01f) {
            player.changeState(std::make_unique<WalkingState>());
        } else {
            player.changeState(std::make_unique<StandingState>());
        }
    }
}

void JumpingState::update(Player& player, float deltaTime) {
    (void)deltaTime;
}

} // namespace entity
