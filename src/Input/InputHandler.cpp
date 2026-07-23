#include "Input/InputHandler.hpp"
#include <SFML/Window/Keyboard.hpp>

void InputHandler::update() {
    const bool left = 
    sf::Keyboard::isKeyPressed(sf::Keyboard::Scancode::A) ||
    sf::Keyboard::isKeyPressed(sf::Keyboard::Scancode::Left);
    
    const bool right = 
    sf::Keyboard::isKeyPressed(sf::Keyboard::Scancode::D) ||
    sf::Keyboard::isKeyPressed(sf::Keyboard::Scancode::Right);

    playerInput.moveAxis = static_cast<float>(right) - static_cast<float>(left);

    playerInput.jumpHeld = sf::Keyboard::isKeyPressed(sf::Keyboard::Scancode::Space);
}

void InputHandler::reset() {
    playerInput = PlayerInput{};
}

const PlayerInput& InputHandler::getPlayerInput() const {
    return playerInput;
}

