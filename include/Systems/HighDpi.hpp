#pragma once
#include <SFML/Window/WindowHandle.hpp>

namespace Systems {
    /**
     * @brief Makes the window render at the screen's real pixel density.
     * @param handle Native handle of the window (sf::Window::getNativeHandle()).
     * @return The pixel density of the screen the window is on: 2 on a Retina
     *         display, 1 on an ordinary one (and always 1 outside macOS).
     *
     * Only does something on macOS, and only because SFML needs the help there:
     * it creates its OpenGL surface at one buffer pixel per window *point*, so on
     * a Retina screen the game is drawn at half the screen's resolution and macOS
     * blows the result up to fit - which is exactly what makes the pixel art look
     * smeared. Turning on the best-resolution surface gives us the full 2x buffer.
     *
     * SFML still believes the buffer is only as big as the window, so it computes
     * the OpenGL viewport from that. Callers must therefore scale their view's
     * viewport by the returned factor (see Config::letterboxViewport), otherwise
     * the game would only cover a quarter of the window.
     *
     * Windows and Linux need none of this, so the function reports 1 and the
     * scaling below becomes a no-op.
     */
    float enableHighDpi(sf::WindowHandle handle);
}
