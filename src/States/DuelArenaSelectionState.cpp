#include "States/DuelArenaSelectionState.hpp"

#include "Core/Config.hpp"
#include "States/DuelState.hpp"
#include "States/GameStateManager.hpp"
#include "Systems/MapParser.hpp"
#include "Systems/ResourcePath.hpp"
#include "Systems/SoundController.hpp"

#include <algorithm>
#include <iostream>
#include <string>

namespace {

constexpr const char* kMenuMusicPath = "assets/audio/Theme.mp3";

// The minimap keeps the arena's own 24x15 proportions, just shrunk down.
constexpr float kPreviewTileSize = 18.f;
constexpr float kPreviewWidth = Config::kViewTilesX * kPreviewTileSize;
constexpr float kPreviewHeight = Config::kViewTilesY * kPreviewTileSize;
constexpr sf::Vector2f kPreviewOrigin{
    (Config::kViewWidth - kPreviewWidth) / 2.f,
    Config::kViewHeight * 0.20f
};
constexpr sf::Vector2f kPreviewCentre{
    kPreviewOrigin.x + kPreviewWidth / 2.f,
    kPreviewOrigin.y + kPreviewHeight / 2.f
};
constexpr float kArrowOffset = 46.f;

// Sky and blocks match the colours the duel itself draws, so the minimap reads
// as the arena rather than as an abstract diagram.
const sf::Color kSkyColour(92, 148, 252);
const sf::Color kBlockColour(199, 86, 42);
const sf::Color kBlockOutlineColour(94, 38, 12);
const sf::Color kPlayerOneColour(238, 52, 45);
const sf::Color kPlayerTwoColour(64, 205, 79);

void centreText(sf::Text& text, sf::Vector2f position) {
    const sf::FloatRect bounds = text.getLocalBounds();
    text.setOrigin({
        bounds.position.x + bounds.size.x / 2.f,
        bounds.position.y + bounds.size.y / 2.f
    });
    text.setPosition(position);
}

} // namespace

DuelArenaSelectionState::DuelArenaSelectionState(
    GameStateManager& gsm,
    Systems::AssetManager& assets,
    std::size_t initiallySelected
)
    : State(gsm, assets),
      selectedChoice(
          initiallySelected < duel::kArenaChoiceCount
              ? initiallySelected
              : duel::kRandomArena
      ),
      bgSprite(assets.getTexture("MenuBackground")),
      headerText(assets.getFont("MarioFont")),
      arenaNameText(assets.getFont("MarioFont")),
      arenaDescriptionText(assets.getFont("MarioFont")),
      pagerText(assets.getFont("MarioFont")),
      leftArrowText(assets.getFont("MarioFont")),
      rightArrowText(assets.getFont("MarioFont")),
      randomMarkText(assets.getFont("MarioFont")),
      previewErrorText(assets.getFont("MarioFont")),
      hintText(assets.getFont("MarioFont")) {}

void DuelArenaSelectionState::init() {
    std::cout << "[Core Engine] DuelArenaSelectionState Initialized.\n";

    // Reached either from the mode menu or from a paused duel; the second path
    // still has the battle track playing.
    Systems::SoundController::getInstance().playMusic(
        Systems::resourcePath(kMenuMusicPath)
    );

    const sf::Vector2f bgSize(bgSprite.getTexture().getSize());
    const float bgScale = std::max(
        Config::kViewWidth / bgSize.x,
        Config::kViewHeight / bgSize.y
    );
    bgSprite.setScale({bgScale, bgScale});
    bgSprite.setPosition({
        (Config::kViewWidth - bgSize.x * bgScale) / 2.f,
        (Config::kViewHeight - bgSize.y * bgScale) / 2.f
    });

    darkOverlay.setSize({Config::kViewWidth, Config::kViewHeight});
    darkOverlay.setFillColor(sf::Color(0, 0, 0, 175));

    previewPanel.setSize({kPreviewWidth, kPreviewHeight});
    previewPanel.setPosition(kPreviewOrigin);
    previewPanel.setFillColor(kSkyColour);
    previewPanel.setOutlineColor(sf::Color::White);
    previewPanel.setOutlineThickness(4.f);

    headerText.setString("SELECT ARENA");
    headerText.setCharacterSize(38);
    headerText.setFillColor(sf::Color::Yellow);
    headerText.setOutlineColor(sf::Color::Black);
    headerText.setOutlineThickness(3.f);
    centreText(headerText, {Config::kViewWidth / 2.f, Config::kViewHeight * 0.09f});

    arenaNameText.setCharacterSize(30);
    arenaNameText.setFillColor(sf::Color::Yellow);
    arenaNameText.setOutlineColor(sf::Color::Black);
    arenaNameText.setOutlineThickness(2.f);

    arenaDescriptionText.setCharacterSize(16);
    arenaDescriptionText.setFillColor(sf::Color(220, 220, 220));
    arenaDescriptionText.setOutlineColor(sf::Color::Black);
    arenaDescriptionText.setOutlineThickness(2.f);

    pagerText.setCharacterSize(16);
    pagerText.setFillColor(sf::Color(180, 220, 255));
    pagerText.setOutlineColor(sf::Color::Black);
    pagerText.setOutlineThickness(2.f);

    for (sf::Text* arrow : {&leftArrowText, &rightArrowText}) {
        arrow->setCharacterSize(56);
        arrow->setFillColor(sf::Color::White);
        arrow->setOutlineColor(sf::Color::Black);
        arrow->setOutlineThickness(3.f);
    }
    leftArrowText.setString("<");
    rightArrowText.setString(">");
    centreText(
        leftArrowText,
        {kPreviewOrigin.x - kArrowOffset, kPreviewCentre.y}
    );
    centreText(
        rightArrowText,
        {kPreviewOrigin.x + kPreviewWidth + kArrowOffset, kPreviewCentre.y}
    );

    randomMarkText.setString("?");
    randomMarkText.setCharacterSize(120);
    randomMarkText.setFillColor(sf::Color::White);
    randomMarkText.setOutlineColor(sf::Color::Black);
    randomMarkText.setOutlineThickness(4.f);
    centreText(randomMarkText, kPreviewCentre);

    previewErrorText.setString("MAP FILE MISSING");
    previewErrorText.setCharacterSize(22);
    previewErrorText.setFillColor(sf::Color::White);
    previewErrorText.setOutlineColor(sf::Color::Red);
    previewErrorText.setOutlineThickness(2.f);
    centreText(previewErrorText, kPreviewCentre);

    hintText.setString(
        "LEFT/RIGHT OR A/D: BROWSE  |  ENTER/SPACE: FIGHT  |  B/ESC: BACK"
    );
    hintText.setCharacterSize(15);
    hintText.setFillColor(sf::Color::White);
    hintText.setOutlineColor(sf::Color::Black);
    hintText.setOutlineThickness(2.f);
    centreText(hintText, {Config::kViewWidth / 2.f, Config::kViewHeight * 0.92f});

    buildPreviews();
    refreshSelectionText();
}

void DuelArenaSelectionState::buildPreviews() {
    previews.clear();
    previews.reserve(duel::kArenaCount);

    for (const duel::ArenaDefinition& arena : duel::kArenas) {
        ArenaPreview preview;
        MapParser parser;
        if (!parser.loadFromFile(Systems::resourcePath(arena.file))) {
            std::cerr << "[DuelArenaSelectionState] No preview for arena: "
                      << arena.file << '\n';
            previews.push_back(std::move(preview));
            continue;
        }

        const auto& grid = parser.getGrid();
        for (std::size_t row = 0; row < grid.size(); ++row) {
            for (std::size_t column = 0; column < grid[row].size(); ++column) {
                const sf::Vector2f cell{
                    kPreviewOrigin.x
                        + static_cast<float>(column) * kPreviewTileSize,
                    kPreviewOrigin.y
                        + static_cast<float>(row) * kPreviewTileSize
                };

                if (grid[row][column] == '#') {
                    sf::RectangleShape block({kPreviewTileSize, kPreviewTileSize});
                    block.setPosition(cell);
                    block.setFillColor(kBlockColour);
                    block.setOutlineColor(kBlockOutlineColour);
                    block.setOutlineThickness(-1.f);
                    preview.blocks.push_back(block);
                } else if (grid[row][column] == 'P') {
                    // The map file lists spawns left to right, so the first
                    // marker found is always player one.
                    const bool firstSpawn = preview.spawnMarkers.empty();
                    const float radius = kPreviewTileSize * 0.36f;
                    sf::CircleShape marker(radius);
                    marker.setOrigin({radius, radius});
                    marker.setPosition({
                        cell.x + kPreviewTileSize / 2.f,
                        cell.y + kPreviewTileSize / 2.f
                    });
                    marker.setFillColor(
                        firstSpawn ? kPlayerOneColour : kPlayerTwoColour
                    );
                    marker.setOutlineColor(sf::Color::Black);
                    marker.setOutlineThickness(1.f);
                    preview.spawnMarkers.push_back(marker);
                }
            }
        }

        preview.loaded = !preview.blocks.empty()
            && preview.spawnMarkers.size() == 2;
        previews.push_back(std::move(preview));
    }
}

void DuelArenaSelectionState::chooseArena(std::size_t choice) {
    if (choice == selectedChoice || choice >= duel::kArenaChoiceCount) {
        return;
    }

    selectedChoice = choice;
    playSelectSound();
    refreshSelectionText();
}

void DuelArenaSelectionState::refreshSelectionText() {
    arenaNameText.setString(duel::arenaChoiceName(selectedChoice));
    centreText(arenaNameText, {Config::kViewWidth / 2.f, Config::kViewHeight * 0.64f});

    arenaDescriptionText.setString(duel::arenaChoiceDescription(selectedChoice));
    centreText(
        arenaDescriptionText,
        {Config::kViewWidth / 2.f, Config::kViewHeight * 0.70f}
    );

    pagerText.setString(
        std::to_string(selectedChoice + 1) + " / "
            + std::to_string(duel::kArenaChoiceCount)
    );
    centreText(pagerText, {Config::kViewWidth / 2.f, Config::kViewHeight * 0.75f});
}

void DuelArenaSelectionState::confirmSelection() {
    playSelectSound();
    std::cout << "[Core Engine] Duel arena selected: "
              << duel::arenaChoiceName(selectedChoice) << '\n';

    // Replacing this screen keeps the duel sitting directly on top of the mode
    // menu, which is what the pause menu pops back to.
    gsm.changeState(std::make_unique<DuelState>(gsm, assets, selectedChoice));
}

void DuelArenaSelectionState::goBack() {
    playSelectSound();
    gsm.popState();
}

void DuelArenaSelectionState::playSelectSound() {
    Systems::SoundController::getInstance().playSound(
        assets.getSoundBuffer("SelectSound")
    );
}

void DuelArenaSelectionState::handleInput(const sf::Event& event) {
    if (const auto* keyPressed = event.getIf<sf::Event::KeyPressed>()) {
        const auto key = keyPressed->scancode;

        if (key == sf::Keyboard::Scancode::Left
            || key == sf::Keyboard::Scancode::A
            || key == sf::Keyboard::Scancode::Up
            || key == sf::Keyboard::Scancode::W) {
            chooseArena(duel::previousArenaChoice(selectedChoice));
        } else if (
            key == sf::Keyboard::Scancode::Right
            || key == sf::Keyboard::Scancode::D
            || key == sf::Keyboard::Scancode::Down
            || key == sf::Keyboard::Scancode::S
        ) {
            chooseArena(duel::nextArenaChoice(selectedChoice));
        } else if (
            key == sf::Keyboard::Scancode::Num1
            || key == sf::Keyboard::Scancode::Numpad1
        ) {
            chooseArena(0);
        } else if (
            key == sf::Keyboard::Scancode::Num2
            || key == sf::Keyboard::Scancode::Numpad2
        ) {
            chooseArena(1);
        } else if (
            key == sf::Keyboard::Scancode::Num3
            || key == sf::Keyboard::Scancode::Numpad3
        ) {
            chooseArena(2);
        } else if (
            key == sf::Keyboard::Scancode::Num4
            || key == sf::Keyboard::Scancode::Numpad4
        ) {
            chooseArena(3);
        } else if (
            key == sf::Keyboard::Scancode::Enter
            || key == sf::Keyboard::Scancode::Space
        ) {
            confirmSelection();
        } else if (
            key == sf::Keyboard::Scancode::B
            || key == sf::Keyboard::Scancode::Escape
        ) {
            goBack();
        }

        return;
    }

    const auto* mousePressed = event.getIf<sf::Event::MouseButtonPressed>();
    if (!mousePressed || mousePressed->button != sf::Mouse::Button::Left) {
        return;
    }

    sf::Vector2f mousePosition(
        static_cast<float>(mousePressed->position.x),
        static_cast<float>(mousePressed->position.y)
    );
    if (gsm.getWindow() != nullptr) {
        mousePosition = gsm.getWindow()->mapPixelToCoords(mousePressed->position);
    }

    if (leftArrowText.getGlobalBounds().contains(mousePosition)) {
        chooseArena(duel::previousArenaChoice(selectedChoice));
    } else if (rightArrowText.getGlobalBounds().contains(mousePosition)) {
        chooseArena(duel::nextArenaChoice(selectedChoice));
    } else if (previewPanel.getGlobalBounds().contains(mousePosition)) {
        confirmSelection();
    }
}

void DuelArenaSelectionState::update(sf::Time) {}

void DuelArenaSelectionState::render(sf::RenderWindow& window) {
    window.draw(bgSprite);
    window.draw(darkOverlay);
    window.draw(headerText);
    window.draw(previewPanel);

    if (duel::isRandomArena(selectedChoice)) {
        window.draw(randomMarkText);
    } else if (selectedChoice < previews.size()
               && previews[selectedChoice].loaded) {
        const ArenaPreview& preview = previews[selectedChoice];
        for (const sf::RectangleShape& block : preview.blocks) {
            window.draw(block);
        }
        for (const sf::CircleShape& marker : preview.spawnMarkers) {
            window.draw(marker);
        }
    } else {
        window.draw(previewErrorText);
    }

    window.draw(leftArrowText);
    window.draw(rightArrowText);
    window.draw(arenaNameText);
    window.draw(arenaDescriptionText);
    window.draw(pagerText);
    window.draw(hintText);
}
