#pragma once

#include "States/DuelArenas.hpp"
#include "States/State.hpp"

#include <SFML/Graphics/RectangleShape.hpp>
#include <SFML/Graphics/Text.hpp>

#include <cstddef>

/**
 * @class DuelPauseState
 * @brief Minimal pause overlay for the local Duel mode.
 */
class DuelPauseState final : public State {
private:
    enum class Option {
        Resume,
        ChangeArena,
        BackToModeSelection
    };

    sf::RectangleShape backgroundOverlay;
    sf::Text titleText;
    sf::Text resumeText;
    sf::Text changeArenaText;
    sf::Text backText;
    sf::Text hintText;
    Option selectedOption{Option::Resume};
    /// Arena the paused duel is running, so the selection screen reopens on it.
    std::size_t arenaChoice{duel::kRandomArena};
    bool transitionPending{false};

    void moveSelection(int step);
    void updateOptionStyles();
    void confirmSelection();
    void resumeDuel();
    void changeArena();
    void backToModeSelection();
    void playSelectSound();

public:
    DuelPauseState(
        GameStateManager& gsm,
        Systems::AssetManager& assets,
        std::size_t arenaChoice = duel::kRandomArena
    );
    ~DuelPauseState() override = default;

    void init() override;
    void handleInput(const sf::Event& event) override;
    void update(sf::Time dt) override;
    void render(sf::RenderWindow& window) override;
};
