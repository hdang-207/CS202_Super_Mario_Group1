#pragma once
#include "Core/CharacterType.hpp"
#include "States/State.hpp"
#include "Systems/MapParser.hpp"
#include "Systems/TileMap.hpp"
#include <set>

/**
 * @class PlayState
 * @brief Active gameplay state: loads the level, scrolls the camera, and handles avatar movement & gameplay loop.
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
    sf::Sprite avatarSprite;
    bool facingRight{true};
    float runAnimTimer{0.f};
    int currentRunStep{0};
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
     * Window events need no extra privileges on macOS.
     */
    std::set<sf::Keyboard::Key> heldKeys;

    /**
     * @brief Checks if a specific key is currently held down.
     * @param key Key code to check.
     * @return True if key is held, false otherwise.
     */
    bool holding(sf::Keyboard::Key key) const;

    /**
     * @brief Checks if move left control keys are held.
     * @return True if Left arrow or 'A' is held.
     */
    bool wantsLeft() const;

    /**
     * @brief Checks if move right control keys are held.
     * @return True if Right arrow or 'D' is held.
     */
    bool wantsRight() const;

    /**
     * @brief Checks if jump control keys are held.
     * @return True if Space, Up arrow, or 'W' is held.
     */
    bool wantsJump() const;

    /**
     * @brief Checks if boost/run control keys are held.
     * @return True if Left Shift or Right Shift is held.
     */
    bool wantsBoost() const;

    /**
     * @brief Returns current bounding rectangle of the test avatar.
     * @return FloatRect bounds of avatar.
     */
    sf::FloatRect avatarBounds() const;

    /**
     * @brief Respawns the test avatar at the player spawn location.
     */
    void respawnAvatar();

    /**
     * @brief Updates test avatar physics, movement, and collision resolution against TileMap.
     * @param dt Time elapsed since last update frame.
     */
    void moveAvatar(sf::Time dt);

    /**
     * @brief Centers camera on a target position within map boundaries.
     * @param target World coordinates to center camera on.
     */
    void centreCamera(sf::Vector2f target);

    /**
     * @brief Updates camera position to follow avatar.
     */
    void updateCamera();

    /**
     * @brief Pans camera manually during free-look mode.
     * @param dt Time elapsed since last update frame.
     */
    void panCamera(sf::Time dt);

    /**
     * @brief Draws free-look hint / instructions overlay.
     * @param window Target render window.
     */
    void drawFreeLookHint(sf::RenderWindow& window) const;

public:
    /**
     * @brief Constructor for PlayState.
     * @param gsm Reference to GameStateManager.
     * @param assets Reference to the central AssetManager.
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
     * @brief Renders gameplay scene onto the window.
     * @param window The target render window.
     */
    void render(sf::RenderWindow& window) override;
};
