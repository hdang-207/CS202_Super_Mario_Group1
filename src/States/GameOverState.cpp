#include "States/GameOverState.hpp"
#include "States/IntroMenuState.hpp"
#include "States/GameStateManager.hpp"
#include "Core/Config.hpp"
#include <iostream>

GameOverState::GameOverState(GameStateManager& gsm, Systems::AssetManager& assets)
    : State(gsm, assets), gameOverText(assets.getFont("MarioFont")), promptText(assets.getFont("MarioFont")) {}

#include "Systems/SoundController.hpp"

void GameOverState::init() {
    std::cout << "[Core Engine] GameOverState Initialized.\n";

    Systems::SoundController::getInstance().stopMusic();
    Systems::SoundController::getInstance().playSound(assets.getSoundBuffer("GameOverSound"));

    bgShape.setSize({Config::kViewWidth, Config::kViewHeight});
    bgShape.setFillColor(sf::Color(100, 100, 100)); // Gray background

    gameOverText.setString("GAME OVER");
    gameOverText.setCharacterSize(48);
    gameOverText.setFillColor(sf::Color::White);
    gameOverText.setOutlineColor(sf::Color::Black);
    gameOverText.setOutlineThickness(3.f);
    sf::FloatRect bounds = gameOverText.getLocalBounds();
    gameOverText.setOrigin({bounds.position.x + bounds.size.x / 2.f, bounds.position.y + bounds.size.y / 2.f});
    gameOverText.setPosition({Config::kViewWidth / 2.f, Config::kViewHeight * 0.4f});

    promptText.setString("PRESS ENTER TO MENU");
    promptText.setCharacterSize(24);
    promptText.setFillColor(sf::Color::Yellow);
    promptText.setOutlineColor(sf::Color::Black);
    promptText.setOutlineThickness(2.f);
    sf::FloatRect pBounds = promptText.getLocalBounds();
    promptText.setOrigin({pBounds.position.x + pBounds.size.x / 2.f, pBounds.position.y + pBounds.size.y / 2.f});
    promptText.setPosition({Config::kViewWidth / 2.f, Config::kViewHeight * 0.6f});
}

void GameOverState::handleInput(const sf::Event& event) {
    if (const auto* keyPressed = event.getIf<sf::Event::KeyPressed>()) {
        if (keyPressed->code == sf::Keyboard::Key::Enter) {
            gsm.changeState(std::make_unique<IntroMenuState>(gsm, assets));
        }
    }
}

void GameOverState::update(sf::Time dt) {
}

void GameOverState::render(sf::RenderWindow& window) {
    window.draw(bgShape);
    window.draw(gameOverText);
    window.draw(promptText);
}
