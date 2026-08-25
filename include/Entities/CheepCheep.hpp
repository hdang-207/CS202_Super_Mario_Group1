#pragma once

#include "Entities/Character.hpp"
#include <SFML/Graphics/Texture.hpp>

namespace entity {

/** @brief Left-swimming World 2-2 fish with a two-frame tail animation. */
class CheepCheep final : public Character {
public:
    CheepCheep(const sf::Vector2f& position, float tileSize,
               const sf::FloatRect& swimBounds,
               const sf::Texture* texture = nullptr,
               float swimSpeed = 96.f);

    void update(float deltaTime) override;
    void render(sf::RenderTarget& target) const override;

private:
    const sf::Texture* m_texture{nullptr};
    sf::FloatRect m_swimBounds;
    float m_tileSize{48.f};
    float m_originY{0.f};
    float m_swimSpeed{96.f};
    float m_movementElapsed{0.f};
    float m_animationElapsed{0.f};
    int m_animationFrame{0};
};

} // namespace entity
