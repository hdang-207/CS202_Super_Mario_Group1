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
      m_currentState(std::make_unique<StandingState>()),
      m_smallColliderSize(colliderSize) {}

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

void Player::applyPower(PowerType power) {
    const bool wasSuper = isSuper();
    const auto existing = std::find(m_powerStack.begin(), m_powerStack.end(), power);
    if (existing != m_powerStack.end()) {
        m_powerStack.erase(existing);
    }
    m_powerStack.push_back(power);

    if (!wasSuper && power == PowerType::Super) {
        setSuperCollider(true);
    }
}

bool Player::removeLatestPower() {
    if (m_powerStack.empty()) {
        return false;
    }

    const PowerType removed = m_powerStack.back();
    m_powerStack.pop_back();
    if (removed == PowerType::Super) {
        setSuperCollider(false);
    }
    return true;
}

bool Player::hasPower(PowerType power) const noexcept {
    return std::find(m_powerStack.begin(), m_powerStack.end(), power)
        != m_powerStack.end();
}

bool Player::isSuper() const noexcept {
    return hasPower(PowerType::Super);
}

bool Player::hasFirePower() const noexcept {
    return hasPower(PowerType::Fire);
}

void Player::setSuperCollider(bool super) {
    const sf::Vector2f oldSize = m_physicsBody.getColliderSize();
    sf::Vector2f newSize = m_smallColliderSize;
    if (super) {
        newSize.y *= 2.0f;
    }

    sf::Vector2f position = m_physicsBody.getPosition();
    const float feetY = position.y + oldSize.y;
    position.y = feetY - newSize.y;
    m_physicsBody.setPosition(position);
    m_physicsBody.setCollider(newSize);
}

} // namespace entity
