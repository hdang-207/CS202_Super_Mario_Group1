#pragma once

#include "States/State.hpp"

#include <SFML/Graphics/RectangleShape.hpp>
#include <SFML/Graphics/Text.hpp>

/**
 * @class DuelPauseState
 * @brief Minimal pause overlay for the local Duel mode.
 */
class DuelPauseState final : public State {
private:
    enum class Option {
        Resume,
        BackToModeSelection
    };

    sf::RectangleShape backgroundOverlay;
    sf::Text titleText;
    sf::Text resumeText;
    sf::Text backText;
    sf::Text hintText;
    Option selectedOption{Option::Resume};
    bool transitionPending{false};

    void moveSelection();
    void updateOptionStyles();
    void confirmSelection();
    void resumeDuel();
    void backToModeSelection();
    void playSelectSound();

public:
    DuelPauseState(GameStateManager& gsm, Systems::AssetManager& assets);
    ~DuelPauseState() override = default;

    void init() override;
    void handleInput(const sf::Event& event) override;
    void update(sf::Time dt) override;
    void render(sf::RenderWindow& window) override;
};
