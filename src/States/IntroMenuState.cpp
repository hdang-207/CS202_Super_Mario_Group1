#include "States/IntroMenuState.hpp"
#include "Core/Config.hpp"
#include "States/CharacterSelectionState.hpp"
#include "States/GameModeSelectionState.hpp"
#include "States/PlayState.hpp"
#include "States/GameStateManager.hpp"
#include "Systems/ResourcePath.hpp"
#include "Systems/SaveManager.hpp"
#include "Systems/SoundController.hpp"
#include <algorithm>
#include <iostream>
#include <cmath>

IntroMenuState::IntroMenuState(GameStateManager& gsm, Systems::AssetManager& assets) 
    : State(gsm, assets), bgSprite(assets.getTexture("MenuBackground")),
      titleText(assets.getFont("MarioFont")), promptText(assets.getFont("MarioFont")),
      promptHintText(assets.getFont("MarioFont")) {}

void IntroMenuState::init() {
    std::cout << "[Core Engine] IntroMenuState Initialized.\n";

    // Scale background image to fill screen while preserving aspect ratio
    sf::Vector2f bgSize(bgSprite.getTexture().getSize());
    float bgScale = std::max(Config::kViewWidth / bgSize.x, Config::kViewHeight / bgSize.y);
    bgSprite.setScale({bgScale, bgScale});
    bgSprite.setPosition({(Config::kViewWidth - bgSize.x * bgScale) / 2.f,
                          (Config::kViewHeight - bgSize.y * bgScale) / 2.f});

    titleText.setString("SUPER MARIO BROS");
    titleText.setCharacterSize(80); // Make it larger
    titleText.setFillColor(sf::Color::White); // Base color white to let texture show
    titleText.setOutlineColor(sf::Color::Black); // Black outline
    titleText.setOutlineThickness(4.f);
    
    // Make text narrower (scale X slightly)
    titleText.setScale({0.7f, 1.0f});

    sf::FloatRect titleBounds = titleText.getLocalBounds();
    // Create render texture to fit the scaled text plus outline
    float scaledWidth = titleBounds.size.x * 0.7f;
    float scaledHeight = titleBounds.size.y * 1.0f;
    float outlinePadding = 10.f; // Extra padding for outline
    if (titleRT.resize({static_cast<unsigned int>(scaledWidth + outlinePadding * 2.f), 
                        static_cast<unsigned int>(scaledHeight + outlinePadding * 2.f)})) {
        
        // Setup text origin and position within the RT
        titleText.setOrigin({titleBounds.position.x, titleBounds.position.y});
        titleText.setPosition({outlinePadding, outlinePadding});
        
        titleRT.clear(sf::Color::Transparent);
        titleRT.draw(titleText);

        // Blend the silver texture over the text
        sf::Sprite silverSprite(assets.getTexture("SilverBackground"));
        silverSprite.setOrigin({0.f, 0.f});
        silverSprite.setPosition({0.f, 0.f});
        
        // Scale silver texture to cover the text
        sf::Vector2f silverSize(silverSprite.getTexture().getSize());
        float sScale = std::max((scaledWidth + outlinePadding * 2.f) / silverSize.x, 
                                (scaledHeight + outlinePadding * 2.f) / silverSize.y);
        silverSprite.setScale({sScale, sScale});
        
        sf::RenderStates blendState;
        blendState.blendMode = sf::BlendMultiply;
        titleRT.draw(silverSprite, blendState);
        titleRT.display();

        titleSprite.emplace(titleRT.getTexture());
        // Center the resulting sprite
        titleSprite->setOrigin({titleSprite->getLocalBounds().size.x / 2.f, 
                               titleSprite->getLocalBounds().size.y / 2.f});
    }

    // Position the title sprite slightly higher to avoid overlapping start button
    if (titleSprite.has_value()) {
        titleSprite->setPosition({Config::kViewWidth / 2.f, Config::kViewHeight * 0.20f});
    }

    // Frame 1 Prompt Text
    promptText.setString("PRESS ENTER / ANY KEY TO START");
    promptText.setCharacterSize(22);
    promptText.setFillColor(sf::Color::Yellow);
    promptText.setOutlineColor(sf::Color::Black);
    promptText.setOutlineThickness(2.f);

    sf::FloatRect promptBounds = promptText.getLocalBounds();
    promptText.setOrigin({promptBounds.position.x + promptBounds.size.x / 2.f,
                          promptBounds.position.y + promptBounds.size.y / 2.f});
    promptText.setPosition({Config::kViewWidth / 2.f, Config::kViewHeight * 0.65f});

    // Frame 2 Modal Overlay
    modalOverlay.setSize({Config::kViewWidth, Config::kViewHeight});
    modalOverlay.setFillColor(sf::Color(0, 0, 0, 175)); // Translucent dark overlay

    // Frame 2 Hint Text
    promptHintText.setString("UP/DOWN: SELECT | LEFT/RIGHT: ADJUST | ENTER: CONFIRM | ESC: BACK");
    promptHintText.setCharacterSize(14);
    promptHintText.setFillColor(sf::Color::White);
    promptHintText.setOutlineColor(sf::Color::Black);
    promptHintText.setOutlineThickness(1.5f);
    sf::FloatRect hBounds = promptHintText.getLocalBounds();
    promptHintText.setOrigin({hBounds.position.x + hBounds.size.x / 2.f, hBounds.position.y + hBounds.size.y / 2.f});
    promptHintText.setPosition({Config::kViewWidth / 2.f, Config::kViewHeight * 0.90f});

    menuLabels = {
        "START NEW GAME",
        "LOAD GAME PROGRESS",
        "BGM VOLUME",
        "SFX VOLUME",
        "ALLOW CHEATS",
        "EXIT GAME"
    };

    menuTexts.clear();
    for (size_t i = 0; i < menuLabels.size(); ++i) {
        sf::Text text(assets.getFont("MarioFont"), "", 22);
        text.setOutlineColor(sf::Color::Black);
        text.setOutlineThickness(2.f);
        menuTexts.push_back(text);
    }

    refreshSaveStatus();
    updateMenuLabels();

    Systems::SoundController::getInstance().playMusic(Systems::resourcePath("assets/audio/Theme.mp3"));
}

void IntroMenuState::refreshSaveStatus() {
    hasSaveAvailable = SaveManager::hasSaveFile("savegame.txt");
}

void IntroMenuState::updateMenuLabels() {
    float bgmVol = Systems::SoundController::getInstance().getMusicVolume();
    float sfxVol = Systems::SoundController::getInstance().getSoundVolume();

    for (size_t i = 0; i < menuLabels.size(); ++i) {
        std::string displayText = menuLabels[i];
        if (i == 1) { // Load Game Progress
            // if (!hasSaveAvailable) {
            //     displayText += " [NO SAVE]";
            // }
        } else if (i == 2) { // BGM Volume
            displayText += ": < " + std::to_string(static_cast<int>(std::round(bgmVol))) + "% >";
        } else if (i == 3) { // SFX Volume
            displayText += ": < " + std::to_string(static_cast<int>(std::round(sfxVol))) + "% >";
        } else if (i == 4) { // ALLOW CHEATS
            displayText += std::string(": < ") + (gsm.isCheatsEnabled() ? "ON" : "OFF") + " >";
        }

        menuTexts[i].setString(displayText);
        if (static_cast<int>(i) == selectedIndex) {
            menuTexts[i].setFillColor(sf::Color::Yellow);
            menuTexts[i].setString("> " + displayText + " <");
        } else {
            if (i == 1 && !hasSaveAvailable) {
                menuTexts[i].setFillColor(sf::Color(160, 160, 160)); // Gray out if no save
            } else {
                menuTexts[i].setFillColor(sf::Color::White);
            }
        }

        sf::FloatRect bounds = menuTexts[i].getLocalBounds();
        menuTexts[i].setOrigin({bounds.position.x + bounds.size.x / 2.f, bounds.position.y + bounds.size.y / 2.f});
        menuTexts[i].setPosition({Config::kViewWidth / 2.f, Config::kViewHeight * 0.42f + i * 45.f});
    }
}

void IntroMenuState::handleInput(const sf::Event& event) {
    const auto* keyPressed = event.getIf<sf::Event::KeyPressed>();
    if (!keyPressed) {
        return;
    }

    sf::Keyboard::Scancode key = keyPressed->scancode;

    // Ignores system keys
    if (key == sf::Keyboard::Scancode::Tab ||
        key == sf::Keyboard::Scancode::CapsLock ||
        key == sf::Keyboard::Scancode::LShift || key == sf::Keyboard::Scancode::RShift ||
        key == sf::Keyboard::Scancode::LControl || key == sf::Keyboard::Scancode::RControl ||
        key == sf::Keyboard::Scancode::LAlt || key == sf::Keyboard::Scancode::RAlt ||
        key == sf::Keyboard::Scancode::LSystem || key == sf::Keyboard::Scancode::RSystem ||
        (key >= sf::Keyboard::Scancode::F1 && key <= sf::Keyboard::Scancode::F12)) {
        return;
    }

    // --- FRAME 1: Title Screen ---
    if (currentPhase == MenuPhase::TitleScreen) {
        if (key == sf::Keyboard::Scancode::Escape) {
            return;
        }
        Systems::SoundController::getInstance().playSound(assets.getSoundBuffer("SelectSound"));
        currentPhase = MenuPhase::ActionMenu;
        refreshSaveStatus();
        updateMenuLabels();
        return;
    }

    // --- FRAME 2: Action Menu & Settings ---
    if (key == sf::Keyboard::Scancode::Escape || key == sf::Keyboard::Scancode::Backspace) {
        Systems::SoundController::getInstance().playSound(assets.getSoundBuffer("SelectSound"));
        currentPhase = MenuPhase::TitleScreen;
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
        } else if (selectedIndex == 4) { // ALLOW CHEATS
            gsm.setCheatsEnabled(!gsm.isCheatsEnabled());
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
        } else if (selectedIndex == 4) { // ALLOW CHEATS
            gsm.setCheatsEnabled(!gsm.isCheatsEnabled());
            Systems::SoundController::getInstance().playSound(assets.getSoundBuffer("SelectSound"));
            updateMenuLabels();
        }
    } else if (key == sf::Keyboard::Scancode::Enter || key == sf::Keyboard::Scancode::Space) {
        if (selectedIndex == 0) { // START NEW GAME
            Systems::SoundController::getInstance().playSound(assets.getSoundBuffer("SelectSound"));
            gsm.changeState(std::make_unique<GameModeSelectionState>(gsm, assets));
        } else if (selectedIndex == 1) { // LOAD GAME PROGRESS
            SaveData data;
            if (SaveManager::loadProgress("savegame.txt", data)) {
                Systems::SoundController::getInstance().playSound(assets.getSoundBuffer("SelectSound"));
                std::cout << "[Core Engine] Loaded saved game successfully! Launching PlayState...\n";
                gsm.changeState(std::make_unique<PlayState>(gsm, assets, data));
            } else {
                Systems::SoundController::getInstance().playSound(assets.getSoundBuffer("BrickCollision"));
                std::cout << "[Core Engine] No valid save file found or failed to load.\n";
            }
        } else if (selectedIndex == 4) { // ALLOW CHEATS
            gsm.setCheatsEnabled(!gsm.isCheatsEnabled());
            Systems::SoundController::getInstance().playSound(assets.getSoundBuffer("SelectSound"));
            updateMenuLabels();
        } else if (selectedIndex == 5) { // EXIT GAME
            Systems::SoundController::getInstance().playSound(assets.getSoundBuffer("SelectSound"));
            std::cout << "[IntroMenuState] Exiting game...\n";
            if (gsm.getWindow()) {
                gsm.getWindow()->close();
            } else {
                gsm.clearStates();
            }
        }
    }
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
    if (titleSprite.has_value()) window.draw(*titleSprite);

    if (currentPhase == MenuPhase::TitleScreen) {
        if (showPrompt) {
            window.draw(promptText);
        }
    } else {
        window.draw(modalOverlay);
        if (titleSprite.has_value()) window.draw(*titleSprite); // Draw title on top of overlay
        for (const auto& text : menuTexts) {
            window.draw(text);
        }
        window.draw(promptHintText);
    }
}
