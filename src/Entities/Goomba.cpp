#include "Entities/Goomba.hpp"
#include "Physics/PhysicsSystem.hpp"
#include <SFML/Graphics/RenderWindow.hpp>
#include <SFML/Graphics/Sprite.hpp>
#include <cmath>

namespace entity {

namespace {
    constexpr float kGoombaFrameDuration = 0.3f;
    constexpr float kStompDisplayDuration = 0.5f;
    constexpr int kSourceTileSize = 16;
}

Goomba::Goomba(const sf::Vector2f& position, float tileSize, GoombaType type, float initialSpeed)
    : Character(position, {tileSize, tileSize}),
      m_type(type),
      m_tileSize(tileSize),
      m_walkSpeed(initialSpeed) {
    m_physicsBody.setVelocity({-m_walkSpeed, 0.f});
}

void Goomba::update(float deltaTime) {
    if (!m_alive || !m_active) {
        return;
    }

    if (m_stomped) {
        m_stompTimer += deltaTime;
        if (m_stompTimer >= kStompDisplayDuration) {
            m_alive = false;
        }
        return;
    }

    m_animElapsed += deltaTime;
    if (m_animElapsed >= kGoombaFrameDuration) {
        m_animElapsed -= kGoombaFrameDuration;
        m_animationFrame = (m_animationFrame + 1) % 2;
    }
}

void Goomba::update(float deltaTime, physics::PhysicsSystem& physicsSystem, const std::vector<physics::AABB>& solids, float mapWidth, float mapHeight) {
    if (!m_alive || !m_active) {
        return;
    }

    if (m_stomped) {
        m_stompTimer += deltaTime;
        if (m_stompTimer >= kStompDisplayDuration) {
            m_alive = false;
        }
        return;
    }

    m_animElapsed += deltaTime;
    if (m_animElapsed >= kGoombaFrameDuration) {
        m_animElapsed -= kGoombaFrameDuration;
        m_animationFrame = (m_animationFrame + 1) % 2;
    }

    // Kinematics and physics collision update
    physicsSystem.update(m_physicsBody, solids, deltaTime);
    m_position = m_physicsBody.getPosition();
    m_velocity = m_physicsBody.getVelocity();

    // Turn around at walls
    if (m_physicsBody.hitWallLeft()) {
        m_physicsBody.setVelocity({m_walkSpeed, m_velocity.y});
    } else if (m_physicsBody.hitWallRight()) {
        m_physicsBody.setVelocity({-m_walkSpeed, m_velocity.y});
    }

    // Map bounds
    if (m_position.x < 0.f) {
        m_position.x = 0.f;
        m_physicsBody.setPosition(m_position);
        m_physicsBody.setVelocity({m_walkSpeed, m_velocity.y});
    } else if (m_position.x + m_tileSize > mapWidth) {
        m_position.x = mapWidth - m_tileSize;
        m_physicsBody.setPosition(m_position);
        m_physicsBody.setVelocity({-m_walkSpeed, m_velocity.y});
    }

    // Fell in pit
    if (m_position.y > mapHeight) {
        m_alive = false;
    }
}

void Goomba::render(sf::RenderTarget& /*target*/) const {
    // Default render
}

void Goomba::renderWithTexture(sf::RenderWindow& window, const sf::Texture& texture) const {
    if (!m_alive && !m_stomped) {
        return;
    }

    sf::Sprite sprite(texture);
    const float scale = m_tileSize / static_cast<float>(kSourceTileSize);
    sprite.setScale({scale, scale});

    int frameX = m_stomped ? 2 : m_animationFrame;
    sprite.setTextureRect(sf::IntRect(
        {frameX * kSourceTileSize, 0},
        {kSourceTileSize, kSourceTileSize}
    ));
    sprite.setPosition(m_physicsBody.getPosition());
    window.draw(sprite);
}

void Goomba::stomp() {
    m_stomped = true;
    m_stompTimer = 0.f;
    m_physicsBody.setVelocity({0.f, 0.f});
}

void Goomba::defeat() {
    m_alive = false;
}

} // namespace entity
