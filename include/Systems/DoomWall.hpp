#pragma once

#include <SFML/Graphics.hpp>
#include "Systems/AssetManager.hpp"

namespace Systems {

/**
 * @class DoomWall
 * @brief Represents the fire wall in Inferno Mode that chases the player.
 */
class DoomWall {
public:
    DoomWall(AssetManager& assets);

    void update(sf::Time dt, float cameraX);
    void render(sf::RenderWindow& window) const;

    bool checkCollision(const sf::FloatRect& playerBounds) const;

    float getX() const { return m_wallX; }
    void setX(float x) { m_wallX = x; }

private:
    float m_wallX;
    float m_speed;

    mutable sf::Sprite m_bodySprite;
    mutable sf::Sprite m_edgeSprite;

    // We keep track of animation time to scroll the texture
    float m_animTime;
};

} // namespace Systems
