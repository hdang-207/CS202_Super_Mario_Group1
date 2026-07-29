#include "States/IntroMenuState.hpp"
#include "Core/Config.hpp"
#include "States/CharacterSelectionState.hpp"
#include "States/GameStateManager.hpp"
#include "Systems/ResourcePath.hpp"
#include <algorithm>
#include <iostream>

IntroMenuState::IntroMenuState(GameStateManager& gsm, Systems::AssetManager& assets) 
    : State(gsm, assets),titleText(assets.getFont("MarioFont")),bgSprite(assets.getTexture("MenuBackground")) {}

void IntroMenuState::init() {
    // Log info representing game starting up.
    std::cout << "[Core Engine] IntroMenuState Initialized. Press ENTER to select character.\n";

    // Phóng ảnh nền để phủ kín màn hình mà không bị méo (giữ nguyên tỉ lệ gốc),
    // phần thừa được cắt đều hai bên.
    sf::Vector2f bgSize(bgSprite.getTexture().getSize());
    float bgScale = std::max(Config::kViewWidth / bgSize.x, Config::kViewHeight / bgSize.y);
    bgSprite.setScale({bgScale, bgScale});
    bgSprite.setPosition({(Config::kViewWidth - bgSize.x * bgScale) / 2.f,
                          (Config::kViewHeight - bgSize.y * bgScale) / 2.f});

    titleText.setString("SUPER MARIO BROS");
    titleText.setCharacterSize(64); // Kích thước chữ lớn
    titleText.setFillColor(sf::Color::Blue); // Màu đỏ đặc trưng của Mario

    // Căn giữa Tên Game theo chiều ngang của vùng chơi
    sf::FloatRect titleBounds = titleText.getLocalBounds();
    titleText.setOrigin({titleBounds.position.x + titleBounds.size.x / 2.f,
                             titleBounds.position.y + titleBounds.size.y / 2.f});
    titleText.setPosition({Config::kViewWidth / 2.f, Config::kViewHeight * 0.25f});

    if (bgMusic.openFromFile(Systems::resourcePath("assets/audio/Theme.mp3"))) {
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

