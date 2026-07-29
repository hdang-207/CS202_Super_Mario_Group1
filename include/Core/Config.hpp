#pragma once
#include <SFML/Graphics/Rect.hpp>
#include <SFML/System/Vector2.hpp>

/**
 * @namespace Config
 * @brief Single source of truth for how big the game is drawn on screen.
 *
 * The artwork is authored at 16x16 pixels per tile, exactly like the original
 * game. Everything else is derived from that tile size so there is only one
 * number to change when the framing needs tuning:
 *
 *   kZoom       how many screen pixels one artwork pixel becomes (keep it a
 *               whole number, otherwise tiles get blurry/uneven edges)
 *   kViewTiles* how much of the level fits on screen, counted in tiles
 *
 * With the defaults below the camera shows 24x15 tiles - the level is 15 rows
 * tall, so the ground sits exactly on the bottom edge of the screen and the
 * sky fills the top, the same framing as the original Super Mario Bros.
 */
namespace Config {
    /// Size of one tile in the atlas artwork, in pixels.
    inline constexpr float kSourceTileSize = 16.f;

    /// Whole-number zoom applied to the artwork (16px tile -> 48px on screen).
    inline constexpr float kZoom = 3.f;

    /// Size of one tile in world/screen pixels once zoomed.
    inline constexpr float kTileSize = kSourceTileSize * kZoom;

    /// How many tiles the camera shows horizontally / vertically.
    inline constexpr int kViewTilesX = 24;
    inline constexpr int kViewTilesY = 15;

    /// Size of the visible game area in world pixels (1152 x 720).
    inline constexpr float kViewWidth = kViewTilesX * kTileSize;
    inline constexpr float kViewHeight = kViewTilesY * kTileSize;

    /// Default window size: one screen pixel per world pixel, so nothing is scaled.
    inline constexpr unsigned kWindowWidth = static_cast<unsigned>(kViewWidth);
    inline constexpr unsigned kWindowHeight = static_cast<unsigned>(kViewHeight);

    /**
     * @brief Viewport that keeps the game's aspect ratio inside any window size.
     * @param windowSize Current window size in pixels.
     * @return Rectangle in 0..1 window coordinates, centred, with black bars on
     *         the two sides that have spare room.
     *
     * Without this the picture would stretch when the player resizes the window
     * or goes fullscreen; with it the game is only ever scaled uniformly.
     */
    inline sf::FloatRect letterboxViewport(sf::Vector2u windowSize) {
        if (windowSize.x == 0 || windowSize.y == 0) {
            return sf::FloatRect({0.f, 0.f}, {1.f, 1.f});
        }

        float windowRatio = static_cast<float>(windowSize.x) / static_cast<float>(windowSize.y);
        float viewRatio = kViewWidth / kViewHeight;

        sf::Vector2f position(0.f, 0.f);
        sf::Vector2f size(1.f, 1.f);

        if (windowRatio > viewRatio) {
            // Window is wider than the game: bars on the left and right.
            size.x = viewRatio / windowRatio;
            position.x = (1.f - size.x) / 2.f;
        } else {
            // Window is taller than the game: bars on the top and bottom.
            size.y = windowRatio / viewRatio;
            position.y = (1.f - size.y) / 2.f;
        }

        return sf::FloatRect(position, size);
    }
}
