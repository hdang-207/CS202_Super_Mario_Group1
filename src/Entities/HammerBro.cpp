#include "Entities/HammerBro.hpp"
#include "Physics/PhysicsSystem.hpp"
#include <SFML/Graphics/RenderTarget.hpp>
#include <SFML/Graphics/Sprite.hpp>
#include <cmath>

namespace entity {

namespace {
    constexpr int kSourceTileSize = 16;
    constexpr int kSourceHeight = 24;      ///< A Bro is a tile and a half tall.
    constexpr int kFrameCount = 2;
    constexpr float kFrameDuration = 0.25f;
    constexpr float kThrowInterval = 2.0f;
    /// How far either side of its marker the Bro shuffles, in tiles.
    constexpr float kPatrolTiles = 1.f;
}

HammerBro::HammerBro(const sf::Vector2f& position, float tileSize, const sf::Texture* texture)
    : Character(position, {tileSize, tileSize * 1.5f}),
      m_texture(texture),
      m_tileSize(tileSize),
      m_originX(position.x) {
    m_active = true;
    m_velocity = {-m_walkSpeed, 0.f};
    m_physicsBody.setVelocity(m_velocity);
}

void HammerBro::update(float deltaTime) {
    if (!m_alive || !m_active) {
        return;
    }

    m_animElapsed += deltaTime;
    while (m_animElapsed >= kFrameDuration) {
        m_animElapsed -= kFrameDuration;
        m_animationFrame = (m_animationFrame + 1) % kFrameCount;
    }

    m_throwTimer += deltaTime;
    if (m_throwTimer >= kThrowInterval) {
        m_throwTimer -= kThrowInterval;
        m_pendingThrow = true;
    }
}

void HammerBro::update(float deltaTime, physics::PhysicsSystem& physicsSystem,
                       const std::vector<physics::AABB>& solids,
                       float mapWidth, float mapHeight) {
    if (!m_alive || !m_active) {
        return;
    }

    update(deltaTime);

    // Shuffle inside a fixed span so the Bro can never walk off its own bricks.
    const float patrol = kPatrolTiles * m_tileSize;
    sf::Vector2f velocity = m_physicsBody.getVelocity();
    if (m_position.x <= m_originX - patrol) {
        velocity.x = m_walkSpeed;
    } else if (m_position.x >= m_originX + patrol) {
        velocity.x = -m_walkSpeed;
    }
    m_physicsBody.setVelocity(velocity);

    physicsSystem.update(m_physicsBody, solids, deltaTime);
    m_position = m_physicsBody.getPosition();
    m_velocity = m_physicsBody.getVelocity();

    if (m_physicsBody.hitWallLeft()) {
        m_velocity.x = m_walkSpeed;
        m_physicsBody.setVelocity(m_velocity);
    } else if (m_physicsBody.hitWallRight()) {
        m_velocity.x = -m_walkSpeed;
        m_physicsBody.setVelocity(m_velocity);
    }

    if (m_position.x < 0.f) {
        m_position.x = 0.f;
        m_physicsBody.setPosition(m_position);
    } else if (m_position.x + m_tileSize > mapWidth) {
        m_position.x = mapWidth - m_tileSize;
        m_physicsBody.setPosition(m_position);
    }

    if (m_position.y > mapHeight + 100.f) {
        m_alive = false;
    }
}

void HammerBro::faceTowards(float targetX) {
    m_facingRight = targetX > m_position.x + m_tileSize * 0.5f;
}

bool HammerBro::takePendingThrow() {
    if (!m_pendingThrow || !m_alive || !m_active) {
        return false;
    }
    m_pendingThrow = false;
    return true;
}

sf::Vector2f HammerBro::hammerSpawnPoint() const {
    // Out of the raised hand: shoulder height, on the side he is looking at.
    const float offsetX = m_facingRight ? m_tileSize * 0.6f : -m_tileSize * 0.1f;
    return {m_position.x + offsetX, m_position.y - m_tileSize * 0.4f};
}

void HammerBro::render(sf::RenderTarget& target) const {
    if (!m_alive || !m_texture) {
        return;
    }

    sf::Sprite sprite(*m_texture);
    const float scale = m_tileSize / static_cast<float>(kSourceTileSize);
    sprite.setTextureRect(sf::IntRect(
        {m_animationFrame * kSourceTileSize, 0},
        {kSourceTileSize, kSourceHeight}
    ));

    // The sheet draws the Bro looking left, so facing right is a mirror image;
    // the flip moves its origin, hence the width added back to the position.
    if (m_facingRight) {
        sprite.setScale({-scale, scale});
        sprite.setPosition({m_position.x + kSourceTileSize * scale, m_position.y});
    } else {
        sprite.setScale({scale, scale});
        sprite.setPosition(m_position);
    }

    target.draw(sprite);
}

} // namespace entity
