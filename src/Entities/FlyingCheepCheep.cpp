#include "Entities/FlyingCheepCheep.hpp"

#include <SFML/Graphics/RenderTarget.hpp>
#include <SFML/Graphics/Sprite.hpp>

namespace entity {

namespace {
    constexpr int kFrameSize = 16;
    constexpr int kFrameCount = 2;
    constexpr float kFrameDuration = 0.16f;
    constexpr float kFlightGravity = 1400.f;
}

FlyingCheepCheep::FlyingCheepCheep(
    const sf::Vector2f& position, const sf::Vector2f& velocity, float tileSize,
    const sf::FloatRect& flightBounds, const sf::Texture* texture)
    : Character(position, {tileSize, tileSize}),
      m_texture(texture),
      m_flightBounds(flightBounds),
      m_tileSize(tileSize) {
    m_velocity = velocity;
    m_physicsBody.setVelocity(m_velocity);
}

void FlyingCheepCheep::update(float deltaTime) {
    if (!m_alive || deltaTime <= 0.f) {
        return;
    }

    m_animationElapsed += deltaTime;
    while (m_animationElapsed >= kFrameDuration) {
        m_animationElapsed -= kFrameDuration;
        m_animationFrame = (m_animationFrame + 1) % kFrameCount;
    }

    m_velocity.y += kFlightGravity * deltaTime;
    m_position += m_velocity * deltaTime;
    m_physicsBody.setPosition(m_position);
    m_physicsBody.setVelocity(m_velocity);

    const float left = m_flightBounds.position.x - m_tileSize * 2.f;
    const float right = m_flightBounds.position.x + m_flightBounds.size.x
                      + m_tileSize * 2.f;
    const float bottom = m_flightBounds.position.y + m_flightBounds.size.y
                       + m_tileSize * 2.f;
    if (m_position.x + m_tileSize < left || m_position.x > right
        || (m_velocity.y > 0.f && m_position.y > bottom)) {
        m_alive = false;
    }
}

void FlyingCheepCheep::render(sf::RenderTarget& target) const {
    if (!m_alive || m_texture == nullptr) {
        return;
    }

    sf::Sprite sprite(*m_texture);
    sprite.setTextureRect(sf::IntRect(
        {m_animationFrame * kFrameSize, 0}, {kFrameSize, kFrameSize}));
    const float scale = m_tileSize / static_cast<float>(kFrameSize);
    if (m_velocity.x > 0.f) {
        sprite.setOrigin({static_cast<float>(kFrameSize), 0.f});
        sprite.setScale({-scale, scale});
    } else {
        sprite.setScale({scale, scale});
    }
    sprite.setPosition(m_position);
    target.draw(sprite);
}

} // namespace entity
