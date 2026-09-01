#include "States/WorldSelectionState.hpp"

#include "Core/Config.hpp"
#include "States/CharacterSelectionState.hpp"
#include "States/DuelState.hpp"
#include "States/GameStateManager.hpp"
#include "States/PlayState.hpp"
#include "Systems/SaveData.hpp"
#include "Systems/SoundController.hpp"
#include <algorithm>
#include <array>
#include <iostream>
#include <string>

WorldSelectionState::WorldSelectionState(
    GameStateManager& gsm, Systems::AssetManager& assets, CharacterType character)
    : State(gsm, assets), selectedCharacter(character),
      bgSprite(assets.getTexture("MenuBackground")),
      headerText(assets.getFont("MarioFont")),
      world1Text(assets.getFont("MarioFont")),
      world2Text(assets.getFont("MarioFont")),
      world3Text(assets.getFont("MarioFont")),
      duelText(assets.getFont("MarioFont")),
      routeText(assets.getFont("MarioFont")),
      hintText(assets.getFont("MarioFont")) {}

void WorldSelectionState::init() {
    std::cout << "[Core Engine] WorldSelectionState Initialized.\n";

    const sf::Vector2f bgSize(bgSprite.getTexture().getSize());
    const float bgScale = std::max(Config::kViewWidth / bgSize.x,
                                   Config::kViewHeight / bgSize.y);
    bgSprite.setScale({bgScale, bgScale});
    bgSprite.setPosition({(Config::kViewWidth - bgSize.x * bgScale) / 2.f,
                          (Config::kViewHeight - bgSize.y * bgScale) / 2.f});

    darkOverlay.setSize({Config::kViewWidth, Config::kViewHeight});
    darkOverlay.setFillColor(sf::Color(0, 0, 0, 175));

    headerText.setString("SELECT LEVEL MODE");
    headerText.setCharacterSize(38);
    headerText.setFillColor(sf::Color::Yellow);
    headerText.setOutlineColor(sf::Color::Black);
    headerText.setOutlineThickness(3.f);
    const sf::FloatRect headerBounds = headerText.getLocalBounds();
    headerText.setOrigin({headerBounds.position.x + headerBounds.size.x / 2.f,
                          headerBounds.position.y + headerBounds.size.y / 2.f});
    headerText.setPosition({Config::kViewWidth / 2.f,
                            Config::kViewHeight * 0.18f});

    for (sf::Text* option : {&world1Text, &world2Text, &world3Text, &duelText}) {
        option->setCharacterSize(24);
        option->setOutlineColor(sf::Color::Black);
        option->setOutlineThickness(2.f);
    }

    routeText.setCharacterSize(18);
    routeText.setFillColor(sf::Color::White);
    routeText.setOutlineColor(sf::Color::Black);
    routeText.setOutlineThickness(2.f);

    hintText.setString(
        "UP/DOWN OR 1/2/3/4: SELECT  |  ENTER/SPACE: START  |  B/ESC: BACK");
    hintText.setCharacterSize(15);
    hintText.setFillColor(sf::Color::White);
    hintText.setOutlineColor(sf::Color::Black);
    hintText.setOutlineThickness(2.f);
    const sf::FloatRect hintBounds = hintText.getLocalBounds();
    hintText.setOrigin({hintBounds.position.x + hintBounds.size.x / 2.f,
                        hintBounds.position.y + hintBounds.size.y / 2.f});
    hintText.setPosition({Config::kViewWidth / 2.f,
                          Config::kViewHeight * 0.86f});
}

void WorldSelectionState::selectPreviousOption() {
    switch (selectedOption) {
        case SelectionOption::World1:
            chooseOption(SelectionOption::Duel);
            break;
        case SelectionOption::World2:
            chooseOption(SelectionOption::World1);
            break;
        case SelectionOption::World3:
            chooseOption(SelectionOption::World2);
            break;
        case SelectionOption::Duel:
            chooseOption(SelectionOption::World3);
            break;
    }
}

void WorldSelectionState::selectNextOption() {
    switch (selectedOption) {
        case SelectionOption::World1:
            chooseOption(SelectionOption::World2);
            break;
        case SelectionOption::World2:
            chooseOption(SelectionOption::World3);
            break;
        case SelectionOption::World3:
            chooseOption(SelectionOption::Duel);
            break;
        case SelectionOption::Duel:
            chooseOption(SelectionOption::World1);
            break;
    }
}

void WorldSelectionState::chooseOption(SelectionOption option) {
    if (option == selectedOption) {
        return;
    }
    selectedOption = option;
    Systems::SoundController::getInstance().playSound(
        assets.getSoundBuffer("SelectSound"));
}

void WorldSelectionState::startSelectedOption() {
    Systems::SoundController::getInstance().playSound(
        assets.getSoundBuffer("SelectSound"));

    if (selectedOption == SelectionOption::Duel) {
        std::cout << "[Core Engine] Starting Duel Mode.\n";
        gsm.pushState(std::make_unique<DuelState>(gsm, assets));
        return;
    }

    int selectedWorld = 1;
    switch (selectedOption) {
        case SelectionOption::World1:
            selectedWorld = 1;
            break;
        case SelectionOption::World2:
            selectedWorld = 2;
            break;
        case SelectionOption::World3:
            selectedWorld = 3;
            break;
        case SelectionOption::Duel:
            return;
    }

    SaveData progress;
    progress.currentLevel = Config::firstLevelOfWorld(selectedWorld);
    progress.selectedCharacter = selectedCharacter;

    std::cout << "[Core Engine] Starting Level " << selectedWorld
              << " route at World " << selectedWorld << "-1.\n";
    gsm.changeState(std::make_unique<PlayState>(gsm, assets, progress));
}

void WorldSelectionState::handleInput(const sf::Event& event) {
    if (const auto* keyPressed = event.getIf<sf::Event::KeyPressed>()) {
        const auto key = keyPressed->scancode;
        if (key == sf::Keyboard::Scancode::Up || key == sf::Keyboard::Scancode::W) {
            selectPreviousOption();
        } else if (key == sf::Keyboard::Scancode::Down || key == sf::Keyboard::Scancode::S) {
            selectNextOption();
        } else if (key == sf::Keyboard::Scancode::Num1
                   || key == sf::Keyboard::Scancode::Numpad1) {
            chooseOption(SelectionOption::World1);
        } else if (key == sf::Keyboard::Scancode::Num2
                   || key == sf::Keyboard::Scancode::Numpad2) {
            chooseOption(SelectionOption::World2);
        } else if (key == sf::Keyboard::Scancode::Num3
                   || key == sf::Keyboard::Scancode::Numpad3) {
            chooseOption(SelectionOption::World3);
        } else if (key == sf::Keyboard::Scancode::Num4
                   || key == sf::Keyboard::Scancode::Numpad4) {
            chooseOption(SelectionOption::Duel);
        } else if (key == sf::Keyboard::Scancode::Enter
                   || key == sf::Keyboard::Scancode::Space) {
            startSelectedOption();
        } else if (key == sf::Keyboard::Scancode::B
                   || key == sf::Keyboard::Scancode::Escape) {
            Systems::SoundController::getInstance().playSound(
                assets.getSoundBuffer("SelectSound"));
            gsm.changeState(std::make_unique<CharacterSelectionState>(gsm, assets));
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

    if (world1Text.getGlobalBounds().contains(mousePosition)) {
        chooseOption(SelectionOption::World1);
    } else if (world2Text.getGlobalBounds().contains(mousePosition)) {
        chooseOption(SelectionOption::World2);
    } else if (world3Text.getGlobalBounds().contains(mousePosition)) {
        chooseOption(SelectionOption::World3);
    } else if (duelText.getGlobalBounds().contains(mousePosition)) {
        chooseOption(SelectionOption::Duel);
    }
}

void WorldSelectionState::update(sf::Time) {
    const std::array<std::pair<SelectionOption, sf::Text*>, 4> options{{
        {SelectionOption::World1, &world1Text},
        {SelectionOption::World2, &world2Text},
        {SelectionOption::World3, &world3Text},
        {SelectionOption::Duel, &duelText}
    }};

    for (std::size_t index = 0; index < options.size(); ++index) {
        const SelectionOption optionValue = options[index].first;
        sf::Text& optionText = *options[index].second;
        const std::string prefix = selectedOption == optionValue ? "> " : "  ";

        if (optionValue == SelectionOption::Duel) {
            optionText.setString(prefix + "DUEL MODE");
        } else {
            const int world = static_cast<int>(index) + 1;
            optionText.setString(prefix + "LEVEL " + std::to_string(world)
                                 + "    WORLD " + std::to_string(world)
                                 + "-1 TO " + std::to_string(world) + "-"
                                 + std::to_string(Config::stageCount(world)));
        }

        optionText.setFillColor(selectedOption == optionValue
            ? sf::Color::Yellow : sf::Color(180, 180, 180));

        const sf::FloatRect bounds = optionText.getLocalBounds();
        optionText.setOrigin({bounds.position.x + bounds.size.x / 2.f,
                              bounds.position.y + bounds.size.y / 2.f});
        optionText.setPosition({Config::kViewWidth / 2.f,
                                Config::kViewHeight * (0.32f + index * 0.09f)});
    }

    std::string route;
    if (selectedOption == SelectionOption::Duel) {
        route = "MARIO VS LUIGI  |  LOCAL 2 PLAYER";
    } else {
        const int selectedWorld = static_cast<int>(selectedOption) + 1;
        route = "ROUTE: ";
        for (int stage = 1; stage <= Config::stageCount(selectedWorld); ++stage) {
            if (stage > 1) {
                route += "  >  ";
            }
            route += std::to_string(selectedWorld) + "-" + std::to_string(stage);
        }
    }
    routeText.setString(route);
    const sf::FloatRect routeBounds = routeText.getLocalBounds();
    routeText.setOrigin({routeBounds.position.x + routeBounds.size.x / 2.f,
                         routeBounds.position.y + routeBounds.size.y / 2.f});
    routeText.setPosition({Config::kViewWidth / 2.f,
                           Config::kViewHeight * 0.72f});
}

void WorldSelectionState::render(sf::RenderWindow& window) {
    window.draw(bgSprite);
    window.draw(darkOverlay);
    window.draw(headerText);
    window.draw(world1Text);
    window.draw(world2Text);
    window.draw(world3Text);
    window.draw(duelText);
    window.draw(routeText);
    window.draw(hintText);
}
