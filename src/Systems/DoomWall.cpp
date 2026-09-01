#include "Systems/DoomWall.hpp"
#include "Core/Config.hpp"
#include <cmath>

namespace Systems {

DoomWall::DoomWall(AssetManager& assets) 
    : m_wallX(-200.f), m_speed(215.f), m_animTime(0.f),
      m_bodySprite(assets.getTexture("DoomFire2")),
      m_edgeSprite(assets.getTexture("DoomFire1")) {
    
    // Set up textures
    auto& texBody = assets.getTexture("DoomFire2");
    const_cast<sf::Texture&>(texBody).setRepeated(true);

    auto& texEdge = assets.getTexture("DoomFire1");
    const_cast<sf::Texture&>(texEdge).setRepeated(true);
}

void DoomWall::update(sf::Time dt, float cameraX) {
    m_animTime += dt.asSeconds();
    
    // Move the wall
    m_wallX += m_speed * dt.asSeconds();

    // Prevent the wall from getting too far behind or too far ahead of the camera
    // If the player rushes, the wall catches up slightly but doesn't warp.
    // However, if the player stands still, the wall WILL catch them.
    // The cameraX is the left edge of the view. 
    // If the wall is extremely far behind the camera, we can teleport it closer so the player still feels pressure.
    if (m_wallX < cameraX - Config::kViewWidth) {
        m_wallX = cameraX - Config::kViewWidth;
    }
}

bool DoomWall::checkCollision(const sf::FloatRect& playerBounds) const {
    // The player dies if their left edge is behind the fire wall's edge.
    // The fire wall edge is at m_wallX.
    if (playerBounds.position.x + playerBounds.size.x / 2.f <= m_wallX) {
        return true;
    }
    return false;
}

void DoomWall::render(sf::RenderWindow& window) const {
    const float frameWidth = 32.f;
    int frameIndex = static_cast<int>(m_animTime * 10.f) % 4; // 10 fps

    float edgeTexWidth = m_edgeSprite.getTexture().getSize().x;
    float bodyTexWidth = m_bodySprite.getTexture().getSize().x;
    
    float wEdge = std::min(frameWidth, edgeTexWidth - frameIndex * frameWidth);
    float wBody = std::min(frameWidth, bodyTexWidth - frameIndex * frameWidth);

    // Render 4 overlapping layers to make it denser
    for (int i = 0; i < 4; ++i) {
        float xOffset = i * -12.f; // Shift each layer left
        float yOffset = i * 8.f;   // Shift each layer down slightly
        
        // Render the edge
        sf::IntRect edgeRect({static_cast<int>(frameIndex * frameWidth), 0}, 
                             {static_cast<int>(wEdge), static_cast<int>(Config::kViewHeight + 40)});
        m_edgeSprite.setTextureRect(edgeRect);
        m_edgeSprite.setPosition({m_wallX - wEdge + xOffset, -20.f + yOffset});
        window.draw(m_edgeSprite);

        // Render the body
        sf::IntRect bodyRect({static_cast<int>(frameIndex * frameWidth), 0}, 
                             {static_cast<int>(wBody), static_cast<int>(Config::kViewHeight + 40)});
        m_bodySprite.setTextureRect(bodyRect);
        
        float cameraLeft = window.getView().getCenter().x - window.getView().getSize().x / 2.f;
        float x = m_wallX - wEdge + xOffset;
        while (x > cameraLeft - wBody) {
            x -= wBody;
            m_bodySprite.setPosition({x, -20.f + yOffset});
            window.draw(m_bodySprite);
        }
    }
}

} // namespace Systems
