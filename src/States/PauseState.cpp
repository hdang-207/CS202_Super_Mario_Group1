#include "States/PauseState.hpp"
#include "States/PlayState.hpp"
#include "States/IntroMenuState.hpp"
#include "States/GameStateManager.hpp"
#include "Systems/SaveManager.hpp"
#include "Systems/SoundController.hpp"
#include "Core/Config.hpp"
#include "Core/EventSystem.hpp"
#include <iostream>
#include <cmath>

PauseState::PauseState(GameStateManager& gsm, Systems::AssetManager& assets, PlayState& playStateRef)
    : State(gsm, assets), playState(playStateRef), titleText(assets.getFont("MarioFont")),
      promptHintText(assets.getFont("MarioFont")) {}

void PauseState::init() {
    std::cout << "[Core Engine] PauseState Initialized.\n";

    bgOverlay.setSize({Config::kViewWidth, Config::kViewHeight});
    bgOverlay.setFillColor(sf::Color(0, 0, 0, 180)); // Translucent black overlay

    titleText.setString("PAUSED");
    titleText.setCharacterSize(44);
    titleText.setFillColor(sf::Color::Yellow);
    titleText.setOutlineColor(sf::Color::Black);
    titleText.setOutlineThickness(3.f);
    sf::FloatRect tBounds = titleText.getLocalBounds();
    titleText.setOrigin({tBounds.position.x + tBounds.size.x / 2.f, tBounds.position.y + tBounds.size.y / 2.f});
    titleText.setPosition({Config::kViewWidth / 2.f, Config::kViewHeight * 0.15f});

    promptHintText.setString("UP/DOWN: SELECT | LEFT/RIGHT: ADJUST | ENTER: CONFIRM | ESC: RESUME");
    promptHintText.setCharacterSize(14);
    promptHintText.setFillColor(sf::Color::White);
    promptHintText.setOutlineColor(sf::Color::Black);
    promptHintText.setOutlineThickness(1.5f);
    sf::FloatRect hBounds = promptHintText.getLocalBounds();
    promptHintText.setOrigin({hBounds.position.x + hBounds.size.x / 2.f, hBounds.position.y + hBounds.size.y / 2.f});
    promptHintText.setPosition({Config::kViewWidth / 2.f, Config::kViewHeight * 0.90f});

    menuLabels = {
        "RESUME GAME",
        "RESTART LEVEL",
        "BGM VOLUME",
        "SFX VOLUME",
        "SAVE PROGRESS",
        "SAVE & EXIT TO MAIN MENU",
        "EXIT TO MAIN MENU"
    };

    menuTexts.clear();
    for (size_t i = 0; i < menuLabels.size(); ++i) {
        sf::Text text(assets.getFont("MarioFont"), "", 20);
        text.setOutlineColor(sf::Color::Black);
        text.setOutlineThickness(2.f);
        menuTexts.push_back(text);
    }

    updateMenuLabels();

    saveNoticeText.emplace(assets.getFont("MarioFont"), "GAME SAVED!", 48);
    saveNoticeText->setFillColor(sf::Color::Green);
    saveNoticeText->setOutlineColor(sf::Color::Black);
    saveNoticeText->setOutlineThickness(4.f);
    sf::FloatRect sBounds = saveNoticeText->getLocalBounds();
    saveNoticeText->setOrigin({sBounds.position.x + sBounds.size.x / 2.f, sBounds.position.y + sBounds.size.y / 2.f});
    saveNoticeText->setPosition({Config::kViewWidth / 2.f, Config::kViewHeight / 2.f});

    saveNoticeBg.emplace();
    saveNoticeBg->setSize({sBounds.size.x + 80.f, sBounds.size.y + 60.f});
    saveNoticeBg->setFillColor(sf::Color(0, 0, 0, 200));
    saveNoticeBg->setOutlineColor(sf::Color::Green);
    saveNoticeBg->setOutlineThickness(2.f);
    saveNoticeBg->setOrigin({saveNoticeBg->getSize().x / 2.f, saveNoticeBg->getSize().y / 2.f});
    saveNoticeBg->setPosition({Config::kViewWidth / 2.f, Config::kViewHeight / 2.f});
}

void PauseState::updateMenuLabels() {
    float bgmVol = Systems::SoundController::getInstance().getMusicVolume();
    float sfxVol = Systems::SoundController::getInstance().getSoundVolume();

    for (size_t i = 0; i < menuLabels.size(); ++i) {
        std::string displayText = menuLabels[i];
        if (i == 2) { // BGM Volume
            displayText += ": < " + std::to_string(static_cast<int>(std::round(bgmVol))) + "% >";
        } else if (i == 3) { // SFX Volume
            displayText += ": < " + std::to_string(static_cast<int>(std::round(sfxVol))) + "% >";
        }

        menuTexts[i].setString(displayText);
        if (static_cast<int>(i) == selectedIndex) {
            menuTexts[i].setFillColor(sf::Color::Yellow);
            menuTexts[i].setString("> " + displayText + " <");
        } else {
            menuTexts[i].setFillColor(sf::Color::White);
        }

        sf::FloatRect bounds = menuTexts[i].getLocalBounds();
        menuTexts[i].setOrigin({bounds.position.x + bounds.size.x / 2.f, bounds.position.y + bounds.size.y / 2.f});
        menuTexts[i].setPosition({Config::kViewWidth / 2.f, Config::kViewHeight * 0.28f + i * 38.f});
    }
}

void PauseState::handleInput(const sf::Event& event) {
    const auto* keyPressed = event.getIf<sf::Event::KeyPressed>();
    if (!keyPressed) {
        return;
    }

    const auto key = keyPressed->scancode;

    if (key == sf::Keyboard::Scancode::Escape || key == sf::Keyboard::Scancode::P) {
        gsm.popState();
        return;
    }

    if (key == sf::Keyboard::Scancode::Up || key == sf::Keyboard::Scancode::W) {
        selectedIndex = (selectedIndex - 1 + static_cast<int>(menuLabels.size())) % static_cast<int>(menuLabels.size());
        Systems::SoundController::getInstance().playSound(assets.getSoundBuffer("SelectSound"));
        updateMenuLabels();
    } else if (key == sf::Keyboard::Scancode::Down || key == sf::Keyboard::Scancode::S) {
        selectedIndex = (selectedIndex + 1) % static_cast<int>(menuLabels.size());
        Systems::SoundController::getInstance().playSound(assets.getSoundBuffer("SelectSound"));
        updateMenuLabels();
    } else if (key == sf::Keyboard::Scancode::Left || key == sf::Keyboard::Scancode::A) {
        if (selectedIndex == 2) { // BGM Volume
            float vol = std::max(0.f, Systems::SoundController::getInstance().getMusicVolume() - 10.f);
            Systems::SoundController::getInstance().setMusicVolume(vol);
            Systems::SoundController::getInstance().playSound(assets.getSoundBuffer("SelectSound"));
            updateMenuLabels();
        } else if (selectedIndex == 3) { // SFX Volume
            float vol = std::max(0.f, Systems::SoundController::getInstance().getSoundVolume() - 10.f);
            Systems::SoundController::getInstance().setSoundVolume(vol);
            Systems::SoundController::getInstance().playSound(assets.getSoundBuffer("SelectSound"));
            updateMenuLabels();
        }
    } else if (key == sf::Keyboard::Scancode::Right || key == sf::Keyboard::Scancode::D) {
        if (selectedIndex == 2) { // BGM Volume
            float vol = std::min(100.f, Systems::SoundController::getInstance().getMusicVolume() + 10.f);
            Systems::SoundController::getInstance().setMusicVolume(vol);
            Systems::SoundController::getInstance().playSound(assets.getSoundBuffer("SelectSound"));
            updateMenuLabels();
        } else if (selectedIndex == 3) { // SFX Volume
            float vol = std::min(100.f, Systems::SoundController::getInstance().getSoundVolume() + 10.f);
            Systems::SoundController::getInstance().setSoundVolume(vol);
            Systems::SoundController::getInstance().playSound(assets.getSoundBuffer("SelectSound"));
            updateMenuLabels();
        }
    } else if (key == sf::Keyboard::Scancode::Enter || key == sf::Keyboard::Scancode::Space) {
        Systems::SoundController::getInstance().playSound(assets.getSoundBuffer("SelectSound"));
        switch (selectedIndex) {
            case 0: // RESUME GAME
                gsm.popState();
                break;
            case 1: // RESTART LEVEL
                playState.restartCurrentLevel();
                gsm.popState();
                break;
            case 2: // BGM Volume
            case 3: // SFX Volume
                break;
            case 4: // SAVE PROGRESS
                if (SaveManager::saveProgress("savegame.txt", playState.getSaveData())) {
                    saveNoticeTimer = 1.5f;
                    std::cout << "[PauseState] Game progress saved successfully.\n";
                }
                break;
            case 5: // SAVE & EXIT TO MAIN MENU
                SaveManager::saveProgress("savegame.txt", playState.getSaveData());
                std::cout << "[PauseState] Saved progress and exiting to main menu...\n";
                gsm.changeState(std::make_unique<IntroMenuState>(gsm, assets));
                break;
            case 6: // EXIT TO MAIN MENU
                std::cout << "[PauseState] Exiting to main menu without saving...\n";
                gsm.changeState(std::make_unique<IntroMenuState>(gsm, assets));
                break;
        }
    }
}

void PauseState::update(sf::Time dt) {
    if (saveNoticeTimer > 0.f) {
        saveNoticeTimer -= dt.asSeconds();
    }
}

void PauseState::render(sf::RenderWindow& window) {
    window.draw(bgOverlay);
    window.draw(titleText);
    window.draw(promptHintText);

    for (const auto& text : menuTexts) {
        window.draw(text);
    }

    if (saveNoticeTimer > 0.f) {
        if (saveNoticeBg) window.draw(*saveNoticeBg);
        if (saveNoticeText) window.draw(*saveNoticeText);
    }
}
