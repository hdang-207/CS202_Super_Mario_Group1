#pragma once
#include "Core/CharacterType.hpp"
#include "States/State.hpp"
#include "Systems/MapParser.hpp"
#include "Systems/TileMap.hpp"
#include <set>

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

    // Free-look: F detaches the camera from the avatar so the level can be
    // scrolled through and inspected without playing it.
    bool freeLook{false};
    sf::Vector2f freeLookCentre;

    /**
     * Keys held right now, tracked from the window's key events.
     *
     * Deliberately not sf::Keyboard::isKeyPressed(): on macOS that goes through
     * the HID manager, which needs the Input Monitoring privilege. Without it the
     * call does not fail loudly, it just reports every key as up forever, and the
     * game silently stops responding to the movement keys. Window events need no
     * privilege at all.
     */
    std::set<sf::Keyboard::Key> heldKeys;

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
    bool holding(sf::Keyboard::Key key) const;  ///< True while @p key is down.
    bool wantsLeft() const;                     ///< Left arrow or A.
    bool wantsRight() const;                    ///< Right arrow or D.
    bool wantsJump() const;                     ///< Space, Up or W.
    bool wantsBoost() const;                    ///< Either Shift, for fast scrolling.

    /// @brief Current collision box of the test avatar, in world pixels.
    sf::FloatRect avatarBounds() const;

    /// @brief Steps the avatar one frame and resolves it against the tile map.
    void moveAvatar(sf::Time dt);

    /// @brief Puts the avatar back on the level's spawn tile.
    void respawnAvatar();

    /// @brief Points the camera at @p target, without ever leaving the level.
    void centreCamera(sf::Vector2f target);

    /// @brief Keeps the camera centred on the avatar without leaving the level.
    void updateCamera();

    /// @brief Scrolls the free-look camera with the left/right keys.
    void panCamera(sf::Time dt);

    /// @brief Draws the map-view banner, or the hint that says how to turn it on.
    void drawFreeLookHint(sf::RenderWindow& window) const;
};
