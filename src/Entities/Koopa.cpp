#include "Entities/Koopa.hpp"
#include "Physics/PhysicsSystem.hpp"
#include <SFML/Graphics/RenderWindow.hpp>
#include <SFML/Graphics/Sprite.hpp>
#include <cmath>

namespace entity {

namespace {
    constexpr float kKoopaFrameDuration = 0.2f;
    constexpr int kSourceTileSize = 16;
}

Koopa::Koopa(const sf::Vector2f& position, float tileSize, KoopaKind kind, float initialSpeed)
    : Character(position, {tileSize, tileSize * 1.5f}),
      m_kind(kind),
      m_tileSize(tileSize),
      m_walkSpeed(initialSpeed) {
    m_physicsBody.setVelocity({-m_walkSpeed, 0.f});
}

void Koopa::adjustColliderForState() {
    sf::Vector2f currentPos = m_physicsBody.getPosition();
    if (m_state == KoopaState::Walking) {
        m_physicsBody = physics::PhysicsBody(currentPos, {m_tileSize, m_tileSize * 1.5f});
    } else {
        const float shellHeight = m_tileSize * 14.f / 16.f;
        m_physicsBody = physics::PhysicsBody(currentPos, {m_tileSize, shellHeight});
    }
}

void Koopa::setState(KoopaState state) {
    if (m_state == state) {
        return;
    }
    m_state = state;
    adjustColliderForState();
    if (m_state == KoopaState::ShellIdle) {
        m_physicsBody.setVelocity({0.f, m_physicsBody.getVelocity().y});
    }
}

void Koopa::update(float deltaTime) {
    if (!m_alive || !m_active) {
        return;
    }

    if (m_state == KoopaState::Walking) {
        m_animElapsed += deltaTime;
        if (m_animElapsed >= kKoopaFrameDuration) {
            m_animElapsed -= kKoopaFrameDuration;
            m_animationFrame = (m_animationFrame + 1) % 2;
        }
    }
}

void Koopa::update(float deltaTime, physics::PhysicsSystem& physicsSystem, const std::vector<physics::AABB>& solids, float mapWidth, float mapHeight) {
    if (!m_alive || !m_active) {
        return;
    }

    if (m_state == KoopaState::Walking) {
        m_animElapsed += deltaTime;
        if (m_animElapsed >= kKoopaFrameDuration) {
            m_animElapsed -= kKoopaFrameDuration;
            m_animationFrame = (m_animationFrame + 1) % 2;
        }
    }

    float currentMoveSpeed = (m_state == KoopaState::ShellMoving) ? m_shellSpeed : m_walkSpeed;
    if (m_state == KoopaState::ShellIdle) {
        m_physicsBody.setVelocity({0.f, m_physicsBody.getVelocity().y});
    }

    physicsSystem.update(m_physicsBody, solids, deltaTime);
    m_position = m_physicsBody.getPosition();
    m_velocity = m_physicsBody.getVelocity();

    if (m_physicsBody.hitWallLeft()) {
        m_physicsBody.setVelocity({currentMoveSpeed, m_velocity.y});
        m_facingRight = true;
    } else if (m_physicsBody.hitWallRight()) {
        m_physicsBody.setVelocity({-currentMoveSpeed, m_velocity.y});
        m_facingRight = false;
    }

    const float enemyHeight = isShell() ? (m_tileSize * 14.f / 16.f) : (m_tileSize * 1.5f);

    if (m_position.x < 0.f) {
        m_position.x = 0.f;
        m_physicsBody.setPosition(m_position);
        m_physicsBody.setVelocity({currentMoveSpeed, m_velocity.y});
        m_facingRight = true;
    } else if (m_position.x + m_tileSize > mapWidth) {
        m_position.x = mapWidth - m_tileSize;
        m_physicsBody.setPosition(m_position);
        m_physicsBody.setVelocity({-currentMoveSpeed, m_velocity.y});
        m_facingRight = false;
    }

    if (m_position.y > mapHeight) {
        m_alive = false;
    }
}

void Koopa::render(sf::RenderTarget& /*target*/) const {
    // Default render
}

void Koopa::renderWithTexture(sf::RenderWindow& window, const sf::Texture& walkingTexture, const sf::Texture& shellTexture) const {
    if (!m_alive) {
        return;
    }

    const float scale = m_tileSize / static_cast<float>(kSourceTileSize);

    if (isShell()) {
        sf::Sprite sprite(shellTexture);
        sprite.setScale({scale, scale});
        sprite.setPosition(m_physicsBody.getPosition());
        window.draw(sprite);
    } else {
        sf::Sprite sprite(walkingTexture);
        sprite.setScale({scale, scale});
        const int sourceHeight = static_cast<int>(kSourceTileSize * 1.5f);
        sprite.setTextureRect(sf::IntRect(
            {m_animationFrame * kSourceTileSize, 0},
            {kSourceTileSize, sourceHeight}
        ));
        sprite.setPosition(m_physicsBody.getPosition());
        window.draw(sprite);
    }
}

void Koopa::stomp(bool playerFacingRight) {
    if (m_state == KoopaState::Walking) {
        const float shellHeight = m_tileSize * 14.f / 16.f;
        sf::Vector2f pos = m_physicsBody.getPosition();
        pos.y += (m_tileSize * 1.5f) - shellHeight;
        m_physicsBody.setPosition(pos);
        setState(KoopaState::ShellIdle);
    } else if (m_state == KoopaState::ShellMoving) {
        setState(KoopaState::ShellIdle);
    } else if (m_state == KoopaState::ShellIdle) {
        kick(playerFacingRight);
    }
}

void Koopa::kick(bool directionRight) {
    m_state = KoopaState::ShellMoving;
    m_facingRight = directionRight;
    m_physicsBody.setVelocity({directionRight ? m_shellSpeed : -m_shellSpeed, m_physicsBody.getVelocity().y});
}

} // namespace entity
