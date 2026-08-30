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
    setCrouching(m_input.crouchHeld);

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
    // A crouching player digs his heels in: he keeps whatever momentum he had
    // and slides to a stop, but steering does nothing until he stands up.
    const float moveAxis = (m_crouching && m_physicsBody.isGrounded())
        ? 0.0f
        : std::clamp(m_input.moveAxis, -1.0f, 1.0f);

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

    if (m_input.jumpHeld && m_physicsBody.isGrounded()) {
        velocity.y = -m_movementConfig.jumpSpeed;
        m_physicsBody.setGrounded(false);
    }

    if (!m_input.jumpHeld
        && velocity.y < -m_movementConfig.jumpSpeed * m_movementConfig.jumpCutoff) {
        velocity.y = -m_movementConfig.jumpSpeed * m_movementConfig.jumpCutoff;
    }

    m_physicsBody.setVelocity(velocity);
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
    // A power already held stays where it is. Moving it back to the top would
    // reorder the stack - a Super mushroom taken as Fire Mario would sit above
    // Fire, and the next hit would then shrink the collider while the Fire
    // artwork stayed two tiles tall.
    if (std::find(m_powerStack.begin(), m_powerStack.end(), power)
        == m_powerStack.end()) {
        m_powerStack.push_back(power);
    }

    if (!wasSuper && power == PowerType::Super) {
        setSuperCollider(true);
    }
}

bool Player::removeLatestPower() {
    if (m_powerStack.empty()) {
        return false;
    }
    bool wasSuper = isSuper();
    m_powerStack.clear();
    if (wasSuper) {
        setSuperCollider(false);
    }
    return true;
}

void Player::setCrouching(bool crouching) noexcept {
    m_crouching = crouching && isSuper() && m_physicsBody.isGrounded();
}

bool Player::isCrouching() const noexcept {
    return m_crouching;
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
