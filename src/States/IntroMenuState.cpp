#include "States/IntroMenuState.hpp"
#include "Core/Config.hpp"
#include "States/CharacterSelectionState.hpp"
#include "States/PlayState.hpp"
#include "States/GameStateManager.hpp"
#include "Systems/ResourcePath.hpp"
#include "Systems/SaveManager.hpp"
#include "Systems/SoundController.hpp"
#include <algorithm>
#include <iostream>

IntroMenuState::IntroMenuState(GameStateManager& gsm, Systems::AssetManager& assets) 
    : State(gsm, assets), bgSprite(assets.getTexture("MenuBackground")),
      titleText(assets.getFont("MarioFont")), promptText(assets.getFont("MarioFont")) {}

void IntroMenuState::init() {
    // Log info representing game starting up.
    std::cout << "[Core Engine] IntroMenuState Initialized. Press ENTER for new game or L to load.\n";

    // Scale background image to fill screen while preserving aspect ratio
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

    promptText.setString("PRESS ANY KEY TO START | PRESS L TO LOAD GAME");
    promptText.setCharacterSize(20);
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

    sf::Keyboard::Scancode key = keyPressed->scancode;

    // Ignores system keys
    if (key == sf::Keyboard::Scancode::Escape || key == sf::Keyboard::Scancode::Tab ||
        key == sf::Keyboard::Scancode::CapsLock ||
        key == sf::Keyboard::Scancode::Backspace ||
        key == sf::Keyboard::Scancode::LShift || key == sf::Keyboard::Scancode::RShift ||
        key == sf::Keyboard::Scancode::LControl || key == sf::Keyboard::Scancode::RControl ||
        key == sf::Keyboard::Scancode::LAlt || key == sf::Keyboard::Scancode::RAlt ||
        key == sf::Keyboard::Scancode::LSystem || key == sf::Keyboard::Scancode::RSystem ||
        (key >= sf::Keyboard::Scancode::F1 && key <= sf::Keyboard::Scancode::F12)) {
        return;
    }

    if (key == sf::Keyboard::Scancode::L) {
        Systems::SoundController::getInstance().playSound(assets.getSoundBuffer("SelectSound"));
        SaveData data;
        if (SaveManager::loadProgress("savegame.txt", data)) {
            std::cout << "[Core Engine] Loaded saved game successfully! Launching PlayState...\n";
            gsm.changeState(std::make_unique<PlayState>(gsm, assets, data));
            return;
        } else {
            std::cout << "[Core Engine] No valid save file found or failed to load.\n";
        }
    }

    std::cout << "[Core Engine] Valid key pressed in IntroMenu. Transitioning to CharacterSelectionState...\n";
    Systems::SoundController::getInstance().playSound(assets.getSoundBuffer("SelectSound"));
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
