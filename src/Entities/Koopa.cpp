#include "Entities/Koopa.hpp"
#include "Physics/PhysicsSystem.hpp"
#include <SFML/Graphics/RenderTarget.hpp>
#include <SFML/Graphics/RenderWindow.hpp>
#include <SFML/Graphics/Sprite.hpp>

namespace entity {

namespace {
    constexpr float kKoopaFrameDuration = 0.25f;
    constexpr int kSourceTileSize = 16;
}

Koopa::Koopa(const sf::Vector2f& position, float tileSize, const sf::Texture* walkingTexture, const sf::Texture* shellTexture, KoopaKind kind, float initialSpeed)
    : Character(position, {tileSize, tileSize * 1.5f}),
      m_walkingTexture(walkingTexture),
      m_shellTexture(shellTexture),
      m_kind(kind),
      m_tileSize(tileSize),
      m_walkSpeed(initialSpeed) {
    m_active = true;
    m_velocity = {-m_walkSpeed, 0.f};
    m_physicsBody.setVelocity(m_velocity);
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
        m_position.x += m_velocity.x * deltaTime;
    } else if (m_state == KoopaState::ShellMoving) {
        m_position.x += m_velocity.x * deltaTime;
    }
    m_physicsBody.setPosition(m_position);
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

    physicsSystem.update(m_physicsBody, solids, deltaTime);
    m_position = m_physicsBody.getPosition();
    m_velocity = m_physicsBody.getVelocity();

    float currentMoveSpeed = (m_state == KoopaState::ShellMoving) ? m_shellSpeed : m_walkSpeed;

    if (m_physicsBody.hitWallLeft()) {
        m_velocity.x = currentMoveSpeed;
        m_physicsBody.setVelocity(m_velocity);
        m_facingRight = true;
    } else if (m_physicsBody.hitWallRight()) {
        m_velocity.x = -currentMoveSpeed;
        m_physicsBody.setVelocity(m_velocity);
        m_facingRight = false;
    }

    if (m_position.x < 0.f) {
        m_position.x = 0.f;
        m_velocity.x = currentMoveSpeed;
        m_physicsBody.setPosition(m_position);
        m_physicsBody.setVelocity(m_velocity);
        m_facingRight = true;
    } else if (m_position.x + m_tileSize > mapWidth) {
        m_position.x = mapWidth - m_tileSize;
        m_velocity.x = -currentMoveSpeed;
        m_physicsBody.setPosition(m_position);
        m_physicsBody.setVelocity(m_velocity);
        m_facingRight = false;
    }

    if (m_position.y > mapHeight + 100.f) {
        m_alive = false;
    }
}

void Koopa::render(sf::RenderTarget& target) const {
    if (!m_alive) {
        return;
    }

    const float scale = m_tileSize / static_cast<float>(kSourceTileSize);

    if (isShell()) {
        if (!m_shellTexture) return;
        sf::Sprite sprite(*m_shellTexture);
        sprite.setScale({scale, scale});
        sprite.setPosition(m_position);
        target.draw(sprite);
    } else {
        if (!m_walkingTexture) return;
        sf::Sprite sprite(*m_walkingTexture);
        const float scaleX = m_facingRight ? -scale : scale;
        sprite.setScale({scaleX, scale});
        
        // Both supplied Koopa strips contain two 16x24 walking frames.  The
        // previous green asset was cropped to one 16x23 frame, which made both
        // Green Koopas and Paratroopas appear frozen even though update() was
        // advancing m_animationFrame.
        constexpr int sourceHeight = 24;
        const int frame = m_animationFrame;

        sprite.setTextureRect(sf::IntRect(
            {frame * kSourceTileSize, 0},
            {kSourceTileSize, sourceHeight}
        ));
        sprite.setPosition({m_facingRight ? m_position.x + m_tileSize : m_position.x,
                            m_position.y});
        target.draw(sprite);
    }
}

void Koopa::renderWithTexture(sf::RenderWindow& window, const sf::Texture& walkingTexture, const sf::Texture& shellTexture) const {
    if (!m_alive) {
        return;
    }

    const float scale = m_tileSize / static_cast<float>(kSourceTileSize);

    if (isShell()) {
        sf::Sprite sprite(shellTexture);
        sprite.setScale({scale, scale});
        sprite.setPosition(m_position);
        window.draw(sprite);
    } else {
        sf::Sprite sprite(walkingTexture);
        const float scaleX = m_facingRight ? -scale : scale;
        sprite.setScale({scaleX, scale});
        const int sourceHeight = static_cast<int>(kSourceTileSize * 1.5f);
        sprite.setTextureRect(sf::IntRect(
            {m_animationFrame * kSourceTileSize, 0},
            {kSourceTileSize, sourceHeight}
        ));
        sprite.setPosition({m_facingRight ? m_position.x + m_tileSize : m_position.x, m_position.y});
        window.draw(sprite);
    }
}

void Koopa::stomp() {
    if (m_state == KoopaState::Walking) {
        m_state = KoopaState::ShellIdle;
        m_velocity = {0.f, 0.f};
        m_physicsBody.setVelocity(m_velocity);
        m_physicsBody.setCollider({m_tileSize, m_tileSize * 14.f / 16.f});
    } else if (m_state == KoopaState::ShellIdle) {
        kick(true);
    } else if (m_state == KoopaState::ShellMoving) {
        m_state = KoopaState::ShellIdle;
        m_velocity = {0.f, 0.f};
        m_physicsBody.setVelocity(m_velocity);
    }
}

void Koopa::kick(bool directionRight, float kickSpeed) {
    m_state = KoopaState::ShellMoving;
    m_shellSpeed = kickSpeed;
    m_velocity = {directionRight ? m_shellSpeed : -m_shellSpeed, 0.f};
    m_physicsBody.setVelocity(m_velocity);
    m_facingRight = directionRight;
}

void Koopa::defeat() {
    m_alive = false;
}

} // namespace entity
