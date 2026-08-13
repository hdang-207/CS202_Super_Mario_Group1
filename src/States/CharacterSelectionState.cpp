#include "States/CharacterSelectionState.hpp"
#include "Core/Config.hpp"
#include "States/IntroMenuState.hpp"
#include "States/PlayState.hpp"
#include "States/WorldSelectionState.hpp"
#include "States/GameStateManager.hpp"
#include "Core/CharacterType.hpp"
#include "Systems/SoundController.hpp"
#include "Systems/SaveManager.hpp"
#include <iostream>
#include <cmath>

CharacterSelectionState::CharacterSelectionState(GameStateManager& gsm, Systems::AssetManager& assets) 
    : State(gsm, assets),
      bgSprite(assets.getTexture("MenuBackground")),
      headerText(assets.getFont("MarioFont")),
      marioOptionText(assets.getFont("MarioFont")),
      luigiOptionText(assets.getFont("MarioFont")),
      descText(assets.getFont("MarioFont")),
      backHintText(assets.getFont("MarioFont")),
      previewSprite(assets.getTexture("MarioPreview")),
      musicIconSprite(assets.getTexture("MusicSymbol")),
      soundIconSprite(assets.getTexture("SoundSymbol")) {}

void CharacterSelectionState::init() {
    std::cout << "[Core Engine] CharacterSelectionState Initialized.\n";

    // Scale menu background image to fill screen
    sf::Vector2f bgSize(bgSprite.getTexture().getSize());
    float bgScale = std::max(Config::kViewWidth / bgSize.x, Config::kViewHeight / bgSize.y);
    bgSprite.setScale({bgScale, bgScale});
    bgSprite.setPosition({(Config::kViewWidth - bgSize.x * bgScale) / 2.f,
                          (Config::kViewHeight - bgSize.y * bgScale) / 2.f});

    // Dark overlay rectangle to increase contrast for text and preview sprites
    darkOverlay.setSize({Config::kViewWidth, Config::kViewHeight});
    darkOverlay.setFillColor(sf::Color(0, 0, 0, 160));

    headerText.setString("SELECT YOUR CHARACTER");
    headerText.setCharacterSize(32);
    headerText.setFillColor(sf::Color::Yellow);
    headerText.setOutlineColor(sf::Color::Black);
    headerText.setOutlineThickness(2.f);
    sf::FloatRect hb = headerText.getLocalBounds();
    headerText.setOrigin({hb.position.x + hb.size.x / 2.f, hb.position.y + hb.size.y / 2.f});
    headerText.setPosition({Config::kViewWidth / 2.f, Config::kViewHeight * 0.12f});

    // Outer card container for previewing character
    previewCard.setSize({140.f, 160.f});
    previewCard.setOrigin({70.f, 80.f});
    previewCard.setPosition({Config::kViewWidth / 2.f, Config::kViewHeight * 0.38f});
    previewCard.setFillColor(sf::Color(20, 20, 20, 220));
    previewCard.setOutlineThickness(3.f);
    previewCard.setOutlineColor(sf::Color::White);

    descText.setCharacterSize(16);
    descText.setFillColor(sf::Color::White);
    descText.setOutlineColor(sf::Color::Black);
    descText.setOutlineThickness(2.f);

    marioOptionText.setCharacterSize(22);
    marioOptionText.setOutlineColor(sf::Color::Black);
    marioOptionText.setOutlineThickness(2.f);

    luigiOptionText.setCharacterSize(22);
    luigiOptionText.setOutlineColor(sf::Color::Black);
    luigiOptionText.setOutlineThickness(2.f);

    backHintText.setString("B: MENU | ENTER/SPACE: NEXT | L: LOAD SAVE\nM: TOGGLE MUSIC | N: TOGGLE SOUND");
    backHintText.setCharacterSize(13);
    backHintText.setFillColor(sf::Color::White);
    backHintText.setOutlineColor(sf::Color::Black);
    backHintText.setOutlineThickness(2.f);
    sf::FloatRect bb = backHintText.getLocalBounds();
    backHintText.setOrigin({bb.position.x + bb.size.x / 2.f, bb.position.y + bb.size.y / 2.f});
    backHintText.setPosition({Config::kViewWidth / 2.f, Config::kViewHeight * 0.90f});

    // Configure Music & Sound Icon Sprites in Bottom Left
    float iconTargetSize = 36.f;
    sf::FloatRect mbounds = musicIconSprite.getLocalBounds();
    if (mbounds.size.y > 0.f) {
        float scaleM = iconTargetSize / mbounds.size.y;
        musicIconSprite.setScale({scaleM, scaleM});
    }
    musicIconSprite.setPosition({30.f, Config::kViewHeight - 55.f});

    sf::FloatRect sbounds = soundIconSprite.getLocalBounds();
    if (sbounds.size.y > 0.f) {
        float scaleS = iconTargetSize / sbounds.size.y;
        soundIconSprite.setScale({scaleS, scaleS});
    }
    soundIconSprite.setPosition({80.f, Config::kViewHeight - 55.f});
}

void CharacterSelectionState::handleInput(const sf::Event& event) {
    if (const auto* keyPressed = event.getIf<sf::Event::KeyPressed>()) {
        if (keyPressed->code == sf::Keyboard::Key::Up || keyPressed->code == sf::Keyboard::Key::W ||
            keyPressed->code == sf::Keyboard::Key::Num1 || keyPressed->code == sf::Keyboard::Key::Numpad1) {
            selectedIndex = 0; // Select Mario
            Systems::SoundController::getInstance().playSound(assets.getSoundBuffer("SelectSound"));
        } 
        else if (keyPressed->code == sf::Keyboard::Key::Down || keyPressed->code == sf::Keyboard::Key::S ||
                 keyPressed->code == sf::Keyboard::Key::Num2 || keyPressed->code == sf::Keyboard::Key::Numpad2) {
            selectedIndex = 1; // Select Luigi
            Systems::SoundController::getInstance().playSound(assets.getSoundBuffer("SelectSound"));
        }
        else if (keyPressed->code == sf::Keyboard::Key::Enter || keyPressed->code == sf::Keyboard::Key::Space) {
            Systems::SoundController::getInstance().playSound(assets.getSoundBuffer("SelectSound"));
            CharacterType chosen = (selectedIndex == 0) ? CharacterType::Mario : CharacterType::Luigi;
            std::cout << "[Core Engine] Character confirmed! Transitioning to WorldSelectionState...\n";
            gsm.changeState(std::make_unique<WorldSelectionState>(gsm, assets, chosen));
        }
        else if (keyPressed->code == sf::Keyboard::Key::B) {
            std::cout << "[Core Engine] Going back to IntroMenuState...\n";
            gsm.changeState(std::make_unique<IntroMenuState>(gsm, assets));
        }
        else if (keyPressed->code == sf::Keyboard::Key::L) {
            SaveData data;
            if (SaveManager::loadFromFile("savegame.txt", data)) {
                std::cout << "[Core Engine] Loading saved game at World "
                          << Config::worldNumber(data.currentLevel) << "-"
                          << Config::stageNumber(data.currentLevel) << "...\n";
                gsm.changeState(std::make_unique<PlayState>(gsm, assets, data));
            }
        }
        else if (keyPressed->code == sf::Keyboard::Key::M) {
            Systems::SoundController::getInstance().toggleMusicMuted();
        }
        else if (keyPressed->code == sf::Keyboard::Key::N) {
            Systems::SoundController::getInstance().toggleSoundMuted();
        }
    }
    else if (const auto* mousePressed = event.getIf<sf::Event::MouseButtonPressed>()) {
        if (mousePressed->button == sf::Mouse::Button::Left) {
            sf::Vector2f mousePos(static_cast<float>(mousePressed->position.x), static_cast<float>(mousePressed->position.y));
            if (gsm.getWindow() != nullptr) {
                // Map raw window pixels to exact UI view coordinates regardless of window size / maximization
                mousePos = gsm.getWindow()->mapPixelToCoords(mousePressed->position);
            }

            if (musicIconSprite.getGlobalBounds().contains(mousePos)) {
                Systems::SoundController::getInstance().toggleMusicMuted();
            }
            else if (soundIconSprite.getGlobalBounds().contains(mousePos)) {
                Systems::SoundController::getInstance().toggleSoundMuted();
            }
            else if (marioOptionText.getGlobalBounds().contains(mousePos)) {
                selectedIndex = 0;
            }
            else if (luigiOptionText.getGlobalBounds().contains(mousePos)) {
                selectedIndex = 1;
            }
        }
    }
}

void CharacterSelectionState::update(sf::Time) {
    // Map current mouse position for hover effects across resized/maximized windows
    sf::Vector2f mousePos(-9999.f, -9999.f);
    if (gsm.getWindow() != nullptr) {
        sf::Vector2i pixelPos = sf::Mouse::getPosition(*gsm.getWindow());
        mousePos = gsm.getWindow()->mapPixelToCoords(pixelPos);
    }

    // Hover effect for Music & Sound icons
    float baseSize = 36.f;
    float mbSize = musicIconSprite.getLocalBounds().size.y;
    if (mbSize > 0.f) {
        float scale = (baseSize / mbSize) * (musicIconSprite.getGlobalBounds().contains(mousePos) ? 1.18f : 1.0f);
        musicIconSprite.setScale({scale, scale});
    }

    float sbSize = soundIconSprite.getLocalBounds().size.y;
    if (sbSize > 0.f) {
        float scale = (baseSize / sbSize) * (soundIconSprite.getGlobalBounds().contains(mousePos) ? 1.18f : 1.0f);
        soundIconSprite.setScale({scale, scale});
    }

    if (selectedIndex == 0) {
        marioOptionText.setString("> 1. MARIO");
        marioOptionText.setFillColor(sf::Color::Yellow);

        luigiOptionText.setString("  2. LUIGI");
        luigiOptionText.setFillColor(sf::Color(180, 180, 180));

        previewSprite.setTexture(assets.getTexture("MarioPreview"), true);
        previewCard.setOutlineColor(sf::Color::Red);
        descText.setString("MARIO: Balanced Speed & Jump");
    } else {
        marioOptionText.setString("  1. MARIO");
        marioOptionText.setFillColor(sf::Color(180, 180, 180));

        luigiOptionText.setString("> 2. LUIGI");
        luigiOptionText.setFillColor(sf::Color::Yellow);

        previewSprite.setTexture(assets.getTexture("LuigiPreview"), true);
        previewCard.setOutlineColor(sf::Color::Green);
        descText.setString("LUIGI: High Jump & Low Friction");
    }

    // Align preview sprite in center and scale up (200px height target)
    sf::FloatRect sb = previewSprite.getLocalBounds();
    previewSprite.setOrigin({sb.position.x + sb.size.x / 2.f, sb.position.y + sb.size.y / 2.f});
    previewSprite.setPosition({Config::kViewWidth / 2.f, Config::kViewHeight * 0.35f});

    if (sb.size.y > 0.f) {
        float targetHeight = 200.f;
        float scale = targetHeight / sb.size.y;
        previewSprite.setScale({scale, scale});
    }

    sf::FloatRect mb = marioOptionText.getLocalBounds();
    marioOptionText.setOrigin({mb.position.x + mb.size.x / 2.f, mb.position.y + mb.size.y / 2.f});
    marioOptionText.setPosition({Config::kViewWidth / 2.f, Config::kViewHeight * 0.65f});

    sf::FloatRect lb = luigiOptionText.getLocalBounds();
    luigiOptionText.setOrigin({lb.position.x + lb.size.x / 2.f, lb.position.y + lb.size.y / 2.f});
    luigiOptionText.setPosition({Config::kViewWidth / 2.f, Config::kViewHeight * 0.74f});

    sf::FloatRect db = descText.getLocalBounds();
    descText.setOrigin({db.position.x + db.size.x / 2.f, db.position.y + db.size.y / 2.f});
    descText.setPosition({Config::kViewWidth / 2.f, Config::kViewHeight * 0.54f});
}

void CharacterSelectionState::render(sf::RenderWindow& window) {
    window.draw(bgSprite);
    window.draw(darkOverlay);

    window.draw(headerText);
    window.draw(previewSprite);
    window.draw(descText);
    window.draw(marioOptionText);
    window.draw(luigiOptionText);
    window.draw(backHintText);

    // Draw Music & Sound icons in bottom-left corner
    window.draw(musicIconSprite);
    window.draw(soundIconSprite);

    // Helper lambda to draw a red slash line over an icon when muted
    auto drawRedSlash = [&](const sf::FloatRect& bounds) {
        sf::RectangleShape slash;
        float lineLength = std::sqrt(bounds.size.x * bounds.size.x + bounds.size.y * bounds.size.y);
        slash.setSize({lineLength, 4.f});
        slash.setFillColor(sf::Color::Red);
        slash.setOrigin({0.f, 2.f});
        slash.setPosition({bounds.position.x, bounds.position.y + bounds.size.y});
        float angleDeg = -std::atan2(bounds.size.y, bounds.size.x) * 180.f / 3.14159265f;
        slash.setRotation(sf::degrees(angleDeg));
        window.draw(slash);
    };

    if (Systems::SoundController::getInstance().isMusicMuted()) {
        drawRedSlash(musicIconSprite.getGlobalBounds());
    }
    if (Systems::SoundController::getInstance().isSoundMuted()) {
        drawRedSlash(soundIconSprite.getGlobalBounds());
    }
}
