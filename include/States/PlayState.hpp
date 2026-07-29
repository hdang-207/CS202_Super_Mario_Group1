#pragma once
#include "States/State.hpp"
#include "Core/CharacterType.hpp"
#include "Systems/MapParser.hpp"

/**
 * @class PlayState
 * @brief Active gameplay state where the map is loaded and character moves.
 */
class PlayState : public State {
private:
    CharacterType selectedCharacter;
    MapParser mapParser;
    sf::Sprite bgSprite; ///< Level tilemap background image.
    float cameraX{0.f}; ///< Horizontal scroll offset (in pixels) of the camera into the level.
    float levelWidth{0.f}; ///< Full scrolled width of the level (scaled tilemap width).

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
