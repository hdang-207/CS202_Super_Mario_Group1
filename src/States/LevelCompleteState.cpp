#include "States/LevelCompleteState.hpp"
#include "States/IntroMenuState.hpp"
#include "States/PlayState.hpp"
#include "States/RespawnState.hpp"
#include "States/VictoryState.hpp"
#include "States/GameStateManager.hpp"
#include "Systems/SaveManager.hpp"
#include "Systems/SoundController.hpp"
#include "Core/Config.hpp"
#include <iostream>
#include <string>

LevelCompleteState::LevelCompleteState(GameStateManager& gsm, Systems::AssetManager& assets, const SaveData& data)
    : State(gsm, assets), titleText(assets.getFont("MarioFont")), statsText(assets.getFont("MarioFont")),
      promptText(assets.getFont("MarioFont")), progress(data) {}

void LevelCompleteState::init() {
    std::cout << "[Core Engine] LevelCompleteState Initialized for Level " << progress.currentLevel << "\n";

    bgShape.setSize({Config::kViewWidth, Config::kViewHeight});
    bgShape.setFillColor(sf::Color(20, 40, 100)); // Dark royal blue background

    const bool finalLevel = progress.currentLevel >= Config::kFinalLevel;
    std::string title = finalLevel
        ? "YOU WIN THE GAME!"
        : "WORLD " + std::to_string(Config::worldNumber(progress.currentLevel))
            + "-" + std::to_string(Config::stageNumber(progress.currentLevel))
            + " CLEAR!";
    titleText.setString(title);
    titleText.setCharacterSize(44);
    titleText.setFillColor(sf::Color::Yellow);
    titleText.setOutlineColor(sf::Color::Black);
    titleText.setOutlineThickness(3.f);
    sf::FloatRect tBounds = titleText.getLocalBounds();
    titleText.setOrigin({tBounds.position.x + tBounds.size.x / 2.f, tBounds.position.y + tBounds.size.y / 2.f});
    titleText.setPosition({Config::kViewWidth / 2.f, Config::kViewHeight * 0.28f});

    std::string stats = "TOTAL SCORE : " + std::to_string(progress.score) + "\n\n" +
                        "COINS       : " + std::to_string(progress.coins) + "\n\n" +
                        "LIVES       : " + std::to_string(progress.lives);
    statsText.setString(stats);
    statsText.setCharacterSize(22);
    statsText.setFillColor(sf::Color::White);
    statsText.setOutlineColor(sf::Color::Black);
    statsText.setOutlineThickness(2.f);
    sf::FloatRect sBounds = statsText.getLocalBounds();
    statsText.setOrigin({sBounds.position.x + sBounds.size.x / 2.f, sBounds.position.y + sBounds.size.y / 2.f});
    statsText.setPosition({Config::kViewWidth / 2.f, Config::kViewHeight * 0.52f});

    std::string prompt = finalLevel
        ? "PRESS ENTER TO RETURN TO MAIN MENU"
        : "PRESS ENTER TO NEXT LEVEL  |  PRESS S TO SAVE & MENU";
    promptText.setString(prompt);
    promptText.setCharacterSize(18);
    promptText.setFillColor(sf::Color::White);
    promptText.setOutlineColor(sf::Color::Black);
    promptText.setOutlineThickness(2.f);
    sf::FloatRect pBounds = promptText.getLocalBounds();
    promptText.setOrigin({pBounds.position.x + pBounds.size.x / 2.f, pBounds.position.y + pBounds.size.y / 2.f});
    promptText.setPosition({Config::kViewWidth / 2.f, Config::kViewHeight * 0.80f});
}

void LevelCompleteState::handleInput(const sf::Event& event) {
    if (const auto* keyPressed = event.getIf<sf::Event::KeyPressed>()) {
        if (keyPressed->code == sf::Keyboard::Key::Enter) {
            if (progress.currentLevel >= Config::kFinalLevel) {
                gsm.changeState(std::make_unique<IntroMenuState>(gsm, assets));
            } else {
                progress.currentLevel++;
                gsm.changeState(std::make_unique<RespawnState>(gsm, assets, progress));
            }
        } else if (keyPressed->code == sf::Keyboard::Key::S && progress.currentLevel < Config::kFinalLevel) {
            progress.currentLevel++;
            if (SaveManager::saveProgress("savegame.txt", progress)) {
                std::cout << "[Core Engine] Progress saved. Returning to Main Menu.\n";
                gsm.changeState(std::make_unique<IntroMenuState>(gsm, assets));
            } else {
                --progress.currentLevel;
                std::cerr << "[Core Engine] Save failed; staying on Level Complete screen.\n";
            }
        }
    }
}

void LevelCompleteState::update(sf::Time dt) {
}

void LevelCompleteState::render(sf::RenderWindow& window) {
    window.draw(bgShape);
    window.draw(titleText);
    window.draw(statsText);
    window.draw(promptText);
}
