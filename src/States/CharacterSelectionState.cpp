#include "States/CharacterSelectionState.hpp"
#include "States/IntroMenuState.hpp"
#include "States/PlayState.hpp"
#include "States/GameStateManager.hpp"
#include "Core/CharacterType.hpp"
#include <iostream>

CharacterSelectionState::CharacterSelectionState(GameStateManager& gsm) : State(gsm) {}

void CharacterSelectionState::init() {
    // Log user instructions for character selection on initialization.
    std::cout << "[Core Engine] CharacterSelectionState Initialized.\n";
    std::cout << "  - Press '1' to select Mario\n";
    std::cout << "  - Press '2' to select Luigi\n";
    std::cout << "  - Press 'B' to go back to Menu\n";
}

void CharacterSelectionState::handleInput(const sf::Event& event) {
    if (const auto* keyPressed = event.getIf<sf::Event::KeyPressed>()) {
        // Option 'B' to return back to the Intro Menu State
        if (keyPressed->code == sf::Keyboard::Key::B) {
            std::cout << "[Core Engine] Going back to IntroMenuState...\n";
            gsm.changeState(std::make_unique<IntroMenuState>(gsm));
        } 
        // Option '1' (or Numpad 1) to select Mario
        else if (keyPressed->code == sf::Keyboard::Key::Num1 || keyPressed->code == sf::Keyboard::Key::Numpad1) {
            std::cout << "[Core Engine] Mario Selected! Transitioning to PlayState...\n";
            gsm.changeState(std::make_unique<PlayState>(gsm, CharacterType::Mario));
        }
        // Option '2' (or Numpad 2) to select Luigi
        else if (keyPressed->code == sf::Keyboard::Key::Num2 || keyPressed->code == sf::Keyboard::Key::Numpad2) {
            std::cout << "[Core Engine] Luigi Selected! Transitioning to PlayState...\n";
            gsm.changeState(std::make_unique<PlayState>(gsm, CharacterType::Luigi));
        }
    }
}

void CharacterSelectionState::update(sf::Time dt) {
    // Skeleton update logic - currently no animations or physics are updated in selection screen.
}

void CharacterSelectionState::render(sf::RenderWindow& window) {
    // Clear screen with Sea Green to represent the character selection screen background.
    window.clear(sf::Color(46, 139, 87));
}

