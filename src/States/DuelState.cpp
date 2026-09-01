#include "States/DuelState.hpp"

#include "Core/Config.hpp"
#include "States/GameStateManager.hpp"
#include "Systems/SoundController.hpp"
#include <algorithm>
#include <iostream>

DuelState::DuelState(GameStateManager& gsm, Systems::AssetManager& assets)
    : State(gsm, assets),
      bgSprite(assets.getTexture("MenuBackground")),
      titleText(assets.getFont("MarioFont")),
      placeholderText(assets.getFont("MarioFont")),
      hintText(assets.getFont("MarioFont")) {}

void DuelState::init() {
    std::cout << "[Core Engine] DuelState Initialized.\n";

    const sf::Vector2f bgSize(bgSprite.getTexture().getSize());
    const float bgScale = std::max(Config::kViewWidth / bgSize.x,
                                   Config::kViewHeight / bgSize.y);
    bgSprite.setScale({bgScale, bgScale});
    bgSprite.setPosition({(Config::kViewWidth - bgSize.x * bgScale) / 2.f,
                          (Config::kViewHeight - bgSize.y * bgScale) / 2.f});

    darkOverlay.setSize({Config::kViewWidth, Config::kViewHeight});
    darkOverlay.setFillColor(sf::Color(0, 0, 0, 200));

    titleText.setString("DUEL MODE");
    titleText.setCharacterSize(48);
    titleText.setFillColor(sf::Color::Yellow);
    titleText.setOutlineColor(sf::Color::Black);
    titleText.setOutlineThickness(3.f);
    const sf::FloatRect titleBounds = titleText.getLocalBounds();
    titleText.setOrigin({titleBounds.position.x + titleBounds.size.x / 2.f,
                         titleBounds.position.y + titleBounds.size.y / 2.f});
    titleText.setPosition({Config::kViewWidth / 2.f,
                           Config::kViewHeight * 0.25f});

    placeholderText.setString("ARENA SETUP PENDING");
    placeholderText.setCharacterSize(24);
    placeholderText.setFillColor(sf::Color::White);
    placeholderText.setOutlineColor(sf::Color::Black);
    placeholderText.setOutlineThickness(2.f);
    const sf::FloatRect placeholderBounds = placeholderText.getLocalBounds();
    placeholderText.setOrigin({placeholderBounds.position.x
                                   + placeholderBounds.size.x / 2.f,
                               placeholderBounds.position.y
                                   + placeholderBounds.size.y / 2.f});
    placeholderText.setPosition({Config::kViewWidth / 2.f,
                                 Config::kViewHeight * 0.50f});

    hintText.setString("B / ESC: BACK");
    hintText.setCharacterSize(18);
    hintText.setFillColor(sf::Color::White);
    hintText.setOutlineColor(sf::Color::Black);
    hintText.setOutlineThickness(2.f);
    const sf::FloatRect hintBounds = hintText.getLocalBounds();
    hintText.setOrigin({hintBounds.position.x + hintBounds.size.x / 2.f,
                        hintBounds.position.y + hintBounds.size.y / 2.f});
    hintText.setPosition({Config::kViewWidth / 2.f,
                          Config::kViewHeight * 0.82f});
}

void DuelState::handleInput(const sf::Event& event) {
    const auto* keyPressed = event.getIf<sf::Event::KeyPressed>();
    if (!keyPressed) {
        return;
    }

    const auto key = keyPressed->scancode;
    if (key == sf::Keyboard::Scancode::B
        || key == sf::Keyboard::Scancode::Escape) {
        Systems::SoundController::getInstance().playSound(
            assets.getSoundBuffer("SelectSound"));
        gsm.popState();
    }
}

void DuelState::update(sf::Time) {}

void DuelState::render(sf::RenderWindow& window) {
    window.draw(bgSprite);
    window.draw(darkOverlay);
    window.draw(titleText);
    window.draw(placeholderText);
    window.draw(hintText);
}
