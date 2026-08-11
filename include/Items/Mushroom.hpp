#pragma once

#include "Items/Item.hpp"
#include "Physics/AABB.hpp"

#include <SFML/Graphics/Texture.hpp>
#include <SFML/System/Vector2.hpp>
#include <vector>

namespace sf {
class RenderWindow;
}

namespace physics {
class PhysicsSystem;
}

namespace items {

enum class MushroomState { Emerging, Moving };
enum class MushroomKind { Super, OneUp };

class Mushroom final : public Item {
public:
    Mushroom(sf::Vector2f blockPosition,
             MushroomKind kind = MushroomKind::Super);

    void update(float deltaTime, float tileSize,
                physics::PhysicsSystem& physicsSystem,
                const std::vector<physics::AABB>& solids);
    void render(sf::RenderWindow& window, const sf::Texture& texture,
                float scale) const override;

    [[nodiscard]] sf::FloatRect getBounds() const override;
    [[nodiscard]] bool isCollectible() const noexcept override;
    [[nodiscard]] bool hasFallenOut(float worldHeight) const noexcept;
    [[nodiscard]] MushroomKind getKind() const noexcept;

private:
    sf::Vector2f m_blockPosition;
    sf::Vector2f m_position;
    sf::Vector2f m_velocity{0.f, 0.f};
    MushroomState m_state{MushroomState::Emerging};
    float m_elapsed{0.f};
    MushroomKind m_kind;
    float m_tileSize{0.f};
};

} // namespace items
