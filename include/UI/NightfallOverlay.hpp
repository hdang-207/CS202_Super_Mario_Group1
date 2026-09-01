#pragma once

#include <SFML/Graphics.hpp>
#include <optional>

namespace UI {

/**
 * @class NightfallOverlay
 * @brief Renders a darkness overlay with a radial gradient light circle around a focal point.
 *
 * Single Responsibility: This class is solely responsible for the visual darkness effect.
 * It does not know about players, game state, or any game logic.
 *
 * Technique: Uses an sf::RenderTexture filled with darkness, then punches a radial
 * gradient hole (TriangleFan with BlendNone) at the light source position,
 * then composites the result onto the main window with BlendMultiply.
 */
class NightfallOverlay {
public:
    /**
     * @brief Constructs the overlay with configurable light parameters.
     * @param viewWidth  Width of the game view in pixels.
     * @param viewHeight Height of the game view in pixels.
     * @param lightRadius Radius of the visible light circle in world pixels.
     * @param segments   Number of triangle segments for the gradient circle (smoothness).
     */
    NightfallOverlay(float viewWidth, float viewHeight,
                     float lightRadius = 160.f, int segments = 64);

    /**
     * @brief Draws the darkness overlay onto the render target.
     * @param target      The render target (window).
     * @param lightCenter The world-space position of the light source (e.g. player center).
     * @param gameView    The current game camera view (for coordinate mapping).
     */
    void draw(sf::RenderTarget& target, sf::Vector2f lightCenter,
              const sf::View& gameView);

    void setLightRadius(float radius) { m_lightRadius = radius; }
    [[nodiscard]] float getLightRadius() const { return m_lightRadius; }

private:
    float m_viewWidth;
    float m_viewHeight;
    float m_lightRadius;
    int m_segments;
    std::optional<sf::RenderTexture> m_maskTexture;

    void ensureMaskTexture();
};

} // namespace UI
