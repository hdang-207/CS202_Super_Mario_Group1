#include "States/VictoryState.hpp"
#include "States/IntroMenuState.hpp"
#include "States/PlayState.hpp"
#include "States/GameStateManager.hpp"
#include "Systems/SaveManager.hpp"
#include "Core/Config.hpp"
#include <iostream>
#include <string>
#include "Systems/SoundController.hpp"

VictoryState::VictoryState(GameStateManager& gsm, Systems::AssetManager& assets, const SaveData& data)
    : State(gsm, assets), victoryText(assets.getFont("MarioFont")), promptText(assets.getFont("MarioFont")), 
      statsText(assets.getFont("MarioFont")), progress(data) {}

void VictoryState::init() {
    std::cout << "[Core Engine] VictoryState Initialized.\n";

    Systems::SoundController::getInstance().stopMusic();
    Systems::SoundController::getInstance().playSound(assets.getSoundBuffer("VictorySound"));

    bgShape.setSize({Config::kViewWidth, Config::kViewHeight});
    bgShape.setFillColor(sf::Color(255, 200, 0)); // Yellow background

    const bool gameComplete = progress.currentLevel >= Config::kFinalLevel;
    std::string title = gameComplete
        ? "YOU WIN THE GAME!"
        : "WORLD " + std::to_string(Config::worldNumber(progress.currentLevel))
            + "-" + std::to_string(Config::stageNumber(progress.currentLevel)) + " CLEAR!";
    victoryText.setString(title);
    victoryText.setCharacterSize(48);
    victoryText.setFillColor(sf::Color::White);
    victoryText.setOutlineColor(sf::Color::Black);
    victoryText.setOutlineThickness(3.f);
    sf::FloatRect bounds = victoryText.getLocalBounds();
    victoryText.setOrigin({bounds.position.x + bounds.size.x / 2.f, bounds.position.y + bounds.size.y / 2.f});
    victoryText.setPosition({Config::kViewWidth / 2.f, Config::kViewHeight * 0.3f});

    std::string stats = "SCORE: " + std::to_string(progress.score) + "\n" +
                        "COINS: " + std::to_string(progress.coins) + "\n" +
                        "LIVES: " + std::to_string(progress.lives);
    statsText.setString(stats);
    statsText.setCharacterSize(24);
    statsText.setFillColor(sf::Color::White);
    statsText.setOutlineColor(sf::Color::Black);
    statsText.setOutlineThickness(2.f);
    sf::FloatRect sBounds = statsText.getLocalBounds();
    statsText.setOrigin({sBounds.position.x + sBounds.size.x / 2.f, sBounds.position.y + sBounds.size.y / 2.f});
    statsText.setPosition({Config::kViewWidth / 2.f, Config::kViewHeight * 0.5f});

    std::string prompt = gameComplete
        ? "PRESS ENTER TO MENU"
        : "PRESS ENTER TO NEXT LEVEL\nPRESS S TO SAVE & MENU";
    promptText.setString(prompt);
    promptText.setCharacterSize(20);
    promptText.setFillColor(sf::Color::Black);
    sf::FloatRect pBounds = promptText.getLocalBounds();
    promptText.setOrigin({pBounds.position.x + pBounds.size.x / 2.f, pBounds.position.y + pBounds.size.y / 2.f});
    promptText.setPosition({Config::kViewWidth / 2.f, Config::kViewHeight * 0.75f});
}

void VictoryState::handleInput(const sf::Event& event) {
    if (const auto* keyPressed = event.getIf<sf::Event::KeyPressed>()) {
        const auto code = keyPressed->scancode;
        if (code == sf::Keyboard::Scancode::Enter) {
            if (progress.currentLevel >= Config::kFinalLevel) {
                gsm.changeState(std::make_unique<IntroMenuState>(gsm, assets));
            } else {
                progress.currentLevel++;
                gsm.changeState(std::make_unique<PlayState>(gsm, assets, progress));
            }
        } else if (code == sf::Keyboard::Scancode::S
                   && progress.currentLevel < Config::kFinalLevel) {
            // Save and return to menu
            progress.currentLevel++; // Save next level's progress
            if (SaveManager::saveToFile("savegame.txt", progress)) {
                std::cout << "[Core Engine] Progress saved. Returning to Menu.\n";
                gsm.changeState(std::make_unique<IntroMenuState>(gsm, assets));
            } else {
                --progress.currentLevel;
                std::cerr << "[Core Engine] Save failed; staying on Victory screen.\n";
            }
        }
    }
}

void VictoryState::update(sf::Time) {
}

void VictoryState::render(sf::RenderWindow& window) {
    window.draw(bgShape);
    window.draw(victoryText);
    window.draw(statsText);
    window.draw(promptText);
}
