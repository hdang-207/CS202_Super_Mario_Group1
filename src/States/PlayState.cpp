#include "States/PlayState.hpp"
#include "States/GameStateManager.hpp"
#include "States/IntroMenuState.hpp"
#include <iostream>

PlayState::PlayState(GameStateManager& gsm, Systems::AssetManager& assets, CharacterType character)
    : State(gsm, assets), selectedCharacter(character), bgSprite(assets.getTexture("CloudBackground")) {}

void PlayState::init() {
    std::string charName = (selectedCharacter == CharacterType::Mario) ? "Mario" : "Luigi";
    std::cout << "[Core Engine] PlayState Initialized with character: " << charName << "\n";

    // Load level 1 map file
    if (mapParser.loadFromFile("/Users/tranquochuy/Downloads/CS202_MarioGame/assets/maps/level1.txt")) {
        mapParser.printMap();
    } else {
        std::cerr << "[Core Engine] Warning: Failed to load level1.txt map!\n";
    }

    // Scale the sky/clouds background image to fill the window. Its aspect ratio (1536x1024)
    // already matches the window's (1200x800), so this does not distort the image.
    sf::Vector2u textureSize = bgSprite.getTexture().getSize();
    sf::Vector2u windowSize = {1200u, 800u};
    bgSprite.setPosition({0.f, 0.f});
    bgSprite.setScale({static_cast<float>(windowSize.x) / textureSize.x,
                        static_cast<float>(windowSize.y) / textureSize.y});
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
    // Same sky blue as the background image, so the letterboxed edges blend in seamlessly.
    window.clear(sf::Color(91, 146, 247));
    window.draw(bgSprite);
}
