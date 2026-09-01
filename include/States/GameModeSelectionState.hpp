#pragma once

#include "States/State.hpp"

/**
 * @class GameModeSelectionState
 * @brief Allows the player to choose Normal, Nightfall, or Duel mode.
 *
 * Single Responsibility: Only handles the game mode selection UI.
 * Appears after pressing "START NEW GAME" and before character selection.
 */
class GameModeSelectionState : public State {
public:
    GameModeSelectionState(
        GameStateManager& gsm,
        Systems::AssetManager& assets,
        bool nightfallInitiallySelected = false
    );
    ~GameModeSelectionState() override = default;

    void init() override;
    void handleInput(const sf::Event& event) override;
    void update(sf::Time dt) override;
    void render(sf::RenderWindow& window) override;

private:
    enum class SelectionOption {
        Normal,
        Nightfall,
        Duel
    };

    SelectionOption selectedOption{SelectionOption::Normal};

    sf::Sprite bgSprite;
    sf::RectangleShape darkOverlay;
    sf::Text headerText;
    sf::Text normalText;
    sf::Text nightfallText;
    sf::Text duelText;
    sf::Text normalDesc;
    sf::Text nightfallDesc;
    sf::Text duelDesc;
    sf::Text hintText;

    void selectPreviousOption();
    void selectNextOption();
    void chooseOption(SelectionOption option);
    void confirmSelection();
};
