#include "States/PlayState.hpp"
#include "States/GameStateManager.hpp"
#include "States/IntroMenuState.hpp"
#include <algorithm>
#include <iostream>

PlayState::PlayState(GameStateManager& gsm, Systems::AssetManager& assets, CharacterType character)
    : State(gsm, assets), selectedCharacter(character), bgSprite(assets.getTexture("LevelTilemap")) {}

void PlayState::init() {
    std::string charName = (selectedCharacter == CharacterType::Mario) ? "Mario" : "Luigi";
    std::cout << "[Core Engine] PlayState Initialized with character: " << charName << "\n";

    // Load level 1 map file
    if (mapParser.loadFromFile("/Users/tranquochuy/Downloads/CS202_MarioGame/assets/maps/level1.txt")) {
        mapParser.printMap();
    } else {
        std::cerr << "[Core Engine] Warning: Failed to load level1.txt map!\n";
    }

    // Zoom the tilemap in so it fills the window height, preserving its aspect ratio.
    // This makes the level wider than the window (a real side-scroller camera), so we
    // scroll a view across it horizontally instead of squeezing the whole level into
    // one screen.
    sf::Vector2u windowSize = {1200u, 800u};
    sf::Vector2u textureSize = bgSprite.getTexture().getSize();
    float scale = static_cast<float>(windowSize.y) / textureSize.y;
    bgSprite.setScale({scale, scale});
    bgSprite.setPosition({0.f, 0.f});
    levelWidth = textureSize.x * scale;
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
    // Scroll the camera across the level with the arrow keys (no player entity yet).
    const float scrollSpeed = 500.f; // pixels per second
    float windowWidth = 1200.f;
    float maxCameraX = std::max(0.f, levelWidth - windowWidth);

    if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Right)) {
        cameraX += scrollSpeed * dt.asSeconds();
    }
    if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Left)) {
        cameraX -= scrollSpeed * dt.asSeconds();
    }
    cameraX = std::clamp(cameraX, 0.f, maxCameraX);
}

void PlayState::render(sf::RenderWindow& window) {
    // Same sky blue as the tilemap image, so any edge gaps blend in seamlessly.
    window.clear(sf::Color(93, 148, 251));

    sf::View camera(sf::FloatRect({cameraX, 0.f}, {1200.f, 800.f}));
    window.setView(camera);
    window.draw(bgSprite);
    window.setView(window.getDefaultView());
}
