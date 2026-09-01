#include "States/GameModeSelectionState.hpp"
#include "Core/Config.hpp"
#include "States/CharacterSelectionState.hpp"
#include "States/DuelState.hpp"
#include "States/IntroMenuState.hpp"
#include "States/GameStateManager.hpp"
#include "Systems/SoundController.hpp"
#include <algorithm>
#include <iostream>

GameModeSelectionState::GameModeSelectionState(
    GameStateManager& gsm,
    Systems::AssetManager& assets,
    bool nightfallInitiallySelected
)
    : State(gsm, assets),
      selectedOption(nightfallInitiallySelected
          ? SelectionOption::Nightfall
          : SelectionOption::Normal),
      bgSprite(assets.getTexture("MenuBackground")),
      headerText(assets.getFont("MarioFont")),
      normalText(assets.getFont("MarioFont")),
      nightfallText(assets.getFont("MarioFont")),
      duelText(assets.getFont("MarioFont")),
      normalDesc(assets.getFont("MarioFont")),
      nightfallDesc(assets.getFont("MarioFont")),
      duelDesc(assets.getFont("MarioFont")),
      hintText(assets.getFont("MarioFont")) {}

void GameModeSelectionState::init() {
    std::cout << "[Core Engine] GameModeSelectionState Initialized.\n";

    // Background
    const sf::Vector2f bgSize(bgSprite.getTexture().getSize());
    const float bgScale = std::max(Config::kViewWidth / bgSize.x,
                                   Config::kViewHeight / bgSize.y);
    bgSprite.setScale({bgScale, bgScale});
    bgSprite.setPosition({(Config::kViewWidth - bgSize.x * bgScale) / 2.f,
                          (Config::kViewHeight - bgSize.y * bgScale) / 2.f});

    darkOverlay.setSize({Config::kViewWidth, Config::kViewHeight});
    darkOverlay.setFillColor(sf::Color(0, 0, 0, 175));

    // Header
    headerText.setString("SELECT GAME MODE");
    headerText.setCharacterSize(38);
    headerText.setFillColor(sf::Color::Yellow);
    headerText.setOutlineColor(sf::Color::Black);
    headerText.setOutlineThickness(3.f);
    const sf::FloatRect headerBounds = headerText.getLocalBounds();
    headerText.setOrigin({headerBounds.position.x + headerBounds.size.x / 2.f,
                          headerBounds.position.y + headerBounds.size.y / 2.f});
    headerText.setPosition({Config::kViewWidth / 2.f, Config::kViewHeight * 0.15f});

    // Options
    for (sf::Text* t : {&normalText, &nightfallText, &duelText}) {
        t->setCharacterSize(28);
        t->setOutlineColor(sf::Color::Black);
        t->setOutlineThickness(2.f);
    }

    // Descriptions
    for (sf::Text* t : {&normalDesc, &nightfallDesc, &duelDesc}) {
        t->setCharacterSize(16);
        t->setFillColor(sf::Color(200, 200, 200));
        t->setOutlineColor(sf::Color::Black);
        t->setOutlineThickness(2.f);
    }
    normalDesc.setString("Classic gameplay with full visibility");
    nightfallDesc.setString("Darkness surrounds you. Only a small light guides your way!");
    duelDesc.setString("Local Mario vs Luigi battle");

    // Hint
    hintText.setString(
        "UP/DOWN OR 1/2/3: SELECT  |  ENTER/SPACE: CONFIRM  |  B/ESC: BACK");
    hintText.setCharacterSize(15);
    hintText.setFillColor(sf::Color::White);
    hintText.setOutlineColor(sf::Color::Black);
    hintText.setOutlineThickness(2.f);
    const sf::FloatRect hintBounds = hintText.getLocalBounds();
    hintText.setOrigin({hintBounds.position.x + hintBounds.size.x / 2.f,
                        hintBounds.position.y + hintBounds.size.y / 2.f});
    hintText.setPosition({Config::kViewWidth / 2.f, Config::kViewHeight * 0.88f});
}

void GameModeSelectionState::selectPreviousOption() {
    switch (selectedOption) {
        case SelectionOption::Normal:
            chooseOption(SelectionOption::Duel);
            break;
        case SelectionOption::Nightfall:
            chooseOption(SelectionOption::Normal);
            break;
        case SelectionOption::Duel:
            chooseOption(SelectionOption::Nightfall);
            break;
    }
}

void GameModeSelectionState::selectNextOption() {
    switch (selectedOption) {
        case SelectionOption::Normal:
            chooseOption(SelectionOption::Nightfall);
            break;
        case SelectionOption::Nightfall:
            chooseOption(SelectionOption::Duel);
            break;
        case SelectionOption::Duel:
            chooseOption(SelectionOption::Normal);
            break;
    }
}

void GameModeSelectionState::chooseOption(SelectionOption option) {
    if (option == selectedOption) {
        return;
    }

    selectedOption = option;
    Systems::SoundController::getInstance().playSound(
        assets.getSoundBuffer("SelectSound"));
}

void GameModeSelectionState::confirmSelection() {
    Systems::SoundController::getInstance().playSound(
        assets.getSoundBuffer("SelectSound"));

    switch (selectedOption) {
        case SelectionOption::Normal:
            std::cout << "[Core Engine] Game mode selected: NORMAL\n";
            gsm.changeState(std::make_unique<CharacterSelectionState>(
                gsm, assets, false));
            break;
        case SelectionOption::Nightfall:
            std::cout << "[Core Engine] Game mode selected: NIGHTFALL\n";
            gsm.changeState(std::make_unique<CharacterSelectionState>(
                gsm, assets, true));
            break;
        case SelectionOption::Duel:
            std::cout << "[Core Engine] Game mode selected: DUEL\n";
            gsm.pushState(std::make_unique<DuelState>(gsm, assets));
            break;
    }
}

void GameModeSelectionState::handleInput(const sf::Event& event) {
    if (const auto* keyPressed = event.getIf<sf::Event::KeyPressed>()) {
        const auto key = keyPressed->scancode;
        if (key == sf::Keyboard::Scancode::Up || key == sf::Keyboard::Scancode::W) {
            selectPreviousOption();
        } else if (key == sf::Keyboard::Scancode::Down
                   || key == sf::Keyboard::Scancode::S) {
            selectNextOption();
        } else if (key == sf::Keyboard::Scancode::Num1
                   || key == sf::Keyboard::Scancode::Numpad1) {
            chooseOption(SelectionOption::Normal);
        } else if (key == sf::Keyboard::Scancode::Num2
                   || key == sf::Keyboard::Scancode::Numpad2) {
            chooseOption(SelectionOption::Nightfall);
        } else if (key == sf::Keyboard::Scancode::Num3
                   || key == sf::Keyboard::Scancode::Numpad3) {
            chooseOption(SelectionOption::Duel);
        } else if (key == sf::Keyboard::Scancode::Enter
                   || key == sf::Keyboard::Scancode::Space) {
            confirmSelection();
        } else if (key == sf::Keyboard::Scancode::B
                   || key == sf::Keyboard::Scancode::Escape) {
            Systems::SoundController::getInstance().playSound(
                assets.getSoundBuffer("SelectSound"));
            gsm.changeState(std::make_unique<IntroMenuState>(gsm, assets));
        }
        return;
    }

    const auto* mousePressed = event.getIf<sf::Event::MouseButtonPressed>();
    if (!mousePressed || mousePressed->button != sf::Mouse::Button::Left) {
        return;
    }

    sf::Vector2f mousePosition(
        static_cast<float>(mousePressed->position.x),
        static_cast<float>(mousePressed->position.y));
    if (gsm.getWindow() != nullptr) {
        mousePosition = gsm.getWindow()->mapPixelToCoords(mousePressed->position);
    }

    if (normalText.getGlobalBounds().contains(mousePosition)) {
        chooseOption(SelectionOption::Normal);
    } else if (nightfallText.getGlobalBounds().contains(mousePosition)) {
        chooseOption(SelectionOption::Nightfall);
    } else if (duelText.getGlobalBounds().contains(mousePosition)) {
        chooseOption(SelectionOption::Duel);
    }
}

void GameModeSelectionState::update(sf::Time) {
    normalText.setString(selectedOption == SelectionOption::Normal
        ? "> NORMAL MODE" : "  NORMAL MODE");
    normalText.setFillColor(selectedOption == SelectionOption::Normal
        ? sf::Color::Yellow : sf::Color(180, 180, 180));
    const sf::FloatRect nBounds = normalText.getLocalBounds();
    normalText.setOrigin({nBounds.position.x + nBounds.size.x / 2.f,
                          nBounds.position.y + nBounds.size.y / 2.f});
    normalText.setPosition({Config::kViewWidth / 2.f, Config::kViewHeight * 0.27f});

    const sf::FloatRect ndBounds = normalDesc.getLocalBounds();
    normalDesc.setOrigin({ndBounds.position.x + ndBounds.size.x / 2.f,
                          ndBounds.position.y + ndBounds.size.y / 2.f});
    normalDesc.setPosition({Config::kViewWidth / 2.f, Config::kViewHeight * 0.33f});

    nightfallText.setString(selectedOption == SelectionOption::Nightfall
        ? "> NIGHTFALL MODE" : "  NIGHTFALL MODE");
    nightfallText.setFillColor(selectedOption == SelectionOption::Nightfall
        ? sf::Color(255, 140, 0) : sf::Color(180, 180, 180));
    const sf::FloatRect nfBounds = nightfallText.getLocalBounds();
    nightfallText.setOrigin({nfBounds.position.x + nfBounds.size.x / 2.f,
                             nfBounds.position.y + nfBounds.size.y / 2.f});
    nightfallText.setPosition({Config::kViewWidth / 2.f, Config::kViewHeight * 0.46f});

    const sf::FloatRect nfdBounds = nightfallDesc.getLocalBounds();
    nightfallDesc.setOrigin({nfdBounds.position.x + nfdBounds.size.x / 2.f,
                             nfdBounds.position.y + nfdBounds.size.y / 2.f});
    nightfallDesc.setPosition({Config::kViewWidth / 2.f, Config::kViewHeight * 0.52f});

    duelText.setString(selectedOption == SelectionOption::Duel
        ? "> DUEL MODE" : "  DUEL MODE");
    duelText.setFillColor(selectedOption == SelectionOption::Duel
        ? sf::Color::Yellow : sf::Color(180, 180, 180));
    const sf::FloatRect duelBounds = duelText.getLocalBounds();
    duelText.setOrigin({duelBounds.position.x + duelBounds.size.x / 2.f,
                        duelBounds.position.y + duelBounds.size.y / 2.f});
    duelText.setPosition({Config::kViewWidth / 2.f, Config::kViewHeight * 0.65f});

    const sf::FloatRect duelDescBounds = duelDesc.getLocalBounds();
    duelDesc.setOrigin({duelDescBounds.position.x + duelDescBounds.size.x / 2.f,
                        duelDescBounds.position.y + duelDescBounds.size.y / 2.f});
    duelDesc.setPosition({Config::kViewWidth / 2.f, Config::kViewHeight * 0.71f});
}

void GameModeSelectionState::render(sf::RenderWindow& window) {
    window.draw(bgSprite);
    window.draw(darkOverlay);
    window.draw(headerText);
    window.draw(normalText);
    window.draw(normalDesc);
    window.draw(nightfallText);
    window.draw(nightfallDesc);
    window.draw(duelText);
    window.draw(duelDesc);
    window.draw(hintText);
}
