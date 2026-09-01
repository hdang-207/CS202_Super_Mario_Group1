#include "Items/Mushroom.hpp"

#include "Physics/PhysicsBody.hpp"
#include "Physics/PhysicsSystem.hpp"

#include <SFML/Graphics/RenderWindow.hpp>
#include <SFML/Graphics/Sprite.hpp>
#include <algorithm>

namespace {
constexpr float kMushroomRiseDuration = 0.45f;
constexpr float kMushroomSpeed = 120.f;
}

namespace items {

Mushroom::Mushroom(sf::Vector2f blockPosition, MushroomKind kind, const sf::Texture* texture, float scale)
    : Item(blockPosition),
      m_texture(texture),
      m_scale(scale),
      m_blockPosition(blockPosition),
      m_physicsBody(blockPosition, {16.f * scale, 16.f * scale}),
      m_kind(kind),
      m_tileSize(16.f * scale) {}

void Mushroom::update(sf::Time dt) {
    update(dt.asSeconds());
}

void Mushroom::update(float deltaTime) {
    if (!m_alive) {
        return;
    }

    if (m_state == MushroomState::Emerging) {
        m_elapsed = std::min(m_elapsed + deltaTime, kMushroomRiseDuration);
        const float progress = m_elapsed / kMushroomRiseDuration;
        m_position = {m_blockPosition.x,
                      m_blockPosition.y - m_tileSize * progress};

        if (m_elapsed >= kMushroomRiseDuration) {
            m_state = MushroomState::Moving;
            m_velocity.x = kMushroomSpeed;
            m_physicsBody.setPosition(m_position);
            m_physicsBody.setVelocity(m_velocity);
        }
        return;
    }

    m_position.x += m_velocity.x * deltaTime;
}

void Mushroom::update(float deltaTime, float tileSize,
                       physics::PhysicsSystem& physicsSystem,
                       const std::vector<physics::AABB>& solids) {
    m_tileSize = tileSize;

    if (m_state == MushroomState::Emerging) {
        m_elapsed = std::min(m_elapsed + deltaTime, kMushroomRiseDuration);
        const float progress = m_elapsed / kMushroomRiseDuration;
        m_position = {m_blockPosition.x,
                      m_blockPosition.y - tileSize * progress};

        if (m_elapsed >= kMushroomRiseDuration) {
            m_state = MushroomState::Moving;
            m_velocity = {kMushroomSpeed, 0.f};
            m_physicsBody.setCollider({tileSize, tileSize});
            m_physicsBody.setPosition(m_position);
            m_physicsBody.setVelocity(m_velocity);
        }
        return;
    }

    physicsSystem.update(m_physicsBody, solids, deltaTime);

    m_position = m_physicsBody.getPosition();
    m_velocity = m_physicsBody.getVelocity();

    if (m_physicsBody.hitWallLeft()) {
        m_velocity.x = kMushroomSpeed;
        m_physicsBody.setVelocity(m_velocity);
    } else if (m_physicsBody.hitWallRight()) {
        m_velocity.x = -kMushroomSpeed;
        m_physicsBody.setVelocity(m_velocity);
    }
}

void Mushroom::reverseDirection() {
    if (!m_alive || m_state != MushroomState::Moving) {
        return;
    }

    m_velocity.x = m_velocity.x > 0.f ? -kMushroomSpeed : kMushroomSpeed;
    m_physicsBody.setVelocity(m_velocity);
}

void Mushroom::render(sf::RenderWindow& window) const {
    if (!m_alive || !m_texture) {
        return;
    }
    render(window, *m_texture, m_scale);
}

void Mushroom::render(sf::RenderWindow& window, const sf::Texture& texture,
                      float scale) const {
    if (!m_alive) {
        return;
    }
    sf::Sprite sprite(texture);
    sprite.setScale({scale, scale});
    sprite.setPosition(m_position);
    window.draw(sprite);
}

sf::FloatRect Mushroom::getBounds() const {
    return sf::FloatRect(m_position, {16.f * m_scale, 16.f * m_scale});
}

bool Mushroom::isCollectible() const noexcept {
    return m_state == MushroomState::Moving && m_alive;
}

bool Mushroom::hasFallenOut(float worldHeight) const noexcept {
    return m_position.y > worldHeight;
}

MushroomKind Mushroom::getKind() const noexcept { return m_kind; }

} // namespace items
