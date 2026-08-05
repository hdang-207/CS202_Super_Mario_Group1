#include "Entities/Character.hpp"

namespace entity {

Character::Character(
    const sf::Vector2f& position,
    const sf::Vector2f& colliderSize,
    const sf::Vector2f& colliderOffset
)
    : m_physicsBody(
          position,
          colliderSize,
          colliderOffset
      ) {}

physics::PhysicsBody& Character::getPhysicsBody() noexcept {
    return m_physicsBody;
}

const physics::PhysicsBody&
Character::getPhysicsBody() const noexcept {
    return m_physicsBody;
}

void Character::setPosition(const sf::Vector2f& position) {
    m_physicsBody.setPosition(position);
}

const sf::Vector2f& Character::getPosition() const noexcept {
    return m_physicsBody.getPosition();
}

} // namespace entity