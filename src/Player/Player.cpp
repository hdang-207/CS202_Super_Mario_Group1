#include "Player/Player.hpp"

#include <algorithm>
#include <cmath>

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
      m_movementConfig(movementConfig),
      m_currentState(std::make_unique<StandingState>()) {}

void Player::setInput(const PlayerInput& input) noexcept {
    m_input = input;
}

void Player::update(float deltaTime) {
    if (m_currentState) {
        m_currentState->handleInput(*this);
        m_currentState->update(*this, deltaTime);
    }

    processHorizontalMovement(deltaTime);
    processJump();
}

void Player::render(sf::RenderTarget& target) const {
    (void)target;
    // Rendering logic for player sprite can be delegating or custom
}

void Player::changeState(std::unique_ptr<PlayerState> newState) {
    if (newState) {
        m_currentState = std::move(newState);
    }
}

const PlayerState* Player::getCurrentState() const noexcept {
    return m_currentState.get();
}

void Player::processHorizontalMovement(float deltaTime) {
    const float moveAxis = std::clamp(
        m_input.moveAxis,
        -1.0f,
        1.0f
    );

    sf::Vector2f velocity = m_physicsBody.getVelocity();

    if (moveAxis != 0.0f) {
        velocity.x += moveAxis * m_movementConfig.walkAcceleration * deltaTime;
        velocity.x = std::clamp(
            velocity.x,
            -m_movementConfig.moveSpeed,
            m_movementConfig.moveSpeed
        );
    } else {
        const float friction = m_physicsBody.isGrounded()
            ? m_movementConfig.groundFriction
            : m_movementConfig.groundFriction
                * m_movementConfig.airFrictionMultiplier;
        const float drop = friction * deltaTime;

        if (std::abs(velocity.x) <= drop) {
            velocity.x = 0.0f;
        } else {
            velocity.x -= std::copysign(drop, velocity.x);
        }
    }

    m_physicsBody.setVelocity(velocity);
}

void Player::processJump() {
    sf::Vector2f velocity = m_physicsBody.getVelocity();

    if (m_input.jumpHeld
        && !m_jumpWasHeldLastFrame
        && m_physicsBody.isGrounded()) {
        velocity.y = -m_movementConfig.jumpSpeed;
        m_physicsBody.setGrounded(false);
    }

    if (!m_input.jumpHeld
        && velocity.y < -m_movementConfig.jumpSpeed * m_movementConfig.jumpCutoff) {
        velocity.y = -m_movementConfig.jumpSpeed * m_movementConfig.jumpCutoff;
    }

    m_physicsBody.setVelocity(velocity);
    m_jumpWasHeldLastFrame = m_input.jumpHeld;
}

const PlayerInput& Player::getInput() const noexcept {
    return m_input;
}

const PlayerMovementConfig&
Player::getMovementConfig() const noexcept {
    return m_movementConfig;
}

} // namespace entity
