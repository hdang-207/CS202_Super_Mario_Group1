#include "States/DuelPauseState.hpp"

#include "Core/Config.hpp"
#include "States/DuelArenaSelectionState.hpp"
#include "States/GameStateManager.hpp"
#include "Systems/ResourcePath.hpp"
#include "Systems/SoundController.hpp"

#include <array>
#include <iostream>
#include <memory>
#include <string>

namespace {

constexpr const char* kMenuMusicPath = "assets/audio/Theme.mp3";

void centreText(sf::Text& text, sf::Vector2f position) {
    const sf::FloatRect bounds = text.getLocalBounds();
    text.setOrigin({
        bounds.position.x + bounds.size.x / 2.f,
        bounds.position.y + bounds.size.y / 2.f
    });
    text.setPosition(position);
}

} // namespace

DuelPauseState::DuelPauseState(
    GameStateManager& gsm,
    Systems::AssetManager& assets,
    std::size_t arenaChoice
)
    : State(gsm, assets),
      titleText(assets.getFont("MarioFont")),
      resumeText(assets.getFont("MarioFont")),
      changeArenaText(assets.getFont("MarioFont")),
      backText(assets.getFont("MarioFont")),
      hintText(assets.getFont("MarioFont")),
      arenaChoice(arenaChoice) {}

void DuelPauseState::init() {
    std::cout << "[Core Engine] DuelPauseState Initialized.\n";
    selectedOption = Option::Resume;
    transitionPending = false;

    backgroundOverlay.setSize({Config::kViewWidth, Config::kViewHeight});
    backgroundOverlay.setFillColor(sf::Color(0, 0, 0, 180));

    titleText.setString("DUEL PAUSED");
    titleText.setCharacterSize(46);
    titleText.setFillColor(sf::Color::Yellow);
    titleText.setOutlineColor(sf::Color::Black);
    titleText.setOutlineThickness(3.f);
    centreText(
        titleText,
        {Config::kViewWidth / 2.f, Config::kViewHeight * 0.27f}
    );

    for (sf::Text* option : {&resumeText, &changeArenaText, &backText}) {
        option->setCharacterSize(28);
        option->setOutlineColor(sf::Color::Black);
        option->setOutlineThickness(2.f);
    }
    updateOptionStyles();

    hintText.setString(
        "UP/DOWN OR W/S: SELECT   ENTER/SPACE: CONFIRM\n"
        "ESC: RESUME   B: MODE SELECT"
    );
    hintText.setCharacterSize(15);
    hintText.setFillColor(sf::Color::White);
    hintText.setOutlineColor(sf::Color::Black);
    hintText.setOutlineThickness(2.f);
    centreText(
        hintText,
        {Config::kViewWidth / 2.f, Config::kViewHeight * 0.80f}
    );
}

void DuelPauseState::handleInput(const sf::Event& event) {
    if (transitionPending) {
        return;
    }

    const auto* keyPressed = event.getIf<sf::Event::KeyPressed>();
    if (!keyPressed) {
        return;
    }

    const auto key = keyPressed->scancode;
    if (key == sf::Keyboard::Scancode::Escape) {
        resumeDuel();
        return;
    }
    if (key == sf::Keyboard::Scancode::B) {
        backToModeSelection();
        return;
    }

    if (key == sf::Keyboard::Scancode::Up
        || key == sf::Keyboard::Scancode::W) {
        moveSelection(-1);
        return;
    }
    if (key == sf::Keyboard::Scancode::Down
        || key == sf::Keyboard::Scancode::S) {
        moveSelection(1);
        return;
    }

    if (key == sf::Keyboard::Scancode::Enter
        || key == sf::Keyboard::Scancode::Space) {
        confirmSelection();
    }
}

void DuelPauseState::update(sf::Time) {}

void DuelPauseState::render(sf::RenderWindow& window) {
    window.draw(backgroundOverlay);
    window.draw(titleText);
    window.draw(resumeText);
    window.draw(changeArenaText);
    window.draw(backText);
    window.draw(hintText);
}

void DuelPauseState::moveSelection(int step) {
    constexpr int optionCount = 3;
    const int current = static_cast<int>(selectedOption);
    const int next = (current + step + optionCount) % optionCount;
    selectedOption = static_cast<Option>(next);
    playSelectSound();
    updateOptionStyles();
}

void DuelPauseState::updateOptionStyles() {
    struct Entry {
        sf::Text* text;
        Option option;
        const char* label;
        float verticalFraction;
    };

    const std::array<Entry, 3> entries{{
        {&resumeText, Option::Resume, "RESUME", 0.41f},
        {&changeArenaText, Option::ChangeArena, "CHANGE ARENA", 0.50f},
        {&backText, Option::BackToModeSelection, "BACK TO MODE SELECT", 0.59f},
    }};

    for (const Entry& entry : entries) {
        const bool selected = selectedOption == entry.option;
        entry.text->setString(
            (selected ? "> " : "  ") + std::string(entry.label)
        );
        entry.text->setFillColor(
            selected ? sf::Color::Yellow : sf::Color::White
        );
        centreText(
            *entry.text,
            {
                Config::kViewWidth / 2.f,
                Config::kViewHeight * entry.verticalFraction
            }
        );
    }
}

void DuelPauseState::confirmSelection() {
    switch (selectedOption) {
        case Option::Resume:
            resumeDuel();
            break;

        case Option::ChangeArena:
            changeArena();
            break;

        case Option::BackToModeSelection:
            backToModeSelection();
            break;
    }
}

void DuelPauseState::resumeDuel() {
    transitionPending = true;
    playSelectSound();
    gsm.popState();
}

void DuelPauseState::changeArena() {
    transitionPending = true;
    playSelectSound();
    Systems::SoundController::getInstance().playMusic(
        Systems::resourcePath(kMenuMusicPath)
    );
    // Close this overlay first, then swap the duel underneath it for the arena
    // screen, so the mode menu stays exactly one state further down.
    gsm.popState();
    gsm.changeState(
        std::make_unique<DuelArenaSelectionState>(gsm, assets, arenaChoice)
    );
}

void DuelPauseState::backToModeSelection() {
    transitionPending = true;
    playSelectSound();
    Systems::SoundController::getInstance().playMusic(
        Systems::resourcePath(kMenuMusicPath)
    );
    gsm.popState();
    gsm.popState();
}

void DuelPauseState::playSelectSound() {
    Systems::SoundController::getInstance().playSound(
        assets.getSoundBuffer("SelectSound")
    );
}
