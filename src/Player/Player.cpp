#include "Player/Player.hpp"

#include <algorithm>

namespace entity {

Player::Player(
    const sf::Vector2f& position,
    const sf::Vector2f& colliderSize,
    const sf::Vector2f& colliderOffset,
    const PlayerMovementConfig& movementConfig
)
    : Character(
          position,
          colliderSize,
          colliderOffset
      ),
      m_movementConfig(movementConfig) {}

void Player::setInput(const PlayerInput& input) noexcept {
    m_input = input;
}

void Player::update(float deltaTime) {
    // Current movement version sets velocity directly, deltaTime unreferenced for now
    (void)deltaTime;

    processHorizontalMovement();
    processJump();
}

void Player::processHorizontalMovement() {
    const float moveAxis = std::clamp(
        m_input.moveAxis,
        -1.0f,
        1.0f
    );

    sf::Vector2f velocity = m_physicsBody.getVelocity();

    velocity.x =
        moveAxis * m_movementConfig.moveSpeed;

    m_physicsBody.setVelocity(velocity);
}

void Player::processJump() {
    if (!m_input.jumpHeld) {
        return;
    }

    if (!m_physicsBody.isGrounded()) {
        return;
    }

    sf::Vector2f velocity = m_physicsBody.getVelocity();

    // In SFML coordinate system: negative Y is upward
    velocity.y = -m_movementConfig.jumpSpeed;

    m_physicsBody.setVelocity(velocity);
}

const PlayerInput& Player::getInput() const noexcept {
    return m_input;
}

const PlayerMovementConfig&
Player::getMovementConfig() const noexcept {
    return m_movementConfig;
}

} // namespace entity