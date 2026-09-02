#pragma once

#include <SFML/Window/Keyboard.hpp>
#include <vector>

/**
 * @brief Maps each player action to zero or more keyboard scancodes.
 *
 * Empty action bindings are valid, allowing a profile to disable actions such
 * as shooting and bombs without adding mode-specific logic to InputHandler.
 */
struct PlayerKeyBindings {
    using Scancode = sf::Keyboard::Scancode;

    std::vector<Scancode> left;
    std::vector<Scancode> right;
    std::vector<Scancode> jump;
    std::vector<Scancode> crouch;
    std::vector<Scancode> shoot;
    std::vector<Scancode> bomb;

    [[nodiscard]] static PlayerKeyBindings campaign() {
        return {
            {Scancode::A, Scancode::Left},
            {Scancode::D, Scancode::Right},
            {Scancode::W, Scancode::Up, Scancode::Space},
            {Scancode::S, Scancode::Down},
            {Scancode::X},
            {Scancode::C}
        };
    }

    [[nodiscard]] static PlayerKeyBindings duelPlayerOne() {
        return {
            {Scancode::A},
            {Scancode::D},
            {Scancode::W},
            {Scancode::S},
            {Scancode::F},
            {Scancode::G}
        };
    }

    [[nodiscard]] static PlayerKeyBindings duelPlayerTwo() {
        return {
            {Scancode::Left},
            {Scancode::Right},
            {Scancode::Up},
            {Scancode::Down},
            {Scancode::J},
            {Scancode::K}
        };
    }
};
