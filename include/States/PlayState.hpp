#pragma once
#include "Core/CharacterType.hpp"
#include "States/State.hpp"
#include "Systems/MapParser.hpp"
#include "Systems/TileMap.hpp"

/**
 * @class PlayState
 * @brief Active gameplay state: loads the level, scrolls the camera and runs the game loop.
 */
class PlayState : public State {
private:
    CharacterType selectedCharacter;
    MapParser mapParser;
    TileMap tileMap;
    sf::View camera;

    // === TEMPORARY test avatar =============================================
    // A plain rectangle with just enough kinematics to walk, jump and stand on
    // the tile map, so the level can be played and checked right now. It is a
    // stand-in for the real Player/Mario/Luigi hierarchy (Physics Lead) and is
    // meant to be deleted once that lands - TileMap below is the piece that
    // stays, and is what the real physics code should collide against.
    sf::RectangleShape avatar;
    sf::Vector2f avatarPos;
    sf::Vector2f avatarVelocity;
    bool onGround{false};
    bool jumpHeld{false};
    // =======================================================================

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

private:
    /// @brief Current collision box of the test avatar, in world pixels.
    sf::FloatRect avatarBounds() const;

    /// @brief Steps the avatar one frame and resolves it against the tile map.
    void moveAvatar(sf::Time dt);

    /// @brief Puts the avatar back on the level's spawn tile.
    void respawnAvatar();

    /// @brief Keeps the camera centred on the avatar without leaving the level.
    void updateCamera();
};
