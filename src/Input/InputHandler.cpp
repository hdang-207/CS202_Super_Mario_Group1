#include "Input/InputHandler.hpp"

#include <algorithm>
#include <utility>

namespace {

bool holdsAny(
    const std::set<sf::Keyboard::Scancode>& heldKeys,
    const std::vector<sf::Keyboard::Scancode>& bindings
) {
    return std::any_of(
        bindings.begin(),
        bindings.end(),
        [&heldKeys](sf::Keyboard::Scancode key) {
            return heldKeys.count(key) > 0;
        }
    );
}

} // namespace

InputHandler::InputHandler()
    : InputHandler(PlayerKeyBindings::campaign()) {}

InputHandler::InputHandler(PlayerKeyBindings bindings)
    : m_bindings(std::move(bindings)) {}

void InputHandler::update(const std::set<sf::Keyboard::Scancode>& heldKeys) {
    m_playerInput = PlayerInput{};

    const bool left = holdsAny(heldKeys, m_bindings.left);
    const bool right = holdsAny(heldKeys, m_bindings.right);
    const bool jump = holdsAny(heldKeys, m_bindings.jump);
    const bool crouch = holdsAny(heldKeys, m_bindings.crouch);
    const bool shoot = holdsAny(heldKeys, m_bindings.shoot);
    const bool bomb = holdsAny(heldKeys, m_bindings.bomb);

    if (left) {
        m_moveLeftCommand.execute(m_playerInput);
    }

    if (right) {
        m_moveRightCommand.execute(m_playerInput);
    }

    if (jump) {
        m_jumpCommand.execute(m_playerInput);
    }
    m_playerInput.jumpPressed = jump && !m_jumpHeldLastFrame;

    if (crouch) {
        m_crouchCommand.execute(m_playerInput);
    }

    if (shoot && !m_shootHeldLastFrame) {
        m_shootCommand.execute(m_playerInput);
    }

    if (bomb && !m_bombHeldLastFrame) {
        m_bombCommand.execute(m_playerInput);
    }

    m_jumpHeldLastFrame = jump;
    m_shootHeldLastFrame = shoot;
    m_bombHeldLastFrame = bomb;
}

void InputHandler::reset() {
    m_playerInput = PlayerInput{};
    m_jumpHeldLastFrame = false;
    m_shootHeldLastFrame = false;
    m_bombHeldLastFrame = false;
}

const PlayerInput& InputHandler::getPlayerInput() const noexcept {
    return m_playerInput;
}
