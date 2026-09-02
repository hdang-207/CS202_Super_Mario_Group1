#include "States/DuelPauseState.hpp"

#include "Core/Config.hpp"
#include "States/GameStateManager.hpp"
#include "Systems/ResourcePath.hpp"
#include "Systems/SoundController.hpp"

#include <iostream>

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
    Systems::AssetManager& assets
)
    : State(gsm, assets),
      titleText(assets.getFont("MarioFont")),
      resumeText(assets.getFont("MarioFont")),
      backText(assets.getFont("MarioFont")),
      hintText(assets.getFont("MarioFont")) {}

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

    for (sf::Text* option : {&resumeText, &backText}) {
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
        || key == sf::Keyboard::Scancode::W
        || key == sf::Keyboard::Scancode::Down
        || key == sf::Keyboard::Scancode::S) {
        moveSelection();
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
    window.draw(backText);
    window.draw(hintText);
}

void DuelPauseState::moveSelection() {
    selectedOption = selectedOption == Option::Resume
        ? Option::BackToModeSelection
        : Option::Resume;
    playSelectSound();
    updateOptionStyles();
}

void DuelPauseState::updateOptionStyles() {
    const bool resumeSelected = selectedOption == Option::Resume;
    resumeText.setString(resumeSelected ? "> RESUME" : "  RESUME");
    backText.setString(
        resumeSelected ? "  BACK TO MODE SELECT" : "> BACK TO MODE SELECT"
    );
    resumeText.setFillColor(
        resumeSelected ? sf::Color::Yellow : sf::Color::White
    );
    backText.setFillColor(
        resumeSelected ? sf::Color::White : sf::Color::Yellow
    );
    centreText(
        resumeText,
        {Config::kViewWidth / 2.f, Config::kViewHeight * 0.43f}
    );
    centreText(
        backText,
        {Config::kViewWidth / 2.f, Config::kViewHeight * 0.53f}
    );
}

void DuelPauseState::confirmSelection() {
    if (selectedOption == Option::Resume) {
        resumeDuel();
    } else {
        backToModeSelection();
    }
}

void DuelPauseState::resumeDuel() {
    transitionPending = true;
    playSelectSound();
    gsm.popState();
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
