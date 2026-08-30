#include "Input/InputHandler.hpp"

void InputHandler::update(const std::set<sf::Keyboard::Scancode>& heldKeys) {
    m_playerInput = PlayerInput{};

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
    const bool crouch = holding(sf::Keyboard::Scancode::S)
                     || holding(sf::Keyboard::Scancode::Down);
    const bool shoot = holding(sf::Keyboard::Scancode::X);
    const bool bomb = holding(sf::Keyboard::Scancode::C);

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
