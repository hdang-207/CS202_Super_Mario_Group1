#pragma once
#include <SFML/Graphics/Rect.hpp>
#include <SFML/System/Vector2.hpp>
#include <algorithm>
#include <cmath>

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
    /// This project currently ships World 1-1 and World 1-2 only.
    inline constexpr int kFinalLevel = 2;

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
     * @brief Viewport that draws the game centred, unstretched and pixel-perfect.
     * @param windowSize Window size as SFML reports it.
     * @param dpiScale Buffer pixels per window unit, from Systems::enableHighDpi().
     * @return Rectangle to hand to sf::View::setViewport().
     *
     * Two things would otherwise make the artwork look smeared:
     *
     *  - a window whose shape does not match the game's would stretch the picture,
     *    so the leftover room becomes black bars instead;
     *  - a fractional zoom (say 2.37x) would make some art pixels 2 screen pixels
     *    wide and their neighbours 3, which reads as blur. The zoom is therefore
     *    rounded down to a whole number whenever the window is big enough to fit
     *    the game at least once. Below that there is nothing to round down to, so
     *    the game is simply fitted as well as it can be.
     *
     * The result is measured against the real buffer but expressed in the units
     * SFML multiplies by, hence the dpiScale factor at the end - see the note in
     * Systems::enableHighDpi(). It stays a plain 0..1 rectangle when dpiScale is 1.
     */
    inline sf::FloatRect letterboxViewport(sf::Vector2u windowSize, float dpiScale = 1.f) {
        if (windowSize.x == 0 || windowSize.y == 0 || dpiScale <= 0.f) {
            return sf::FloatRect({0.f, 0.f}, {1.f, 1.f});
        }

        float windowWidth = static_cast<float>(windowSize.x);
        float windowHeight = static_cast<float>(windowSize.y);

        // Biggest zoom that still fits, rounded down to a whole number.
        float zoom = std::min(windowWidth * dpiScale / kViewWidth,
                              windowHeight * dpiScale / kViewHeight);
        if (zoom >= 1.f) {
            zoom = std::floor(zoom);
        }

        // SFML multiplies the viewport by the window size rather than by the
        // buffer size, so on a high-density screen these deliberately go past 1.
        sf::Vector2f size(kViewWidth * zoom / windowWidth, kViewHeight * zoom / windowHeight);

        // Centring is not symmetric between the two axes: SFML measures the
        // viewport's top edge downwards but hands OpenGL an origin measured
        // upwards, flipping it with the window height. Mirroring the horizontal
        // term would push the picture a whole window height off the bottom.
        sf::Vector2f position((dpiScale - size.x) / 2.f,
                              1.f - dpiScale / 2.f - size.y / 2.f);

        return sf::FloatRect(position, size);
    }
}
