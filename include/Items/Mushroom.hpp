#pragma once

#include "Items/Item.hpp"
#include "Physics/AABB.hpp"
#include "Physics/PhysicsBody.hpp"

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
             MushroomKind kind = MushroomKind::Super,
             const sf::Texture* texture = nullptr,
             float scale = 3.f);

    void update(sf::Time dt) override;
    void update(float deltaTime);
    void update(float deltaTime, float tileSize,
                physics::PhysicsSystem& physicsSystem,
                const std::vector<physics::AABB>& solids);
    void render(sf::RenderWindow& window) const override;
    void render(sf::RenderWindow& window, const sf::Texture& texture,
                float scale) const override;

    [[nodiscard]] sf::FloatRect getBounds() const override;
    [[nodiscard]] bool isCollectible() const noexcept override;
    [[nodiscard]] bool hasFallenOut(float worldHeight) const noexcept;
    [[nodiscard]] MushroomKind getKind() const noexcept;

private:
    const sf::Texture* m_texture{nullptr};
    float m_scale{3.f};
    sf::Vector2f m_blockPosition;
    physics::PhysicsBody m_physicsBody;
    MushroomState m_state{MushroomState::Emerging};
    float m_elapsed{0.f};
    MushroomKind m_kind{MushroomKind::Super};
    float m_tileSize{48.f};
};

} // namespace items
