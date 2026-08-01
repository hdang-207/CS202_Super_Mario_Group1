#include "States/CharacterSelectionState.hpp"
#include "Core/Config.hpp"
#include "States/IntroMenuState.hpp"
#include "States/PlayState.hpp"
#include "States/GameStateManager.hpp"
#include "Core/CharacterType.hpp"
#include <iostream>

CharacterSelectionState::CharacterSelectionState(GameStateManager& gsm, Systems::AssetManager& assets) 
    : State(gsm, assets),
      bgSprite(assets.getTexture("MenuBackground")),
      headerText(assets.getFont("MarioFont")),
      marioOptionText(assets.getFont("MarioFont")),
      luigiOptionText(assets.getFont("MarioFont")),
      descText(assets.getFont("MarioFont")),
      backHintText(assets.getFont("MarioFont")),
      previewSprite(assets.getTexture("MarioPreview")) {}

void CharacterSelectionState::init() {
    std::cout << "[Core Engine] CharacterSelectionState Initialized.\n";

    // Phóng ảnh nền menu phủ kín màn hình
    sf::Vector2f bgSize(bgSprite.getTexture().getSize());
    float bgScale = std::max(Config::kViewWidth / bgSize.x, Config::kViewHeight / bgSize.y);
    bgSprite.setScale({bgScale, bgScale});
    bgSprite.setPosition({(Config::kViewWidth - bgSize.x * bgScale) / 2.f,
                          (Config::kViewHeight - bgSize.y * bgScale) / 2.f});

    // Lớp phủ màu đen làm tối hình nền (Dark Overlay) giúp chữ và ảnh nổi bật hơn
    darkOverlay.setSize({Config::kViewWidth, Config::kViewHeight});
    darkOverlay.setFillColor(sf::Color(0, 0, 0, 160)); // Độ tối 160/255

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

    backHintText.setString("PRESS 'B' TO GO BACK TO MENU | ENTER/SPACE TO CONFIRM");
    backHintText.setCharacterSize(14);
    backHintText.setFillColor(sf::Color::White);
    backHintText.setOutlineColor(sf::Color::Black);
    backHintText.setOutlineThickness(2.f);
    sf::FloatRect bb = backHintText.getLocalBounds();
    backHintText.setOrigin({bb.position.x + bb.size.x / 2.f, bb.position.y + bb.size.y / 2.f});
    backHintText.setPosition({Config::kViewWidth / 2.f, Config::kViewHeight * 0.90f});
}

void CharacterSelectionState::handleInput(const sf::Event& event) {
    if (const auto* keyPressed = event.getIf<sf::Event::KeyPressed>()) {
        if (keyPressed->code == sf::Keyboard::Key::Up || keyPressed->code == sf::Keyboard::Key::W ||
            keyPressed->code == sf::Keyboard::Key::Num1 || keyPressed->code == sf::Keyboard::Key::Numpad1) {
            selectedIndex = 0; // Select Mario
        } 
        else if (keyPressed->code == sf::Keyboard::Key::Down || keyPressed->code == sf::Keyboard::Key::S ||
                 keyPressed->code == sf::Keyboard::Key::Num2 || keyPressed->code == sf::Keyboard::Key::Numpad2) {
            selectedIndex = 1; // Select Luigi
        }
        else if (keyPressed->code == sf::Keyboard::Key::Enter || keyPressed->code == sf::Keyboard::Key::Space) {
            CharacterType chosen = (selectedIndex == 0) ? CharacterType::Mario : CharacterType::Luigi;
            std::cout << "[Core Engine] Character confirmed! Transitioning to PlayState...\n";
            gsm.changeState(std::make_unique<PlayState>(gsm, assets, chosen));
        }
        else if (keyPressed->code == sf::Keyboard::Key::B) {
            std::cout << "[Core Engine] Going back to IntroMenuState...\n";
            gsm.changeState(std::make_unique<IntroMenuState>(gsm, assets));
        }
    }
}

void CharacterSelectionState::update(sf::Time dt) {
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
}

