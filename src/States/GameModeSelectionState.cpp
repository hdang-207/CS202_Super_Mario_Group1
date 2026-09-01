#include "States/GameModeSelectionState.hpp"
#include "Core/Config.hpp"
#include "States/CharacterSelectionState.hpp"
#include "States/IntroMenuState.hpp"
#include "States/GameStateManager.hpp"
#include "Systems/SoundController.hpp"
#include <algorithm>
#include <iostream>

GameModeSelectionState::GameModeSelectionState(GameStateManager& gsm, Systems::AssetManager& assets)
    : State(gsm, assets),
      bgSprite(assets.getTexture("MenuBackground")),
      headerText(assets.getFont("MarioFont")),
      normalText(assets.getFont("MarioFont")),
      nightfallText(assets.getFont("MarioFont")),
      normalDesc(assets.getFont("MarioFont")),
      nightfallDesc(assets.getFont("MarioFont")),
      hintText(assets.getFont("MarioFont")) {}

void GameModeSelectionState::init() {
    std::cout << "[Core Engine] GameModeSelectionState Initialized.\n";

    // Background
    const sf::Vector2f bgSize(bgSprite.getTexture().getSize());
    const float bgScale = std::max(Config::kViewWidth / bgSize.x,
                                   Config::kViewHeight / bgSize.y);
    bgSprite.setScale({bgScale, bgScale});
    bgSprite.setPosition({(Config::kViewWidth - bgSize.x * bgScale) / 2.f,
                          (Config::kViewHeight - bgSize.y * bgScale) / 2.f});

    darkOverlay.setSize({Config::kViewWidth, Config::kViewHeight});
    darkOverlay.setFillColor(sf::Color(0, 0, 0, 175));

    // Header
    headerText.setString("SELECT GAME MODE");
    headerText.setCharacterSize(38);
    headerText.setFillColor(sf::Color::Yellow);
    headerText.setOutlineColor(sf::Color::Black);
    headerText.setOutlineThickness(3.f);
    const sf::FloatRect headerBounds = headerText.getLocalBounds();
    headerText.setOrigin({headerBounds.position.x + headerBounds.size.x / 2.f,
                          headerBounds.position.y + headerBounds.size.y / 2.f});
    headerText.setPosition({Config::kViewWidth / 2.f, Config::kViewHeight * 0.15f});

    // Options
    for (sf::Text* t : {&normalText, &nightfallText}) {
        t->setCharacterSize(28);
        t->setOutlineColor(sf::Color::Black);
        t->setOutlineThickness(2.f);
    }

    // Descriptions
    for (sf::Text* t : {&normalDesc, &nightfallDesc}) {
        t->setCharacterSize(16);
        t->setOutlineColor(sf::Color::Black);
        t->setOutlineThickness(2.f);
    }
    normalDesc.setString("Classic gameplay with full visibility");
    normalDesc.setFillColor(sf::Color(200, 200, 200));

    nightfallDesc.setString("Darkness surrounds you. Only a small light guides your way!");
    nightfallDesc.setFillColor(sf::Color(200, 200, 200));

    // Hint
    hintText.setString("UP/DOWN: SELECT  |  ENTER: CONFIRM  |  B: BACK");
    hintText.setCharacterSize(15);
    hintText.setFillColor(sf::Color::White);
    hintText.setOutlineColor(sf::Color::Black);
    hintText.setOutlineThickness(2.f);
    const sf::FloatRect hintBounds = hintText.getLocalBounds();
    hintText.setOrigin({hintBounds.position.x + hintBounds.size.x / 2.f,
                        hintBounds.position.y + hintBounds.size.y / 2.f});
    hintText.setPosition({Config::kViewWidth / 2.f, Config::kViewHeight * 0.88f});
}

void GameModeSelectionState::handleInput(const sf::Event& event) {
    const auto* keyPressed = event.getIf<sf::Event::KeyPressed>();
    if (!keyPressed) return;

    const auto key = keyPressed->scancode;
    if (key == sf::Keyboard::Scancode::Up || key == sf::Keyboard::Scancode::W) {
        selectedIndex = 0;
        Systems::SoundController::getInstance().playSound(
            assets.getSoundBuffer("SelectSound"));
    } else if (key == sf::Keyboard::Scancode::Down || key == sf::Keyboard::Scancode::S) {
        selectedIndex = 1;
        Systems::SoundController::getInstance().playSound(
            assets.getSoundBuffer("SelectSound"));
    } else if (key == sf::Keyboard::Scancode::Enter
               || key == sf::Keyboard::Scancode::Space) {
        Systems::SoundController::getInstance().playSound(
            assets.getSoundBuffer("SelectSound"));
        bool nightfall = (selectedIndex == 1);
        std::cout << "[Core Engine] Game mode selected: "
                  << (nightfall ? "NIGHTFALL" : "NORMAL") << "\n";
        gsm.changeState(std::make_unique<CharacterSelectionState>(gsm, assets, nightfall));
    } else if (key == sf::Keyboard::Scancode::B
               || key == sf::Keyboard::Scancode::Escape) {
        Systems::SoundController::getInstance().playSound(
            assets.getSoundBuffer("SelectSound"));
        gsm.changeState(std::make_unique<IntroMenuState>(gsm, assets));
    }
}

void GameModeSelectionState::update(sf::Time) {
    // Update option labels
    normalText.setString(selectedIndex == 0 ? "> NORMAL MODE" : "  NORMAL MODE");
    normalText.setFillColor(selectedIndex == 0 ? sf::Color::Yellow : sf::Color(180, 180, 180));
    const sf::FloatRect nBounds = normalText.getLocalBounds();
    normalText.setOrigin({nBounds.position.x + nBounds.size.x / 2.f,
                          nBounds.position.y + nBounds.size.y / 2.f});
    normalText.setPosition({Config::kViewWidth / 2.f, Config::kViewHeight * 0.38f});

    // Normal description
    const sf::FloatRect ndBounds = normalDesc.getLocalBounds();
    normalDesc.setOrigin({ndBounds.position.x + ndBounds.size.x / 2.f,
                          ndBounds.position.y + ndBounds.size.y / 2.f});
    normalDesc.setPosition({Config::kViewWidth / 2.f, Config::kViewHeight * 0.44f});

    nightfallText.setString(selectedIndex == 1 ? "> NIGHTFALL MODE" : "  NIGHTFALL MODE");
    nightfallText.setFillColor(selectedIndex == 1 ? sf::Color(255, 140, 0) : sf::Color(180, 180, 180));
    const sf::FloatRect nfBounds = nightfallText.getLocalBounds();
    nightfallText.setOrigin({nfBounds.position.x + nfBounds.size.x / 2.f,
                             nfBounds.position.y + nfBounds.size.y / 2.f});
    nightfallText.setPosition({Config::kViewWidth / 2.f, Config::kViewHeight * 0.55f});

    // Nightfall description
    const sf::FloatRect nfdBounds = nightfallDesc.getLocalBounds();
    nightfallDesc.setOrigin({nfdBounds.position.x + nfdBounds.size.x / 2.f,
                             nfdBounds.position.y + nfdBounds.size.y / 2.f});
    nightfallDesc.setPosition({Config::kViewWidth / 2.f, Config::kViewHeight * 0.61f});
}

void GameModeSelectionState::render(sf::RenderWindow& window) {
    window.draw(bgSprite);
    window.draw(darkOverlay);
    window.draw(headerText);
    window.draw(normalText);
    window.draw(normalDesc);
    window.draw(nightfallText);
    window.draw(nightfallDesc);
    window.draw(hintText);
}
