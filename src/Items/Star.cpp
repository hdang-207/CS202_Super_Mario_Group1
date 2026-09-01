#include "Items/Star.hpp"
#include "Physics/AABB.hpp"

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

Star::Star(sf::Vector2f blockPosition, const sf::Texture* texture, float scale)
    : Item(blockPosition),
      m_texture(texture),
      m_scale(scale),
      m_blockPosition(blockPosition),
      m_size(16.f * scale) {}

void Star::update(sf::Time dt) {
    update(dt.asSeconds());
}

void Star::update(float deltaTime) {
    if (!m_alive) return;
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

    m_velocity.y = std::min(m_velocity.y + kGravity * deltaTime, kMaxFallSpeed);
    m_position += m_velocity * deltaTime;
}

void Star::update(float deltaTime, float tileSize, const std::vector<physics::AABB>& solids) {
    m_size = tileSize;

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

    m_velocity.y = std::min(m_velocity.y + kGravity * deltaTime, kMaxFallSpeed);

    // Move X and resolve solid wall collision
    m_position.x += m_velocity.x * deltaTime;
    physics::AABB xBounds({m_position.x, m_position.y + 1.f}, {m_size, m_size - 2.f});
    for (const auto& solid : solids) {
        if (xBounds.intersects(solid)) {
            if (m_velocity.x > 0.f) {
                m_position.x = solid.left() - m_size;
            } else {
                m_position.x = solid.right();
            }
            m_velocity.x = -m_velocity.x;
            break;
        }
    }

    // Move Y and resolve solid floor/ceiling collision
    m_position.y += m_velocity.y * deltaTime;
    physics::AABB yBounds({m_position.x + 1.f, m_position.y}, {m_size - 2.f, m_size});
    for (const auto& solid : solids) {
        if (yBounds.intersects(solid)) {
            if (m_velocity.y > 0.f) {
                m_position.y = solid.top() - m_size;
                m_velocity.y = -kStarBounceSpeed; // Bounce up!
            } else {
                m_position.y = solid.bottom();
                m_velocity.y = 0.f;
            }
            break;
        }
    }
}

void Star::render(sf::RenderWindow& window) const {
    if (!m_alive || !m_texture) return;
    render(window, *m_texture, m_scale);
}

void Star::render(sf::RenderWindow& window, const sf::Texture& texture,
                  float scale) const {
    if (!m_alive) return;
    sf::Sprite sprite(texture);
    sprite.setScale({scale, scale});
    sprite.setPosition(m_position);
    window.draw(sprite);
}

sf::FloatRect Star::getBounds() const {
    return sf::FloatRect(m_position, {16.f * m_scale, 16.f * m_scale});
}

bool Star::isCollectible() const noexcept {
    return m_state == StarState::Moving && m_alive;
}

bool Star::hasFallenOut(float worldHeight) const noexcept {
    return m_position.y > worldHeight;
}

} // namespace items
