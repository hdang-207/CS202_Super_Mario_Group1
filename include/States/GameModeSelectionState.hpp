#pragma once

#include "States/State.hpp"

/**
 * @class GameModeSelectionState
 * @brief Allows the player to choose between Normal and Nightfall game modes.
 *
 * Single Responsibility: Only handles the game mode selection UI.
 * Appears after pressing "START NEW GAME" and before character selection.
 */
class GameModeSelectionState : public State {
public:
    GameModeSelectionState(GameStateManager& gsm, Systems::AssetManager& assets);
    ~GameModeSelectionState() override = default;

    void init() override;
    void handleInput(const sf::Event& event) override;
    void update(sf::Time dt) override;
    void render(sf::RenderWindow& window) override;

private:
    int selectedIndex{0};  ///< 0 = Normal, 1 = Nightfall

    sf::Sprite bgSprite;
    sf::RectangleShape darkOverlay;
    sf::Text headerText;
    sf::Text normalText;
    sf::Text nightfallText;
    sf::Text normalDesc;
    sf::Text nightfallDesc;
    sf::Text hintText;
};
