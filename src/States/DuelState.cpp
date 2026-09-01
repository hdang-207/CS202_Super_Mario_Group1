#include "States/DuelState.hpp"

#include "Core/Config.hpp"
#include "States/GameStateManager.hpp"
#include "Systems/ResourcePath.hpp"
#include "Systems/SoundController.hpp"
#include <algorithm>
#include <cstddef>
#include <iostream>
#include <string>
#include <vector>

DuelState::DuelState(GameStateManager& gsm, Systems::AssetManager& assets)
    : State(gsm, assets),
      titleText(assets.getFont("MarioFont")),
      errorText(assets.getFont("MarioFont")),
      hintText(assets.getFont("MarioFont")) {}

void DuelState::init() {
    std::cout << "[Core Engine] DuelState Initialized.\n";
    arenaLoaded = false;

    skyBackground.setSize({Config::kViewWidth, Config::kViewHeight});
    skyBackground.setFillColor(sf::Color(92, 148, 252));

    titleText.setString("DUEL ARENA");
    titleText.setCharacterSize(28);
    titleText.setFillColor(sf::Color::Yellow);
    titleText.setOutlineColor(sf::Color::Black);
    titleText.setOutlineThickness(2.f);
    const sf::FloatRect titleBounds = titleText.getLocalBounds();
    titleText.setOrigin({titleBounds.position.x + titleBounds.size.x / 2.f,
                         titleBounds.position.y + titleBounds.size.y / 2.f});
    titleText.setPosition({Config::kViewWidth / 2.f,
                           Config::kViewHeight * 0.07f});

    errorText.setString("FAILED TO LOAD DUEL ARENA");
    errorText.setCharacterSize(24);
    errorText.setFillColor(sf::Color::White);
    errorText.setOutlineColor(sf::Color::Red);
    errorText.setOutlineThickness(2.f);
    const sf::FloatRect errorBounds = errorText.getLocalBounds();
    errorText.setOrigin({errorBounds.position.x + errorBounds.size.x / 2.f,
                         errorBounds.position.y + errorBounds.size.y / 2.f});
    errorText.setPosition({Config::kViewWidth / 2.f,
                           Config::kViewHeight * 0.50f});

    hintText.setString("B / ESC: BACK");
    hintText.setCharacterSize(18);
    hintText.setFillColor(sf::Color::White);
    hintText.setOutlineColor(sf::Color::Black);
    hintText.setOutlineThickness(2.f);
    const sf::FloatRect hintBounds = hintText.getLocalBounds();
    hintText.setOrigin({hintBounds.position.x + hintBounds.size.x / 2.f,
                        hintBounds.position.y + hintBounds.size.y / 2.f});
    hintText.setPosition({Config::kViewWidth / 2.f,
                          Config::kViewHeight * 0.95f});

    tileMap.setTileTexture('#', assets.getTexture("GroundTile"));

    const std::string arenaPath = Systems::resourcePath("assets/maps/duel_arena.txt");
    if (!mapParser.loadFromFile(arenaPath)) {
        std::cerr << "[DuelState] Failed to load arena map: " << arenaPath << '\n';
        return;
    }

    if (!tileMap.build(mapParser, Config::kZoom)) {
        std::cerr << "[DuelState] Arena map is empty and could not be built.\n";
        return;
    }

    constexpr std::size_t expectedColumns =
        static_cast<std::size_t>(Config::kViewTilesX);
    constexpr std::size_t expectedRows =
        static_cast<std::size_t>(Config::kViewTilesY);
    if (mapParser.getWidth() != expectedColumns
        || mapParser.getHeight() != expectedRows) {
        std::cerr << "[DuelState] Arena must be exactly " << Config::kViewTilesX
                  << 'x' << Config::kViewTilesY << " tiles; loaded "
                  << mapParser.getWidth() << 'x' << mapParser.getHeight() << ".\n";
        return;
    }

    const auto& grid = mapParser.getGrid();
    const bool rowWidthsValid = std::all_of(
        grid.begin(),
        grid.end(),
        [](const std::vector<char>& row) {
            return row.size() == expectedColumns;
        }
    );
    if (!rowWidthsValid) {
        std::cerr << "[DuelState] Every arena row must contain exactly "
                  << expectedColumns << " tiles.\n";
        return;
    }

    const auto& spawns = tileMap.playerSpawns();
    if (spawns.size() != 2) {
        std::cerr << "[DuelState] Arena requires exactly two player spawns; found "
                  << spawns.size() << ".\n";
        return;
    }
    if (spawns[0].x >= spawns[1].x) {
        std::cerr << "[DuelState] Arena spawns must be ordered left to right.\n";
        return;
    }

    arenaLoaded = true;
    std::cout << "[DuelState] Arena loaded with two player spawns.\n";
}

void DuelState::handleInput(const sf::Event& event) {
    const auto* keyPressed = event.getIf<sf::Event::KeyPressed>();
    if (!keyPressed) {
        return;
    }

    const auto key = keyPressed->scancode;
    if (key == sf::Keyboard::Scancode::B
        || key == sf::Keyboard::Scancode::Escape) {
        Systems::SoundController::getInstance().playSound(
            assets.getSoundBuffer("SelectSound"));
        gsm.popState();
    }
}

void DuelState::update(sf::Time dt) {
    if (arenaLoaded) {
        tileMap.update(dt);
    }
}

void DuelState::render(sf::RenderWindow& window) {
    window.draw(skyBackground);
    if (arenaLoaded) {
        window.draw(tileMap);
    } else {
        window.draw(errorText);
    }
    window.draw(titleText);
    window.draw(hintText);
}
