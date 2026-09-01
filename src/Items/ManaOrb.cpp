#include "Items/ManaOrb.hpp"

#include <SFML/Graphics/RenderWindow.hpp>
#include <SFML/Graphics/Sprite.hpp>

#include <algorithm>

namespace {
constexpr float kRiseDuration = 0.45f;
}

namespace items {

ManaOrb::ManaOrb(sf::Vector2f blockPosition, const sf::Texture* texture,
                 float scale)
    : Item(blockPosition),
      m_texture(texture),
      m_scale(scale),
      m_blockPosition(blockPosition),
      m_size(16.f * scale) {}

void ManaOrb::update(sf::Time dt) {
    update(dt.asSeconds());
}

void ManaOrb::update(float deltaTime) {
    update(deltaTime, 16.f * m_scale);
}

void ManaOrb::update(float deltaTime, float tileSize) {
    if (!m_alive) {
        return;
    }

    m_size = tileSize;
    if (m_state != ManaOrbState::Emerging) {
        return;
    }

    m_elapsed = std::min(m_elapsed + deltaTime, kRiseDuration);
    const float progress = m_elapsed / kRiseDuration;
    m_position = {m_blockPosition.x, m_blockPosition.y - tileSize * progress};

    if (m_elapsed >= kRiseDuration) {
        m_state = ManaOrbState::Ready;
        m_position = {m_blockPosition.x, m_blockPosition.y - tileSize};
    }
}

void ManaOrb::render(sf::RenderWindow& window) const {
    if (!m_alive || !m_texture) {
        return;
    }
    render(window, *m_texture, 16.f * m_scale);
}

void ManaOrb::render(sf::RenderWindow& window, const sf::Texture& texture,
                     float tileSize) const {
    if (!m_alive) {
        return;
    }

    sf::Sprite sprite(texture);
    const sf::Vector2u textureSize = texture.getSize();
    if (textureSize.x > 0 && textureSize.y > 0) {
        sprite.setScale({tileSize / static_cast<float>(textureSize.x),
                         tileSize / static_cast<float>(textureSize.y)});
    }
    sprite.setPosition(m_position);
    window.draw(sprite);
}

sf::FloatRect ManaOrb::getBounds() const {
    return sf::FloatRect(m_position, {m_size, m_size});
}

bool ManaOrb::isCollectible() const noexcept {
    return m_state == ManaOrbState::Ready && m_alive;
}

} // namespace items
