#include "States/IntroMenuState.hpp"
#include "States/CharacterSelectionState.hpp"
#include "States/GameStateManager.hpp"
#include <iostream>

IntroMenuState::IntroMenuState(GameStateManager& gsm) : State(gsm) {}

void IntroMenuState::init() {
    // Log info representing game starting up.
    std::cout << "[Core Engine] IntroMenuState Initialized. Press ENTER to select character.\n";
}

void IntroMenuState::handleInput(const sf::Event& event) {
    if (const auto* keyPressed = event.getIf<sf::Event::KeyPressed>()) {
        // If the user hits Enter, switch the state to CharacterSelectionState.
        if (keyPressed->code == sf::Keyboard::Key::Enter) {
            std::cout << "[Core Engine] Enter pressed in IntroMenu. Transitioning to CharacterSelectionState...\n";
            gsm.changeState(std::make_unique<CharacterSelectionState>(gsm));
        }
    }
}

void IntroMenuState::update(sf::Time dt) {
    // Skeleton update logic - currently no animations or physics are updated in menu.
}

void IntroMenuState::render(sf::RenderWindow& window) {
    // Clear screen with Cornflower Blue to represent the menu background.
    window.clear(sf::Color(100, 149, 237));
}

