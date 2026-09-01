#pragma once

#include "Core/GameMode.hpp"
#include "States/State.hpp"

/**
 * @class GameModeSelectionState
 * @brief Allows the player to choose Normal, Nightfall, Inferno, or Duel mode.
 *
 * Normal, Nightfall, and Inferno continue to character/world selection.
 * Duel enters its dedicated local multiplayer state directly.
 */
class GameModeSelectionState : public State {
public:
    GameModeSelectionState(
        GameStateManager& gsm,
        Systems::AssetManager& assets,
        GameMode initiallySelected = GameMode::Normal
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
        Inferno,
        Duel
    };

    SelectionOption selectedOption{SelectionOption::Normal};

    sf::Sprite bgSprite;
    sf::RectangleShape darkOverlay;
    sf::Text headerText;

    sf::Text normalText;
    sf::Text nightfallText;
    sf::Text infernoText;
    sf::Text duelText;

    sf::Text normalDesc;
    sf::Text nightfallDesc;
    sf::Text infernoDesc;
    sf::Text duelDesc;

    sf::Text hintText;

    void selectPreviousOption();
    void selectNextOption();
    void chooseOption(SelectionOption option);
    void confirmSelection();
};