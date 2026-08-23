#include "Input/InputHandler.hpp"

void InputHandler::update(const std::set<sf::Keyboard::Scancode>& heldKeys) {
    reset();

    const auto holding = [&heldKeys](sf::Keyboard::Scancode key) {
        return heldKeys.count(key) > 0;
    };
    const bool left = holding(sf::Keyboard::Scancode::A)
                   || holding(sf::Keyboard::Scancode::Left);
    const bool right = holding(sf::Keyboard::Scancode::D)
                    || holding(sf::Keyboard::Scancode::Right);
    const bool jump = holding(sf::Keyboard::Scancode::Space)
                   || holding(sf::Keyboard::Scancode::W)
                   || holding(sf::Keyboard::Scancode::Up);

    if (left) {
        m_moveLeftCommand.execute(m_playerInput);
    }

    if (right) {
        m_moveRightCommand.execute(m_playerInput);
    }

    if (jump) {
        m_jumpCommand.execute(m_playerInput);
    }
}

void InputHandler::reset() {
    m_playerInput = PlayerInput{};
}

const PlayerInput& InputHandler::getPlayerInput() const noexcept {
    return m_playerInput;
}
