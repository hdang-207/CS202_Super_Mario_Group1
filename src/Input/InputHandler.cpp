#include "Input/InputHandler.hpp"

void InputHandler::update(const std::set<sf::Keyboard::Key>& heldKeys) {
    reset();

    const auto holding = [&heldKeys](sf::Keyboard::Key key) {
        return heldKeys.count(key) > 0;
    };
    const bool left = holding(sf::Keyboard::Key::A)
                   || holding(sf::Keyboard::Key::Left);
    const bool right = holding(sf::Keyboard::Key::D)
                    || holding(sf::Keyboard::Key::Right);
    const bool jump = holding(sf::Keyboard::Key::Space)
                   || holding(sf::Keyboard::Key::W)
                   || holding(sf::Keyboard::Key::Up);
    const bool crouch = holding(sf::Keyboard::Key::S)
                     || holding(sf::Keyboard::Key::Down);

    if (left) {
        m_moveLeftCommand.execute(m_playerInput);
    }

    if (right) {
        m_moveRightCommand.execute(m_playerInput);
    }

    if (jump) {
        m_jumpCommand.execute(m_playerInput);
    }

    if (crouch) {
        m_crouchCommand.execute(m_playerInput);
    }
}

void InputHandler::reset() {
    m_playerInput = PlayerInput{};
}

const PlayerInput& InputHandler::getPlayerInput() const noexcept {
    return m_playerInput;
}
