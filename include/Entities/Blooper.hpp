#pragma once

#include "Entities/Character.hpp"
#include <SFML/Graphics/Texture.hpp>

namespace entity {

/**
 * @brief Underwater squid that pulses toward the player and ignores terrain.
 *
 * SMB aquatic enemies are allowed to cross coral and rock formations.  The
 * swim bounds only keep the Blooper inside the World 2-2 water room and away
 * from the ocean floor.
 */
class Blooper final : public Character {
public:
    Blooper(const sf::Vector2f& position, float tileSize,
            const sf::FloatRect& swimBounds,
            const sf::Texture* texture = nullptr);

    void update(float deltaTime) override;
    void render(sf::RenderTarget& target) const override;

    void setTarget(const sf::Vector2f& target) noexcept { m_target = target; }

private:
    const sf::Texture* m_texture{nullptr};
    sf::FloatRect m_swimBounds;
    sf::Vector2f m_target;
    float m_tileSize{48.f};
    float m_movementElapsed{0.f};
    float m_animationElapsed{0.f};
    int m_animationFrame{0};
};

} // namespace entity
