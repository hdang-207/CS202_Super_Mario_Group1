#include "Input/InputHandler.hpp"
#include <SFML/Window/Keyboard.hpp>

void InputHandler::update() {

    reset();

    const bool left = 
    sf::Keyboard::isKeyPressed(sf::Keyboard::Scancode::A) ||
    sf::Keyboard::isKeyPressed(sf::Keyboard::Scancode::Left);
    
    const bool right = 
    sf::Keyboard::isKeyPressed(sf::Keyboard::Scancode::D) ||
    sf::Keyboard::isKeyPressed(sf::Keyboard::Scancode::Right);

    const bool jump = 
        sf::Keyboard::isKeyPressed(sf::Keyboard::Scancode::Space) ||
        sf::Keyboard::isKeyPressed(sf::Keyboard::Scancode::W) ||
        sf::Keyboard::isKeyPressed(sf::Keyboard::Scancode::Up);

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

