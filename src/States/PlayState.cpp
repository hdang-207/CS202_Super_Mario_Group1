#include "States/PlayState.hpp"
#include "States/GameStateManager.hpp"
#include "States/IntroMenuState.hpp"
#include <iostream>

PlayState::PlayState(GameStateManager& gsm, Systems::AssetManager& assets, CharacterType character)
    : State(gsm, assets), selectedCharacter(character) {}

void PlayState::init() {
    std::string charName = (selectedCharacter == CharacterType::Mario) ? "Mario" : "Luigi";
    std::cout << "[Core Engine] PlayState Initialized with character: " << charName << "\n";

    // Load level 1 map file
    if (mapParser.loadFromFile("assets/maps/level1.txt")) {
        mapParser.printMap();
    } else {
        std::cerr << "[Core Engine] Warning: Failed to load level1.txt map!\n";
    }

    // Scatter a handful of decorative clouds across the sky at varying heights.
    clouds.emplace_back(sf::Vector2f(60.f, 50.f));
    clouds.emplace_back(sf::Vector2f(400.f, 110.f));
    clouds.emplace_back(sf::Vector2f(750.f, 60.f));
    clouds.emplace_back(sf::Vector2f(980.f, 140.f));
}

void PlayState::handleInput(const sf::Event& event) {
    if (const auto* keyPressed = event.getIf<sf::Event::KeyPressed>()) {
        // Press Escape to return to Main Menu
        if (keyPressed->code == sf::Keyboard::Key::Escape) {
            std::cout << "[Core Engine] Escape pressed in PlayState. Returning to IntroMenuState...\n";
            gsm.changeState(std::make_unique<PlayState>(gsm, assets, CharacterType::Mario));
        }
    }
}

void PlayState::update(sf::Time dt) {
    // Skeleton gameplay update loop
}

void PlayState::render(sf::RenderWindow& window) {
    // Clear screen with Super Mario Sky Blue color
    window.clear(sf::Color(107, 140, 255));

    for (auto& cloud : clouds) {
        cloud.render(window);
    }
}
