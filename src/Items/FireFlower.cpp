#include "Items/FireFlower.hpp"

#include <SFML/Graphics/RenderWindow.hpp>
#include <SFML/Graphics/Sprite.hpp>
#include <algorithm>

namespace {
constexpr float kRiseDuration = 0.6f;
constexpr float kSizeRatio = 0.75f;
}

namespace items {

FireFlower::FireFlower(sf::Vector2f blockPosition)
    : m_blockPosition(blockPosition), m_position(blockPosition) {}

void FireFlower::update(float deltaTime, float tileSize) {
    m_size = tileSize * kSizeRatio;
    if (m_state != FireFlowerState::Emerging) {
        return;
    }

    m_elapsed = std::min(m_elapsed + deltaTime, kRiseDuration);
    const float progress = m_elapsed / kRiseDuration;
    const float centerX = m_blockPosition.x + (tileSize - m_size) / 2.f;
    m_position = {centerX, m_blockPosition.y - m_size * progress};

    if (m_elapsed >= kRiseDuration) {
        m_state = FireFlowerState::Ready;
    }
}

void FireFlower::render(sf::RenderWindow& window, const sf::Texture& texture,
                        float tileSize) const {
    sf::Sprite sprite(texture);
    const float flowerSize = tileSize * kSizeRatio;
    const sf::Vector2u textureSize = texture.getSize();
    if (textureSize.x > 0 && textureSize.y > 0) {
        sprite.setScale({flowerSize / static_cast<float>(textureSize.x),
                         flowerSize / static_cast<float>(textureSize.y)});
    }
    sprite.setPosition(m_position);
    window.draw(sprite);
}

sf::FloatRect FireFlower::getBounds() const {
    return {m_position, {m_size, m_size}};
}

bool FireFlower::isCollectible() const noexcept {
    return m_state == FireFlowerState::Ready;
}

} // namespace items
