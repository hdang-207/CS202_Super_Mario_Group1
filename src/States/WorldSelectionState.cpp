#include "States/WorldSelectionState.hpp"

#include "Core/Config.hpp"
#include "States/CharacterSelectionState.hpp"
#include "States/GameStateManager.hpp"
#include "States/PlayState.hpp"
#include "Systems/SaveData.hpp"
#include "Systems/SoundController.hpp"
#include <algorithm>
#include <iostream>
#include <string>

WorldSelectionState::WorldSelectionState(
    GameStateManager& gsm,
    Systems::AssetManager& assets,
    CharacterType character,
    bool nightfall
)
    : State(gsm, assets), selectedCharacter(character), nightfallMode(nightfall),
      bgSprite(assets.getTexture("MenuBackground")),
      headerText(assets.getFont("MarioFont")),
      world1Text(assets.getFont("MarioFont")),
      world2Text(assets.getFont("MarioFont")),
      world3Text(assets.getFont("MarioFont")),
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

    headerText.setString("SELECT WORLD");
    headerText.setCharacterSize(38);
    headerText.setFillColor(sf::Color::Yellow);
    headerText.setOutlineColor(sf::Color::Black);
    headerText.setOutlineThickness(3.f);
    const sf::FloatRect headerBounds = headerText.getLocalBounds();
    headerText.setOrigin({headerBounds.position.x + headerBounds.size.x / 2.f,
                          headerBounds.position.y + headerBounds.size.y / 2.f});
    headerText.setPosition({Config::kViewWidth / 2.f,
                            Config::kViewHeight * 0.18f});

    for (sf::Text* option : {&world1Text, &world2Text, &world3Text}) {
        option->setCharacterSize(24);
        option->setOutlineColor(sf::Color::Black);
        option->setOutlineThickness(2.f);
    }

    routeText.setCharacterSize(18);
    routeText.setFillColor(sf::Color::White);
    routeText.setOutlineColor(sf::Color::Black);
    routeText.setOutlineThickness(2.f);

    hintText.setString(
        "UP/DOWN OR 1/2/3: SELECT  |  ENTER/SPACE: START  |  B/ESC: BACK");
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

void WorldSelectionState::chooseWorld(int world) {
    const int clampedWorld = std::clamp(world, 1, Config::kWorldCount);
    if (clampedWorld == selectedWorld) {
        return;
    }
    selectedWorld = clampedWorld;
    Systems::SoundController::getInstance().playSound(
        assets.getSoundBuffer("SelectSound"));
}

void WorldSelectionState::startSelectedWorld() {
    SaveData progress;
    progress.currentLevel = Config::firstLevelOfWorld(selectedWorld);
    progress.selectedCharacter = selectedCharacter;
    progress.nightfallMode = nightfallMode;

    std::cout << "[Core Engine] Starting Level " << selectedWorld
              << " route at World " << selectedWorld << "-1.\n";
    Systems::SoundController::getInstance().playSound(
        assets.getSoundBuffer("SelectSound"));
    gsm.changeState(std::make_unique<PlayState>(gsm, assets, progress));
}

void WorldSelectionState::handleInput(const sf::Event& event) {
    if (const auto* keyPressed = event.getIf<sf::Event::KeyPressed>()) {
        const auto key = keyPressed->scancode;
        if (key == sf::Keyboard::Scancode::Up || key == sf::Keyboard::Scancode::W) {
            chooseWorld(selectedWorld == 1 ? Config::kWorldCount : selectedWorld - 1);
        } else if (key == sf::Keyboard::Scancode::Down || key == sf::Keyboard::Scancode::S) {
            chooseWorld(selectedWorld == Config::kWorldCount ? 1 : selectedWorld + 1);
        } else if (key == sf::Keyboard::Scancode::Num1
                   || key == sf::Keyboard::Scancode::Numpad1) {
            chooseWorld(1);
        } else if (key == sf::Keyboard::Scancode::Num2
                   || key == sf::Keyboard::Scancode::Numpad2) {
            chooseWorld(2);
        } else if (key == sf::Keyboard::Scancode::Num3
                   || key == sf::Keyboard::Scancode::Numpad3) {
            chooseWorld(3);
        } else if (key == sf::Keyboard::Scancode::Enter
                   || key == sf::Keyboard::Scancode::Space) {
            startSelectedWorld();
        } else if (key == sf::Keyboard::Scancode::B
                   || key == sf::Keyboard::Scancode::Escape) {
            Systems::SoundController::getInstance().playSound(
                assets.getSoundBuffer("SelectSound"));
            gsm.changeState(std::make_unique<CharacterSelectionState>(
                gsm, assets, nightfallMode));
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
        chooseWorld(1);
    } else if (world2Text.getGlobalBounds().contains(mousePosition)) {
        chooseWorld(2);
    } else if (world3Text.getGlobalBounds().contains(mousePosition)) {
        chooseWorld(3);
    }
}

void WorldSelectionState::update(sf::Time) {
    sf::Text* options[] = {&world1Text, &world2Text, &world3Text};
    for (int index = 0; index < Config::kWorldCount; ++index) {
        const int world = index + 1;
        sf::Text& optionText = *options[index];
        const std::string prefix = selectedWorld == world ? "> " : "  ";
        optionText.setString(prefix + "WORLD " + std::to_string(world));
        optionText.setFillColor(selectedWorld == world
            ? sf::Color::Yellow : sf::Color(180, 180, 180));

        const sf::FloatRect bounds = optionText.getLocalBounds();
        optionText.setOrigin({bounds.position.x + bounds.size.x / 2.f,
                              bounds.position.y + bounds.size.y / 2.f});
        optionText.setPosition({Config::kViewWidth / 2.f,
                                Config::kViewHeight * (0.36f + index * 0.12f)});
    }

    std::string route = "ROUTE: ";
    for (int stage = 1; stage <= Config::stageCount(selectedWorld); ++stage) {
        if (stage > 1) {
            route += "  >  ";
        }
        route += std::to_string(selectedWorld) + "-" + std::to_string(stage);
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
    window.draw(routeText);
    window.draw(hintText);
}
