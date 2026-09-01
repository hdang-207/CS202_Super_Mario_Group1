#include "Items/FireFlower.hpp"

#include <SFML/Graphics/RenderWindow.hpp>
#include <SFML/Graphics/Sprite.hpp>
#include <algorithm>

namespace {
constexpr float kRiseDuration = 0.45f;
}

namespace items {

FireFlower::FireFlower(sf::Vector2f blockPosition, const sf::Texture* texture, float scale)
    : Item(blockPosition),
      m_texture(texture),
      m_scale(scale),
      m_blockPosition(blockPosition),
      m_size(16.f * scale) {}

void FireFlower::update(sf::Time dt) {
    update(dt.asSeconds());
}

void FireFlower::update(float deltaTime) {
    update(deltaTime, 16.f * m_scale);
}

void FireFlower::update(float deltaTime, float tileSize) {
    if (!m_alive) return;
    m_size = tileSize;
    if (m_state != FireFlowerState::Emerging) {
        return;
    }

    m_elapsed = std::min(m_elapsed + deltaTime, kRiseDuration);
    const float progress = m_elapsed / kRiseDuration;
    m_position = {m_blockPosition.x, m_blockPosition.y - tileSize * progress};

    if (m_elapsed >= kRiseDuration) {
        m_state = FireFlowerState::Ready;
        m_position = {m_blockPosition.x, m_blockPosition.y - tileSize};
    }
}

void FireFlower::render(sf::RenderWindow& window) const {
    if (!m_alive || !m_texture) return;
    render(window, *m_texture, 16.f * m_scale);
}

void FireFlower::render(sf::RenderWindow& window, const sf::Texture& texture,
                        float tileSize) const {
    if (!m_alive) return;
    sf::Sprite sprite(texture);
    const sf::Vector2u textureSize = texture.getSize();
    if (textureSize.x > 0 && textureSize.y > 0) {
        sprite.setScale({tileSize / static_cast<float>(textureSize.x),
                         tileSize / static_cast<float>(textureSize.y)});
    }
    sprite.setPosition(m_position);
    window.draw(sprite);
}

sf::FloatRect FireFlower::getBounds() const {
    return sf::FloatRect(m_position, {m_size, m_size});
}

bool FireFlower::isCollectible() const noexcept {
    return m_state == FireFlowerState::Ready && m_alive;
}

} // namespace items
