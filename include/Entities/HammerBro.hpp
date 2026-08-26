#pragma once

#include "Entities/Character.hpp"
#include <SFML/Graphics/Texture.hpp>
#include <vector>

namespace physics {
class PhysicsSystem;
struct AABB;
}

namespace entity {

/**
 * @class HammerBro
 * @brief The pair of throwers guarding the brick platforms in World 3-1.
 *
 * A Hammer Bro never chases: it shuffles a tile to either side of where the
 * map put it and keeps turning to face the avatar, so the hammers always come
 * at whoever is in front of it. Throwing itself is left to the level - the Bro
 * only raises the flag through takePendingThrow(), the same way the stage
 * spawns its own Cheep-Cheeps, because entities cannot reach the manager that
 * owns them.
 *
 * Everything else it inherits: it is stomped, burnt and starred exactly like a
 * Goomba because the collision code only asks for an Entity.
 */
class HammerBro : public Character {
public:
    HammerBro(const sf::Vector2f& position, float tileSize,
              const sf::Texture* texture = nullptr);
    virtual ~HammerBro() override = default;

    void update(float deltaTime) override;
    void update(float deltaTime, physics::PhysicsSystem& physicsSystem,
                const std::vector<physics::AABB>& solids, float mapWidth, float mapHeight);

    void render(sf::RenderTarget& target) const override;

    /// @brief Turns to face @p targetX, which is where the avatar stands.
    void faceTowards(float targetX);

    /// @brief True once per throw; clears itself so one call spawns one hammer.
    bool takePendingThrow();

    /// @brief Where the next hammer leaves its hand, in world pixels.
    [[nodiscard]] sf::Vector2f hammerSpawnPoint() const;

    [[nodiscard]] bool isFacingRight() const noexcept { return m_facingRight; }
    [[nodiscard]] float getTileSize() const noexcept { return m_tileSize; }

private:
    const sf::Texture* m_texture{nullptr};
    float m_tileSize{48.f};
    float m_originX{0.f};       ///< Centre of the shuffle, taken from the map marker.
    float m_walkSpeed{45.f};
    float m_animElapsed{0.f};
    int m_animationFrame{0};
    float m_throwTimer{0.f};
    bool m_pendingThrow{false};
    bool m_facingRight{false};
};

} // namespace entity
