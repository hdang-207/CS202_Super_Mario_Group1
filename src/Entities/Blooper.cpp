#include "Entities/Blooper.hpp"

#include <SFML/Graphics/RenderTarget.hpp>
#include <SFML/Graphics/Sprite.hpp>
#include <algorithm>
#include <cmath>

namespace entity {

namespace {
    constexpr int kFrameWidth = 16;
    constexpr int kFrameHeight = 24;
    constexpr int kFrameCount = 2;
    constexpr float kFrameDuration = 0.24f;
    constexpr float kHorizontalSpeed = 54.f;
    constexpr float kVerticalSpeed = 64.f;
    constexpr float kPulseSpeed = 3.4f;
}

Blooper::Blooper(const sf::Vector2f& position, float tileSize,
                 const sf::FloatRect& swimBounds, const sf::Texture* texture)
    : Character(position, {tileSize, tileSize * 1.5f}),
      m_texture(texture),
      m_swimBounds(swimBounds),
      m_target(position),
      m_tileSize(tileSize) {
    // It must remain at its map marker until the camera approaches; otherwise
    // every Blooper would start moving while Mario is still at the entrance.
    m_active = false;
}

void Blooper::update(float deltaTime) {
    if (!m_alive || !m_active || deltaTime <= 0.f) {
        return;
    }

    m_animationElapsed += deltaTime;
    while (m_animationElapsed >= kFrameDuration) {
        m_animationElapsed -= kFrameDuration;
        m_animationFrame = (m_animationFrame + 1) % kFrameCount;
    }

    m_movementElapsed += deltaTime;
    const float deltaX = m_target.x - m_position.x;
    const float deltaY = m_target.y - m_position.y;
    const float pulse = std::sin(m_movementElapsed * kPulseSpeed);

    m_velocity.x = std::clamp(deltaX * 0.45f,
                              -kHorizontalSpeed, kHorizontalSpeed);
    m_velocity.y = std::clamp(deltaY * 0.35f,
                              -kVerticalSpeed, kVerticalSpeed)
                 - std::max(0.f, pulse) * 34.f;

    m_position += m_velocity * deltaTime;

    const float spriteHeight = m_tileSize * 1.5f;
    const float floorClearance = m_tileSize * 0.5f;
    const float maxX = m_swimBounds.position.x + m_swimBounds.size.x - m_tileSize;
    const float maxY = m_swimBounds.position.y + m_swimBounds.size.y
                     - spriteHeight - floorClearance;
    m_position.x = std::clamp(m_position.x, m_swimBounds.position.x, maxX);
    m_position.y = std::clamp(m_position.y, m_swimBounds.position.y, maxY);

    m_physicsBody.setPosition(m_position);
    m_physicsBody.setVelocity(m_velocity);
}

void Blooper::render(sf::RenderTarget& target) const {
    if (!m_alive || !m_active || m_texture == nullptr) {
        return;
    }

    sf::Sprite sprite(*m_texture);
    sprite.setTextureRect(sf::IntRect(
        {m_animationFrame * kFrameWidth, 0},
        {kFrameWidth, kFrameHeight}
    ));
    const float scale = m_tileSize / static_cast<float>(kFrameWidth);
    sprite.setScale({scale, scale});
    sprite.setPosition(m_position);
    target.draw(sprite);
}

} // namespace entity
