#include "States/RespawnState.hpp"
#include "States/PlayState.hpp"
#include "States/GameOverState.hpp"
#include "States/GameStateManager.hpp"
#include "Systems/SoundController.hpp"
#include "Core/Config.hpp"
#include <iostream>
#include <string>

RespawnState::RespawnState(GameStateManager& gsm, Systems::AssetManager& assets, const SaveData& data)
    : State(gsm, assets), worldText(assets.getFont("MarioFont")),
      livesText(assets.getFont("MarioFont")), promptText(assets.getFont("MarioFont")),
      characterSprite(assets.getTexture(data.selectedCharacter == CharacterType::Mario ? "MarioIdle" : "LuigiIdle")),
      progress(data), displayTimer(2.5f) {}

void RespawnState::init() {
    std::cout << "[Core Engine] RespawnState Initialized. Lives remaining: " << progress.lives << "\n";
    Systems::SoundController::getInstance().pauseMusic();

    bgShape.setSize({Config::kViewWidth, Config::kViewHeight});
    bgShape.setFillColor(sf::Color::Black);

    std::string worldStr = "WORLD "
        + std::to_string(Config::worldNumber(progress.currentLevel)) + "-"
        + std::to_string(Config::stageNumber(progress.currentLevel));
    worldText.setString(worldStr);
    worldText.setCharacterSize(36);
    worldText.setFillColor(sf::Color::White);
    sf::FloatRect wBounds = worldText.getLocalBounds();
    worldText.setOrigin({wBounds.position.x + wBounds.size.x / 2.f, wBounds.position.y + wBounds.size.y / 2.f});
    worldText.setPosition({Config::kViewWidth / 2.f, Config::kViewHeight * 0.35f});

    characterSprite.setScale({Config::kZoom, Config::kZoom});
    sf::FloatRect cBounds = characterSprite.getLocalBounds();
    characterSprite.setOrigin({cBounds.position.x + cBounds.size.x / 2.f, cBounds.position.y + cBounds.size.y / 2.f});
    characterSprite.setPosition({Config::kViewWidth / 2.f - 40.f, Config::kViewHeight * 0.52f});

    std::string livesStr = "x  " + std::to_string(progress.lives);
    livesText.setString(livesStr);
    livesText.setCharacterSize(32);
    livesText.setFillColor(sf::Color::White);
    sf::FloatRect lBounds = livesText.getLocalBounds();
    livesText.setOrigin({lBounds.position.x, lBounds.position.y + lBounds.size.y / 2.f});
    livesText.setPosition({Config::kViewWidth / 2.f + 10.f, Config::kViewHeight * 0.52f});

    promptText.setString("PRESS ENTER TO SKIP");
    promptText.setCharacterSize(16);
    promptText.setFillColor(sf::Color(180, 180, 180));
    sf::FloatRect pBounds = promptText.getLocalBounds();
    promptText.setOrigin({pBounds.position.x + pBounds.size.x / 2.f, pBounds.position.y + pBounds.size.y / 2.f});
    promptText.setPosition({Config::kViewWidth / 2.f, Config::kViewHeight * 0.85f});
}

void RespawnState::handleInput(const sf::Event& event) {
    if (const auto* keyPressed = event.getIf<sf::Event::KeyPressed>()) {
        displayTimer = 0.f; // Skip directly on any key
    }
}

void RespawnState::update(sf::Time dt) {
    displayTimer -= dt.asSeconds();
    if (displayTimer <= 0.f) {
        if (progress.lives > 0) {
            gsm.changeState(std::make_unique<PlayState>(gsm, assets, progress));
        } 
        else {
            gsm.changeState(std::make_unique<GameOverState>(gsm, assets, progress.gameMode));
        }
    }
}

void RespawnState::render(sf::RenderWindow& window) {
    window.draw(bgShape);
    window.draw(worldText);
    window.draw(characterSprite);
    window.draw(livesText);
    window.draw(promptText);
}
