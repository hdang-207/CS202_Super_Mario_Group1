#include "States/GameOverState.hpp"
#include "States/IntroMenuState.hpp"
#include "States/GameStateManager.hpp"
#include "Systems/SaveManager.hpp"
#include "Core/Config.hpp"
#include "Systems/SoundController.hpp"
#include <iostream>

GameOverState::GameOverState(GameStateManager& gsm, Systems::AssetManager& assets, GameMode mode)
    : State(gsm, assets), m_gameMode(mode), gameOverText(assets.getFont("MarioFont")), promptText(assets.getFont("MarioFont")) {}

void GameOverState::init() {
    std::cout << "[Core Engine] GameOverState Initialized. Clearing save progress...\n";
    SaveManager::deleteSaveFile("savegame.txt");

    Systems::SoundController::getInstance().stopMusic();
    Systems::SoundController::getInstance().playSound(assets.getSoundBuffer("GameOverSound"));

    std::string texName = "NormalGameover";
    if (m_gameMode == GameMode::Nightfall) texName = "NightfallGameover";
    else if (m_gameMode == GameMode::Inferno) texName = "InfernoGameover";
    else if (m_gameMode == GameMode::Apocalypse) texName = "ApocalypseGameover";

    const auto& tex = assets.getTexture(texName);
    bgSprite.emplace(tex);

    // Scale to fit screen
    float scaleX = Config::kViewWidth / static_cast<float>(tex.getSize().x);
    float scaleY = Config::kViewHeight / static_cast<float>(tex.getSize().y);
    float scale = std::max(scaleX, scaleY);
    bgSprite->setScale({scale, scale});
    
    // Center it
    bgSprite->setOrigin({tex.getSize().x / 2.f, tex.getSize().y / 2.f});
    bgSprite->setPosition({Config::kViewWidth / 2.f, Config::kViewHeight / 2.f});

    promptText.setString("PRESS ENTER TO MENU");
    promptText.setCharacterSize(24);
    promptText.setFillColor(sf::Color::Yellow);
    promptText.setOutlineColor(sf::Color::Black);
    promptText.setOutlineThickness(2.f);
    sf::FloatRect pBounds = promptText.getLocalBounds();
    promptText.setOrigin({pBounds.position.x + pBounds.size.x / 2.f, pBounds.position.y + pBounds.size.y / 2.f});
    promptText.setPosition({Config::kViewWidth / 2.f, Config::kViewHeight * 0.9f}); // moved down a bit

    gameOverText.setString("GAME OVER");
    gameOverText.setCharacterSize(48);
    gameOverText.setFillColor(sf::Color::White);
    gameOverText.setOutlineColor(sf::Color::Black);
    gameOverText.setOutlineThickness(3.f);
    sf::FloatRect bounds = gameOverText.getLocalBounds();
    gameOverText.setOrigin({bounds.position.x + bounds.size.x / 2.f, bounds.position.y + bounds.size.y / 2.f});
    gameOverText.setPosition({Config::kViewWidth / 2.f, Config::kViewHeight * 0.4f});
}

void GameOverState::handleInput(const sf::Event& event) {
    if (const auto* keyPressed = event.getIf<sf::Event::KeyPressed>()) {
        if (keyPressed->scancode == sf::Keyboard::Scancode::Enter) {
            gsm.changeState(std::make_unique<IntroMenuState>(gsm, assets));
        }
    }
}

void GameOverState::update(sf::Time) {
}

void GameOverState::render(sf::RenderWindow& window) {
    if (bgSprite.has_value()) {
        window.draw(*bgSprite);
    }
    // Draw dimming overlay if needed, currently not created but could be added if backgrounds are too bright
    window.draw(gameOverText);
    window.draw(promptText);
}
