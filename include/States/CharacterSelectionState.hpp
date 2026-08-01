#pragma once
#include "States/State.hpp"

/**
 * @class CharacterSelectionState
 * @brief Represents the character selection screen of the game.
 *
 * Implements the State interface. It allows the player to choose their character
 * (Mario or Luigi) using keyboard numbers, or press 'B' to return to the main menu.
 */
class CharacterSelectionState : public State {
private:
    sf::Sprite bgSprite;           ///< Background menu sprite
    sf::RectangleShape darkOverlay;///< Darkening overlay for contrast
    sf::Text headerText;
    sf::Text marioOptionText;
    sf::Text luigiOptionText;
    sf::Text descText;             ///< Text display for character attributes
    sf::Text backHintText;

    sf::RectangleShape previewCard;///< Outer preview frame
    sf::Sprite previewSprite;      ///< Preview sprite for chosen character avatar
    int selectedIndex = 0;         ///< 0 for Mario, 1 for Luigi

public:
    /**
     * @brief Constructor for CharacterSelectionState.
     * @param gsm Reference to the GameStateManager.
     * @param assets Reference to the central AssetManager.
     */
    CharacterSelectionState(GameStateManager& gsm, Systems::AssetManager& assets);

    /**
     * @brief Destructor.
     */
    ~CharacterSelectionState() override = default;

    /**
     * @brief Initializes the character selection view, logging available options.
     */
    void init() override;

    /**
     * @brief Listens for keyboard inputs to select characters or go back to the menu.
     * @param event The event being polled.
     */
    void handleInput(const sf::Event& event) override;

    /**
     * @brief Updates any logic/animations for character selection.
     * @param dt Time elapsed since last frame.
     */
    void update(sf::Time dt) override;

    /**
     * @brief Renders the selection scene background and text instructions.
     * @param window Graphical window to draw into.
     */
    void render(sf::RenderWindow& window) override;
};
