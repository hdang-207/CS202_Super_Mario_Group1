#include "States/LevelCompleteState.hpp"
#include "States/IntroMenuState.hpp"
#include "States/PlayState.hpp"
#include "States/RespawnState.hpp"
#include "States/VictoryState.hpp"
#include "States/GameStateManager.hpp"
#include "Systems/SaveManager.hpp"
#include "Systems/SoundController.hpp"
#include "Systems/CompletionTracker.hpp"
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

    const bool worldComplete = Config::isLastStageOfWorld(progress.currentLevel);
    std::string title = worldComplete
        ? "WORLD " + std::to_string(Config::worldNumber(progress.currentLevel))
            + " COMPLETE!"
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

    std::string prompt = worldComplete
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
    if (m_transitionRequested) {
        return;
    }

    // Prefer window events: unlike global hardware polling, they work on macOS
    // without granting the game Input Monitoring permission. Check both the
    // localized key and physical scancode so Return works on every layout.
    if (const auto* keyPressed = event.getIf<sf::Event::KeyPressed>()) {
        const auto code = keyPressed->code;
        const auto scancode = keyPressed->scancode;
        const bool continuePressed =
            code == sf::Keyboard::Key::Enter
            || scancode == sf::Keyboard::Scancode::Enter
            || scancode == sf::Keyboard::Scancode::NumpadEnter
            || scancode == sf::Keyboard::Scancode::Space
            || scancode == sf::Keyboard::Scancode::Z
            || scancode == sf::Keyboard::Scancode::X
            || scancode == sf::Keyboard::Scancode::C;

        if (continuePressed) {
            continueFromLevel();
        } else if ((code == sf::Keyboard::Key::S
                    || scancode == sf::Keyboard::Scancode::S)
                   && !Config::isLastStageOfWorld(progress.currentLevel)) {
            saveAndReturnToMenu();
        }
        return;
    }

    // Some macOS input methods expose Return as text input instead of a
    // KeyPressed event. Accept CR and LF as a final event-based fallback.
    if (const auto* textEntered = event.getIf<sf::Event::TextEntered>();
        textEntered != nullptr
        && (textEntered->unicode == U'\r' || textEntered->unicode == U'\n')) {
        continueFromLevel();
    }
}

void LevelCompleteState::update(sf::Time dt) {
    m_elapsedTime += dt.asSeconds();
    if (m_transitionRequested || m_elapsedTime < 0.5f) {
        return; // Prevent a held gameplay key from instantly skipping the screen
    }

    // Retain direct polling as a fallback for Windows IMEs such as UniKey.
    if (sf::Keyboard::isKeyPressed(sf::Keyboard::Scancode::Enter) ||
        sf::Keyboard::isKeyPressed(sf::Keyboard::Scancode::NumpadEnter) ||
        sf::Keyboard::isKeyPressed(sf::Keyboard::Scancode::Space) ||
        sf::Keyboard::isKeyPressed(sf::Keyboard::Scancode::Z) ||
        sf::Keyboard::isKeyPressed(sf::Keyboard::Scancode::X) ||
        sf::Keyboard::isKeyPressed(sf::Keyboard::Scancode::C)) {
        continueFromLevel();
    } else if (sf::Keyboard::isKeyPressed(sf::Keyboard::Scancode::S)
               && !Config::isLastStageOfWorld(progress.currentLevel)) {
        saveAndReturnToMenu();
    }
}

void LevelCompleteState::continueFromLevel() {
    if (m_transitionRequested) {
        return;
    }
    m_transitionRequested = true;

    if (Config::isLastStageOfWorld(progress.currentLevel)) {
        // Track completion for mode unlock
        if (progress.gameMode == GameMode::Nightfall
            || progress.gameMode == GameMode::Inferno
            || progress.gameMode == GameMode::Apocalypse) {
            Systems::CompletionTracker::getInstance().markWorldComplete(
                progress.gameMode, Config::worldNumber(progress.currentLevel));
        }
        gsm.changeState(std::make_unique<IntroMenuState>(gsm, assets));
    } else {
        progress.currentLevel = Config::nextLevel(progress.currentLevel);
        gsm.changeState(std::make_unique<RespawnState>(gsm, assets, progress));
    }
}

void LevelCompleteState::saveAndReturnToMenu() {
    if (m_transitionRequested
        || Config::isLastStageOfWorld(progress.currentLevel)) {
        return;
    }
    m_transitionRequested = true;

    const int clearedLevel = progress.currentLevel;
    progress.currentLevel = Config::nextLevel(progress.currentLevel);
    if (SaveManager::saveProgress("savegame.txt", progress)) {
        std::cout << "[Core Engine] Progress saved. Returning to Main Menu.\n";
        gsm.changeState(std::make_unique<IntroMenuState>(gsm, assets));
    } else {
        progress.currentLevel = clearedLevel;
        m_transitionRequested = false;
        std::cerr << "[Core Engine] Save failed; staying on Level Complete screen.\n";
    }
}

void LevelCompleteState::render(sf::RenderWindow& window) {
    window.draw(bgShape);
    window.draw(titleText);
    window.draw(statsText);
    window.draw(promptText);
}
