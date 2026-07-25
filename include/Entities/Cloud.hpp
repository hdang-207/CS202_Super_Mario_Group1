#pragma once
#include <SFML/Graphics.hpp>
#include <vector>

/**
 * @class Cloud
 * @brief Decorative background cloud rendered as a retro pixel-art silhouette.
 *
 * Built from a hardcoded pixel pattern (grid of blocks) rather than a texture,
 * so it can be dropped into the map background before art assets exist.
 */
class Cloud {
public:
    /**
     * @brief Builds a cloud shape anchored at the given top-left position.
     * @param position Top-left corner of the cloud in world/screen space.
     * @param pixelSize Size (in screen pixels) of a single "pixel" block, controls overall cloud size.
     */
    explicit Cloud(sf::Vector2f position, float pixelSize = 8.f);

    /**
     * @brief Draws the cloud's blocks onto the window.
     * @param window Window to render onto.
     */
    void render(sf::RenderWindow& window);

private:
    std::vector<sf::RectangleShape> blocks; ///< Pixel blocks making up the cloud silhouette.

    /// @brief Constructs the blocks from the pixel pattern, positioned and scaled.
    void build(sf::Vector2f position, float pixelSize);
};
