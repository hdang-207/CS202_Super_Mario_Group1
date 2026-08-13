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

Mushroom::Mushroom(sf::Vector2f blockPosition, MushroomKind kind)
    : m_blockPosition(blockPosition), m_position(blockPosition), m_kind(kind) {}

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
            m_velocity.x = kMushroomSpeed;
        }
        return;
    }

    physics::PhysicsBody body(m_position, {tileSize, tileSize});
    body.setVelocity(m_velocity);
    physicsSystem.update(body, solids, deltaTime);

    m_position = body.getPosition();
    m_velocity = body.getVelocity();

    if (body.hitWallLeft()) {
        m_velocity.x = kMushroomSpeed;
    } else if (body.hitWallRight()) {
        m_velocity.x = -kMushroomSpeed;
    }
}

void Mushroom::render(sf::RenderWindow& window, const sf::Texture& texture,
                      float scale) const {
    sf::Sprite sprite(texture);
    sprite.setScale({scale, scale});
    sprite.setPosition(m_position);
    window.draw(sprite);
}

sf::FloatRect Mushroom::getBounds() const {
    return {m_position, {m_tileSize, m_tileSize}};
}

bool Mushroom::isCollectible() const noexcept {
    return m_state == MushroomState::Moving;
}

bool Mushroom::hasFallenOut(float worldHeight) const noexcept {
    return m_position.y > worldHeight;
}

MushroomKind Mushroom::getKind() const noexcept {
    return m_kind;
}

} // namespace items
