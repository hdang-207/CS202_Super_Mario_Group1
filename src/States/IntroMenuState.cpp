#include "States/IntroMenuState.hpp"
#include "States/CharacterSelectionState.hpp"
#include "States/GameStateManager.hpp"
#include <iostream>

IntroMenuState::IntroMenuState(GameStateManager& gsm, Systems::AssetManager& assets) 
    : State(gsm, assets),titleText(assets.getFont("MarioFont")),bgSprite(assets.getTexture("MenuBackground")) {}

void IntroMenuState::init() {
    // Log info representing game starting up.
    std::cout << "[Core Engine] IntroMenuState Initialized. Press ENTER to select character.\n";

    bgSprite.setPosition({0.f, 0.f});

    titleText.setString("SUPER MARIO BROS");
    titleText.setCharacterSize(48); // Kích thước chữ lớn
    titleText.setFillColor(sf::Color::Blue); // Màu đỏ đặc trưng của Mario

    // Căn giữa Tên Game trên màn hình (Giả sử màn hình 800x600)
    sf::FloatRect titleBounds = titleText.getLocalBounds();
    titleText.setOrigin({titleBounds.position.x + titleBounds.size.x / 2.f, 
                             titleBounds.position.y + titleBounds.size.y / 2.f});
    titleText.setPosition({400.f, 150.f}); 

    if (bgMusic.openFromFile("/Users/tranquochuy/Downloads/CS202_MarioGame/assets/audio/Theme.mp3")) {
    bgMusic.setLooping(true); // Lặp lại liên tục khi ở Menu
    bgMusic.setVolume(60.f);  // Mức âm lượng (0 - 100)
    bgMusic.play();           // Phát nhạc ngay khi mở Menu!
    }
}

void IntroMenuState::handleInput(const sf::Event& event) {
    if (const auto* keyPressed = event.getIf<sf::Event::KeyPressed>()) {
        // If the user hits Enter, switch the state to CharacterSelectionState.
        if (keyPressed->code == sf::Keyboard::Key::Enter) {
            std::cout << "[Core Engine] Enter pressed in IntroMenu. Transitioning to CharacterSelectionState...\n";
            gsm.changeState(std::make_unique<CharacterSelectionState>(gsm, assets));
        }
    }
}

void IntroMenuState::update(sf::Time dt) {
    // Skeleton update logic - currently no animations or physics are updated in menu.
}

void IntroMenuState::render(sf::RenderWindow& window) {
    // Clear screen with Cornflower Blue to represent the menu background.
    window.draw(bgSprite);
    window.draw(titleText);
}

