#pragma once

#include "Entities/Character.hpp"
#include <SFML/Graphics/Texture.hpp>

namespace entity {

/** @brief Red World 2-3 Cheep-Cheep that leaps in a ballistic arc. */
class FlyingCheepCheep final : public Character {
public:
    FlyingCheepCheep(const sf::Vector2f& position, const sf::Vector2f& velocity,
                     float tileSize, const sf::FloatRect& flightBounds,
                     const sf::Texture* texture = nullptr);

    void update(float deltaTime) override;
    void render(sf::RenderTarget& target) const override;

private:
    const sf::Texture* m_texture{nullptr};
    sf::FloatRect m_flightBounds;
    float m_tileSize{48.f};
    float m_animationElapsed{0.f};
    int m_animationFrame{0};
};

} // namespace entity
