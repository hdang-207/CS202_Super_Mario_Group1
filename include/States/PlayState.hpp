#pragma once
#include "Core/CharacterType.hpp"
#include "States/State.hpp"
#include "Systems/MapParser.hpp"
#include "Systems/TileMap.hpp"
#include "Systems/SaveManager.hpp"
#include <cstddef>
#include <random>
#include "UI/HUD.hpp"
#include <set>
#include <vector>

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
    int currentLevel{1};
    int score{0};
    int coins{0};
    int lives{3};
    UI::HUD hud;

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

    struct CoinPop {
        sf::Vector2f position;
        float velocityY;
        float elapsed;
    };
    std::vector<CoinPop> coinPops;

    enum class MushroomState { Emerging, Moving };
    struct MushroomEntity {
        sf::Vector2f blockPosition;
        sf::Vector2f position;
        sf::Vector2f velocity;
        MushroomState state;
        float elapsed;
    };
    std::vector<MushroomEntity> mushrooms;

    enum class EnemyKind {
        Goomba,
        BlueKoopa
    };

    struct WalkingEnemy {
        EnemyKind kind;
        sf::Vector2f position;
        sf::Vector2f velocity;
        bool active{false};
        bool alive{true};
        float animationElapsed{0.f};
        int animationFrame{0};
    };
    std::vector<WalkingEnemy> walkingEnemies;

    enum class BlockReward {
        Coin,
        Mushroom
    };
    std::vector<BlockReward> blockRewards;
    std::size_t nextBlockReward{0};
    std::mt19937 rewardRandom{std::random_device{}()};
    // =======================================================================

    // Free-look: F detaches the camera from the avatar so the level can be
    // scrolled through and inspected without playing it.
    bool freeLook{false};
    bool isPaused{false};
    bool transitionPending{false};
    sf::Vector2f freeLookCentre;
    float maxCameraCenterX{0.f}; ///< Maximum X position camera center has reached (SMB 1985 one-way scroll lock)

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

    /// @brief Applies one life loss and either restarts the level or opens Game Over.
    void handlePlayerDeath();

    /// @brief Starts the background theme matching the current level.
    void playLevelMusic();

    /**
     * @brief Respawns the test avatar at the player spawn location.
     */
    void respawnAvatar();

    /**
     * @brief Loads and builds one numbered map file.
     * @param level Internal level index: 1 loads level1.txt and 2 loads level1-2.txt.
     * @return True when the map was loaded and built successfully.
     */
    bool loadLevel(int level);

    /**
     * @brief Enters level 2 when the avatar reaches level 1's warp pipe.
     * @return True when a transition happened during this frame.
     */
    bool tryEnterNextLevel();

    /// @brief Starts the short coin animation above an activated question block.
    void spawnCoinPop(sf::Vector2f blockPosition);

    /// @brief Advances and removes temporary question-block coins.
    void updateCoinPops(sf::Time dt);

    /// @brief Draws all temporary coins in world space.
    void drawCoinPops(sf::RenderWindow& window) const;

    /// @brief Builds a random reward bag with at least two coins and two mushrooms.
    void prepareQuestionBlockRewards();

    /// @brief Returns the reward assigned to the next activated question block.
    BlockReward takeNextQuestionBlockReward();

    /// @brief Starts a mushroom emerging from an activated question block.
    void spawnMushroom(sf::Vector2f blockPosition);

    /// @brief Updates mushroom physics and collision
    void updateMushrooms(sf::Time dt);

    /// @brief Draws all emerged mushrooms in world space.
    void drawMushrooms(sf::RenderWindow& window) const;

    /// @brief Creates Goombas and Blue Koopas from their map markers.
    void spawnWalkingEnemies();

    /// @brief Updates walking-enemy activation, movement, gravity, and collisions.
    bool updateWalkingEnemies(sf::Time dt);

    /// @brief Draws all animated walking enemies.
    void drawWalkingEnemies(sf::RenderWindow& window) const;

    /**
     * @brief Updates test avatar physics, movement, and collision resolution against TileMap.
     * @param dt Time elapsed since last update frame.
     */
    bool moveAvatar(sf::Time dt);

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
    PlayState(GameStateManager& gsm, Systems::AssetManager& assets, CharacterType character);

    /**
     * @brief Constructor for PlayState using existing progress.
     * @param gsm Reference to GameStateManager.
     * @param assets Reference to the central AssetManager.
     * @param data SaveData containing current progress.
     */
    PlayState(GameStateManager& gsm, Systems::AssetManager& assets, const SaveData& data);

    /**
     * @brief Destructor.
     */
    ~PlayState() override;

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

    /**
     * @brief Gets current SaveData progress snapshot.
     */
    SaveData getSaveData() const;

    /**
     * @brief Performs quick load from savegame.txt.
     */
    bool quickLoad();
};
