#include "Entities/Paratroopa.hpp"
#include "Physics/PhysicsSystem.hpp"

namespace entity {

Paratroopa::Paratroopa(const sf::Vector2f& position, float tileSize, float initialSpeed, float bounceSpeed)
    : Koopa(position, tileSize, KoopaKind::Green, initialSpeed),
      m_bounceSpeed(bounceSpeed),
      m_hasWings(true) {}

void Paratroopa::update(float deltaTime, physics::PhysicsSystem& physicsSystem, const std::vector<physics::AABB>& solids, float mapWidth, float mapHeight) {
    if (!m_alive || !m_active) {
        return;
    }

    if (m_hasWings && m_state == KoopaState::Walking && m_physicsBody.isGrounded()) {
        sf::Vector2f vel = m_physicsBody.getVelocity();
        vel.y = -m_bounceSpeed;
        m_physicsBody.setVelocity(vel);
        m_physicsBody.setGrounded(false);
    }

    Koopa::update(deltaTime, physicsSystem, solids, mapWidth, mapHeight);
}

void Paratroopa::removeWings() {
    m_hasWings = false;
}

} // namespace entity
