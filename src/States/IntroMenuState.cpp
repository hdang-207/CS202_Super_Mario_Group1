#include "States/IntroMenuState.hpp"
#include "Core/Config.hpp"
#include "States/CharacterSelectionState.hpp"
#include "States/GameStateManager.hpp"
#include "Systems/ResourcePath.hpp"
#include "Systems/SoundController.hpp"
#include <algorithm>
#include <iostream>

IntroMenuState::IntroMenuState(GameStateManager& gsm, Systems::AssetManager& assets) 
    : State(gsm, assets), bgSprite(assets.getTexture("MenuBackground")),
      titleText(assets.getFont("MarioFont")), promptText(assets.getFont("MarioFont")) {}

void IntroMenuState::init() {
    // Log info representing game starting up.
    std::cout << "[Core Engine] IntroMenuState Initialized. Press ANY KEY to select character.\n";

    // Phóng ảnh nền để phủ kín màn hình mà không bị méo (giữ nguyên tỉ lệ gốc),
    // phần thừa được cắt đều hai bên.
    sf::Vector2f bgSize(bgSprite.getTexture().getSize());
    float bgScale = std::max(Config::kViewWidth / bgSize.x, Config::kViewHeight / bgSize.y);
    bgSprite.setScale({bgScale, bgScale});
    bgSprite.setPosition({(Config::kViewWidth - bgSize.x * bgScale) / 2.f,
                          (Config::kViewHeight - bgSize.y * bgScale) / 2.f});

    titleText.setString("SUPER MARIO BROS");
    titleText.setCharacterSize(48);
    titleText.setFillColor(sf::Color::Red);
    titleText.setOutlineColor(sf::Color::White);
    titleText.setOutlineThickness(3.f);

    sf::FloatRect titleBounds = titleText.getLocalBounds();
    titleText.setOrigin({titleBounds.position.x + titleBounds.size.x / 2.f,
                         titleBounds.position.y + titleBounds.size.y / 2.f});
    titleText.setPosition({Config::kViewWidth / 2.f, Config::kViewHeight * 0.25f});

    promptText.setString("PRESS ANY KEY TO START");
    promptText.setCharacterSize(24);
    promptText.setFillColor(sf::Color::Yellow);
    promptText.setOutlineColor(sf::Color::Black);
    promptText.setOutlineThickness(2.f);

    sf::FloatRect promptBounds = promptText.getLocalBounds();
    promptText.setOrigin({promptBounds.position.x + promptBounds.size.x / 2.f,
                          promptBounds.position.y + promptBounds.size.y / 2.f});
    promptText.setPosition({Config::kViewWidth / 2.f, Config::kViewHeight * 0.65f});

    Systems::SoundController::getInstance().playMusic(Systems::resourcePath("assets/audio/Theme.mp3"));
}

void IntroMenuState::handleInput(const sf::Event& event) {
    const auto* keyPressed = event.getIf<sf::Event::KeyPressed>();
    if (!keyPressed) {
        return;
    }

    sf::Keyboard::Key key = keyPressed->code;

    // Ignores system keys (Escape, Tab, CapsLock, F1-F12, Shift, Ctrl, Alt, System/Windows keys)
    if (key == sf::Keyboard::Key::Escape || key == sf::Keyboard::Key::Tab ||
        keyPressed->scancode == sf::Keyboard::Scancode::CapsLock ||
        key == sf::Keyboard::Key::Backspace ||
        key == sf::Keyboard::Key::LShift || key == sf::Keyboard::Key::RShift ||
        key == sf::Keyboard::Key::LControl || key == sf::Keyboard::Key::RControl ||
        key == sf::Keyboard::Key::LAlt || key == sf::Keyboard::Key::RAlt ||
        key == sf::Keyboard::Key::LSystem || key == sf::Keyboard::Key::RSystem ||
        (key >= sf::Keyboard::Key::F1 && key <= sf::Keyboard::Key::F12)) {
        return;
    }

    std::cout << "[Core Engine] Valid key pressed in IntroMenu. Transitioning to CharacterSelectionState...\n";
    gsm.changeState(std::make_unique<CharacterSelectionState>(gsm, assets));
}

void IntroMenuState::update(sf::Time dt) {
    blinkTimer += dt.asSeconds();
    if (blinkTimer >= 0.5f) {
        blinkTimer = 0.0f;
        showPrompt = !showPrompt;
    }
}

void IntroMenuState::render(sf::RenderWindow& window) {
    window.draw(bgSprite);
    window.draw(titleText);
    if (showPrompt) {
        window.draw(promptText);
    }
}
