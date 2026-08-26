#include "Entities/CheepCheep.hpp"

#include <SFML/Graphics/RenderTarget.hpp>
#include <SFML/Graphics/Sprite.hpp>
#include <algorithm>
#include <cmath>

namespace entity {

namespace {
    constexpr int kFrameSize = 16;
    constexpr int kFrameCount = 2;
    constexpr float kFrameDuration = 0.18f;
    constexpr float kWaveSpeed = 2.8f;
}

CheepCheep::CheepCheep(const sf::Vector2f& position, float tileSize,
                       const sf::FloatRect& swimBounds,
                       const sf::Texture* texture, float swimSpeed)
    : Character(position, {tileSize, tileSize}),
      m_texture(texture),
      m_swimBounds(swimBounds),
      m_tileSize(tileSize),
      m_originY(position.y),
      m_swimSpeed(swimSpeed) {
    m_active = false;
    m_velocity = {-m_swimSpeed, 0.f};
    m_physicsBody.setVelocity(m_velocity);
}

void CheepCheep::update(float deltaTime) {
    if (!m_alive || !m_active || deltaTime <= 0.f) {
        return;
    }

    m_animationElapsed += deltaTime;
    while (m_animationElapsed >= kFrameDuration) {
        m_animationElapsed -= kFrameDuration;
        m_animationFrame = (m_animationFrame + 1) % kFrameCount;
    }

    m_movementElapsed += deltaTime;
    m_position.x -= m_swimSpeed * deltaTime;
    m_position.y = m_originY
                 + std::sin(m_movementElapsed * kWaveSpeed) * m_tileSize * 0.28f;

    const float maxY = m_swimBounds.position.y + m_swimBounds.size.y - m_tileSize;
    m_position.y = std::clamp(m_position.y, m_swimBounds.position.y, maxY);
    m_velocity = {-m_swimSpeed,
                  std::cos(m_movementElapsed * kWaveSpeed)
                      * m_tileSize * 0.28f * kWaveSpeed};
    m_physicsBody.setPosition(m_position);
    m_physicsBody.setVelocity(m_velocity);

    if (m_position.x + m_tileSize < m_swimBounds.position.x) {
        m_alive = false;
    }
}

void CheepCheep::render(sf::RenderTarget& target) const {
    if (!m_alive || !m_active || m_texture == nullptr) {
        return;
    }

    sf::Sprite sprite(*m_texture);
    sprite.setTextureRect(sf::IntRect(
        {m_animationFrame * kFrameSize, 0},
        {kFrameSize, kFrameSize}
    ));
    const float scale = m_tileSize / static_cast<float>(kFrameSize);
    sprite.setScale({scale, scale});
    sprite.setPosition(m_position);
    target.draw(sprite);
}

} // namespace entity
