#include "Entities/Paratroopa.hpp"
#include "Physics/PhysicsSystem.hpp"
#include <cmath>

namespace entity {

Paratroopa::Paratroopa(const sf::Vector2f& position, float tileSize, const sf::Texture* walkingTexture, const sf::Texture* shellTexture, float initialSpeed, float bounceSpeed, KoopaKind kind)
    : Koopa(position, tileSize, walkingTexture, shellTexture, kind, initialSpeed),
      m_hasWings(true),
      m_bounceSpeed(bounceSpeed) {}

void Paratroopa::update(float deltaTime) {
    if (!m_alive || !m_active) {
        return;
    }

    if (!m_hasWings) {
        Koopa::update(deltaTime);
        return;
    }

    m_bounceElapsed += deltaTime;
    m_position.x += m_velocity.x * deltaTime;
    m_position.y += std::sin(m_bounceElapsed * 4.f) * 1.2f;
    m_physicsBody.setPosition(m_position);

    m_animElapsed += deltaTime;
    if (m_animElapsed >= 0.2f) {
        m_animElapsed -= 0.2f;
        m_animationFrame = (m_animationFrame + 1) % 2;
    }
}

void Paratroopa::update(float deltaTime, physics::PhysicsSystem& physicsSystem, const std::vector<physics::AABB>& solids, float mapWidth, float mapHeight) {
    if (!m_alive || !m_active) {
        return;
    }

    if (!m_hasWings) {
        Koopa::update(deltaTime, physicsSystem, solids, mapWidth, mapHeight);
        return;
    }

    m_animElapsed += deltaTime;
    if (m_animElapsed >= 0.2f) {
        m_animElapsed -= 0.2f;
        m_animationFrame = (m_animationFrame + 1) % 2;
    }

    physicsSystem.update(m_physicsBody, solids, deltaTime);
    m_position = m_physicsBody.getPosition();
    m_velocity = m_physicsBody.getVelocity();

    if (m_physicsBody.isGrounded()) {
        m_velocity.y = -m_bounceSpeed;
        m_physicsBody.setVelocity(m_velocity);
        m_physicsBody.setGrounded(false);
    }

    if (m_physicsBody.hitWallLeft()) {
        m_velocity.x = m_walkSpeed;
        m_physicsBody.setVelocity(m_velocity);
        m_facingRight = true;
    } else if (m_physicsBody.hitWallRight()) {
        m_velocity.x = -m_walkSpeed;
        m_physicsBody.setVelocity(m_velocity);
        m_facingRight = false;
    }

    if (m_position.y > mapHeight + 100.f) {
        m_alive = false;
    }
}

void Paratroopa::stomp() {
    if (m_hasWings) {
        m_hasWings = false;
        m_velocity.y = 0.f;
        m_physicsBody.setVelocity(m_velocity);
    } else {
        Koopa::stomp();
    }
}

} // namespace entity
