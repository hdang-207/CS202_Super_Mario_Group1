#include "Combat/Bomb.hpp"

namespace {
constexpr float kThrowSpeed = 360.f;
constexpr float kThrowUpSpeed = 620.f;
constexpr float kBombGravity = 1400.f;
}

namespace combat {

Bomb::Bomb(sf::Vector2f position, bool facingRight)
    : m_position(position),
      m_velocity(facingRight ? kThrowSpeed : -kThrowSpeed, -kThrowUpSpeed) {}

void Bomb::update(float deltaTime) {
    if (!m_active) {
        return;
    }
    m_velocity.y += kBombGravity * deltaTime;
    m_position += m_velocity * deltaTime;
    m_fuseRemaining -= deltaTime;
}

void Bomb::render(sf::RenderTarget& target) const {
    if (!m_active) {
        return;
    }
    sf::CircleShape shape(kSize * 0.5f);
    shape.setPosition(m_position);
    shape.setFillColor(sf::Color(35, 35, 35));
    shape.setOutlineColor(sf::Color(255, 140, 0));
    shape.setOutlineThickness(2.f);
    target.draw(shape);
}

sf::FloatRect Bomb::getBounds() const {
    return {m_position, {kSize, kSize}};
}

const sf::Vector2f& Bomb::getPosition() const noexcept {
    return m_position;
}

bool Bomb::isActive() const noexcept {
    return m_active;
}

bool Bomb::fuseExpired() const noexcept {
    return m_fuseRemaining <= 0.f;
}

void Bomb::deactivate() noexcept {
    m_active = false;
}

} // namespace combat
