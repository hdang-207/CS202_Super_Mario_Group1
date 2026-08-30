#include "Entities/Bullet.hpp"

namespace entity {

Bullet::Bullet(const sf::Texture& texture, sf::Vector2f position,
               sf::Vector2f velocity, float lifetime)
    : m_position(position),
      m_velocity(velocity),
      m_sprite(texture),
      m_remainingLifetime(lifetime) {
    const sf::Vector2u textureSize = texture.getSize();
    if (textureSize.x > 0 && textureSize.y > 0) {
        m_sprite.setScale({kSize / static_cast<float>(textureSize.x),
                           kSize / static_cast<float>(textureSize.y)});
    }
    m_sprite.setPosition(m_position);
}

void Bullet::update(sf::Time dt) {
    if (!m_active) {
        return;
    }

    const float seconds = dt.asSeconds();
    m_position += m_velocity * seconds;
    m_sprite.setPosition(m_position);

    m_remainingLifetime -= seconds;
    if (m_remainingLifetime <= 0.f) {
        m_active = false;
    }
}

void Bullet::draw(sf::RenderTarget& target) const {
    if (m_active) {
        target.draw(m_sprite);
    }
}

void Bullet::deactivate() noexcept {
    m_active = false;
}

sf::FloatRect Bullet::bounds() const {
    return m_sprite.getGlobalBounds();
}

sf::Vector2f Bullet::position() const noexcept {
    return m_position;
}

bool Bullet::isActive() const noexcept {
    return m_active;
}

} // namespace entity
