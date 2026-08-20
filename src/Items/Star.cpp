#include "Items/Star.hpp"

#include "Systems/TileMap.hpp"

#include <SFML/Graphics/RenderWindow.hpp>
#include <SFML/Graphics/Sprite.hpp>
#include <algorithm>

namespace {
constexpr float kRiseDuration = 0.45f;
constexpr float kStarSpeed = 150.f;
constexpr float kStarBounceSpeed = 650.f;
constexpr float kGravity = 2400.f;
constexpr float kMaxFallSpeed = 1400.f;
}

namespace items {

Star::Star(sf::Vector2f blockPosition)
    : m_blockPosition(blockPosition), m_position(blockPosition) {}

void Star::update(float deltaTime, const TileMap& tileMap) {
    m_size = tileMap.tileSize();

    if (m_state == StarState::Emerging) {
        m_elapsed = std::min(m_elapsed + deltaTime, kRiseDuration);
        const float progress = m_elapsed / kRiseDuration;
        m_position = {m_blockPosition.x,
                      m_blockPosition.y - m_size * progress};
        if (m_elapsed >= kRiseDuration) {
            m_state = StarState::Moving;
            m_velocity = {kStarSpeed, -kStarBounceSpeed};
        }
        return;
    }

    m_velocity.y = std::min(m_velocity.y + kGravity * deltaTime,
                            kMaxFallSpeed);

    m_position.x += m_velocity.x * deltaTime;
    sf::FloatRect xBounds(m_position, {m_size, m_size});
    xBounds.position.y += 1.f;
    xBounds.size.y -= 2.f;
    for (const sf::FloatRect& tile : tileMap.solidTilesOverlapping(xBounds)) {
        if (m_velocity.x > 0.f) {
            m_position.x = tile.position.x - m_size;
        } else {
            m_position.x = tile.position.x + tile.size.x;
        }
        m_velocity.x = -m_velocity.x;
        break;
    }

    m_position.y += m_velocity.y * deltaTime;
    sf::FloatRect yBounds(m_position, {m_size, m_size});
    yBounds.position.x += 1.f;
    yBounds.size.x -= 2.f;
    for (const sf::FloatRect& tile : tileMap.solidTilesOverlapping(yBounds)) {
        if (m_velocity.y > 0.f) {
            m_position.y = tile.position.y - m_size;
            m_velocity.y = -kStarBounceSpeed;
        } else {
            m_position.y = tile.position.y + tile.size.y;
            m_velocity.y = 0.f;
        }
        break;
    }
}

void Star::render(sf::RenderWindow& window, const sf::Texture& texture,
                  float scale) const {
    sf::Sprite sprite(texture);
    sprite.setScale({scale, scale});
    sprite.setPosition(m_position);
    window.draw(sprite);
}

sf::FloatRect Star::getBounds() const {
    return {m_position, {m_size, m_size}};
}

bool Star::isCollectible() const noexcept {
    return m_state == StarState::Moving;
}

bool Star::hasFallenOut(float worldHeight) const noexcept {
    return m_position.y > worldHeight;
}

} // namespace items
