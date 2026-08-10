#include "Entities/Bullet.hpp"

namespace entity {

Bullet::Bullet(const sf::Texture& texture, sf::Vector2f position,
               sf::Vector2f velocity, float lifetime)
    : sprite(texture), velocity(velocity), remainingLifetime(lifetime) {
    const sf::Vector2u textureSize = texture.getSize();
    if (textureSize.x > 0 && textureSize.y > 0) {
        sprite.setScale({kSize / static_cast<float>(textureSize.x),
                         kSize / static_cast<float>(textureSize.y)});
    }
    sprite.setPosition(position);
}

void Bullet::update(sf::Time dt) {
    if (!active) {
        return;
    }
    sprite.move(velocity * dt.asSeconds());
    remainingLifetime -= dt.asSeconds();
    if (remainingLifetime <= 0.f) {
        active = false;
    }
}

void Bullet::draw(sf::RenderTarget& target) const {
    if (active) {
        target.draw(sprite);
    }
}

void Bullet::deactivate() noexcept {
    active = false;
}

sf::FloatRect Bullet::bounds() const {
    return sprite.getGlobalBounds();
}

sf::Vector2f Bullet::position() const noexcept {
    return sprite.getPosition();
}

bool Bullet::isActive() const noexcept {
    return active;
}

} // namespace entity
