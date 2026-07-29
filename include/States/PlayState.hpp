#pragma once
#include "States/State.hpp"
#include "Core/CharacterType.hpp"
#include "Systems/MapParser.hpp"
#include <vector>

/**
 * @class PlayState
 * @brief Active gameplay state where the map is loaded and character moves.
 */
class PlayState : public State {
private:
    CharacterType selectedCharacter;
    MapParser mapParser;
    sf::Sprite bgSprite; ///< Sky + clouds background image.
    std::vector<sf::Sprite> groundTiles; ///< One tile sprite per '#' cell in the map grid.

public:
    /**
     * @brief Constructor for PlayState.
     * @param gsm Reference to GameStateManager.
     * @param character The character selected by the player (Mario / Luigi).
     */
    PlayState(GameStateManager& gsm, Systems::AssetManager& assets, CharacterType character);

    /**
     * @brief Destructor.
     */
    ~PlayState() override = default;

    /**
     * @brief Initializes gameplay resources and loads level map.
     */
    void init() override;

    /**
     * @brief Handles user input events during gameplay.
     * @param event The SFML event.
     */
    void handleInput(const sf::Event& event) override;

    /**
     * @brief Updates gameplay physics and entities.
     * @param dt Time elapsed since last frame.
     */
    void update(sf::Time dt) override;

    /**
     * @brief Draws level map and player onto window.
     * @param window Graphical window to render into.
     */
    void render(sf::RenderWindow& window) override;
};
