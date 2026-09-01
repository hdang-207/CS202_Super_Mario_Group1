#include "Entities/Character.hpp"
#include <SFML/Graphics/RenderWindow.hpp>

namespace entity {

Character::Character(
    const sf::Vector2f& position,
    const sf::Vector2f& colliderSize,
    const sf::Vector2f& colliderOffset
)
    : Entity(position),
      m_physicsBody(
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
    m_position = position;
    m_physicsBody.setPosition(position);
}

const sf::Vector2f& Character::getPosition() const noexcept {
    return m_physicsBody.getPosition();
}

void Character::render(sf::RenderWindow& window) const {
    render(static_cast<sf::RenderTarget&>(window));
}

} // namespace entity