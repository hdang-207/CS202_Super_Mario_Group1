#pragma once
#include "Core/CharacterType.hpp"
#include "Entities/Bullet.hpp"
#include "Entities/Entity.hpp"
#include "Entities/EntityFactory.hpp"
#include "Input/InputHandler.hpp"
#include "Items/FireFlower.hpp"
#include "Items/Mushroom.hpp"
#include "Items/Star.hpp"
#include "States/State.hpp"
#include "Systems/MapParser.hpp"
#include "Systems/TileMap.hpp"
#include "Systems/SaveManager.hpp"
#include "Physics/PhysicsSystem.hpp"
#include "Physics/PhysicsBody.hpp"
#include "Player/Player.hpp"
#include "Player/Mario.hpp"
#include "Player/Luigi.hpp"
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

    InputHandler inputHandler;

    // === PHYSICS SYSTEM & ENTITY INTEGRATION (Phase 3 & 4) =================
    physics::PhysicsSystem m_physicsSystem;
    
    // Encapsulated Player entity (Mario / Luigi subclass instance)
    std::unique_ptr<entity::Player> m_player;

    /**
     * @brief Helper to convert TileMap FloatRect overlaps to physics::AABB list.
     */
    std::vector<physics::AABB> getSolidAABBsOverlapping(const sf::FloatRect& bounds) const;
    // =======================================================================

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
    float invincibleTimer{0.f}; ///< Brief invincibility after Fire downgrade
    float starPowerRemaining{0.f}; ///< Starman duration; touching enemies defeats them.

    float damageProtectionRemaining{0.f};

    struct CoinPop {
        sf::Vector2f position;
        float velocityY;
        float elapsed;
    };
    std::vector<CoinPop> coinPops;

    std::vector<items::Mushroom> mushrooms;
    std::vector<items::FireFlower> fireFlowers;
    std::vector<items::Star> stars;

    struct GrowingVineEntity {
        sf::Vector2f blockPosition;
        float elapsed;
    };
    std::vector<GrowingVineEntity> growingVines;

    std::vector<entity::Bullet> bullets;
    int availableBullets{3};
    float shootCooldownRemaining{0.f};
    std::vector<float> ammoRechargeTimers;

    struct ExplosionEntity {
        sf::Vector2f position;
        float elapsed;
        int currentFrame;
    };
    std::vector<ExplosionEntity> explosions;

    enum class EnemyKind {
        Goomba,
        BlueKoopa,
        GreenKoopa,
        GreenParatroopa
    };

    enum class EnemyState {
        Walking,
        ShellIdle,
        ShellMoving
    };

    struct WalkingEnemy {
        EnemyKind kind;
        sf::Vector2f position;
        sf::Vector2f velocity;
        bool active{false};
        bool alive{true};
        float animationElapsed{0.f};
        int animationFrame{0};
        EnemyState state{EnemyState::Walking};
    };
    std::vector<WalkingEnemy> walkingEnemies;

    struct PiranhaEntity {
        sf::Vector2f position;
        float pipeTopY;
        float elapsed{0.f};
        float exposure{1.f};
        bool alive{true};
    };
    std::vector<PiranhaEntity> piranhas;

    struct MovingPlatform {
        sf::Vector2f position;
        sf::Vector2f previousPosition;
        float originX;
        float velocityX;
    };
    std::vector<MovingPlatform> movingPlatforms;

    enum class TrampolineState { Normal, Compressed, Launch };
    struct TrampolineEntity {
        float x;
        float bottomY;
        TrampolineState state{TrampolineState::Normal};
        float elapsed{0.f};
        bool carryingPlayer{false};
    };
    std::vector<TrampolineEntity> trampolines;

    struct DeadEnemy {
        EnemyKind kind;
        sf::Vector2f position;
        sf::Vector2f velocity;
        float elapsed;
    };
    std::vector<DeadEnemy> deadEnemies;

    enum class BlockReward {
        Coin,
        Mushroom,
        FireFlower
    };
    std::vector<BlockReward> blockRewards;
    std::size_t nextBlockReward{0};
    std::mt19937 rewardRandom{std::random_device{}()};
    
    struct BouncingBlock {
        int col, row;
        char originalSymbol;
        sf::Vector2f position;
        float startY;
        float velocityY;
        bool active{true};
    };
    std::vector<BouncingBlock> bouncingBlocks;

    struct BrickDebris {
        sf::Vector2f position;
        sf::Vector2f velocity;
        float elapsed;
        int frame;
    };
    std::vector<BrickDebris> brickDebris;

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
     * @brief Returns current bounding rectangle of the test avatar.
     * @return FloatRect bounds of avatar.
     */
    sf::FloatRect avatarBounds() const;

    /// @brief Syncs PlayState-owned visuals with Player-owned form and collider state.
    void syncAvatarPowerVisuals();

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
     * @param level Linear campaign index converted to level<world>-<stage>.txt.
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

    /// @brief Builds a random reward bag for question and hidden item blocks.
    void prepareItemBlockRewards();

    /// @brief Returns the reward assigned to the next activated item block.
    BlockReward takeNextItemBlockReward();

    /// @brief Starts a Super or 1-Up mushroom emerging from an activated block.
    void spawnMushroom(sf::Vector2f blockPosition,
                       items::MushroomKind kind = items::MushroomKind::Super);

    /// @brief Updates mushroom physics and collision
    void updateMushrooms(sf::Time dt);

    /// @brief Draws all emerged mushrooms in world space.
    void drawMushrooms(sf::RenderWindow& window) const;

    void spawnFireFlower(sf::Vector2f blockPosition);
    void updateFireFlowers(sf::Time dt);
    void drawFireFlowers(sf::RenderWindow& window) const;

    /// @brief Releases, moves, collects, and draws the World 2-1 Starman.
    void spawnStar(sf::Vector2f blockPosition);
    void updateStars(sf::Time dt);
    void drawStars(sf::RenderWindow& window) const;

    /// @brief Grows the Coin Heaven vine upward from its special brick.
    bool spawnGrowingVine(sf::Vector2f blockPosition);
    void updateGrowingVines(sf::Time dt);
    void drawGrowingVines(sf::RenderWindow& window) const;

    void spawnBullet();
    void updateBullets(sf::Time dt);
    void drawBullets(sf::RenderWindow& window) const;

    void spawnExplosion(sf::Vector2f position);
    void updateExplosions(sf::Time dt);
    void drawExplosions(sf::RenderWindow& window) const;

    void updateDeadEnemies(sf::Time dt);
    void drawDeadEnemies(sf::RenderWindow& window) const;

    void updateBlocks(sf::Time dt);
    void drawBlocks(sf::RenderWindow& window) const;

    /// @brief Creates Goombas, Koopas, and Paratroopas from their map markers.
    void spawnWalkingEnemies();

    /// @brief Updates walking-enemy activation, movement, gravity, and collisions.
    bool updateWalkingEnemies(sf::Time dt);

    /// @brief Draws all animated walking enemies.
    void drawWalkingEnemies(sf::RenderWindow& window) const;

    /// @brief Creates Piranha Plants from 'R' markers above pipe cells.
    void spawnPiranhas();

    /// @brief Animates Piranhas and applies their contact damage.
    bool updatePiranhas(sf::Time dt);

    /// @brief Draws Piranhas before the pipe tiles so retracted parts are hidden.
    void drawPiranhas(sf::RenderWindow& window) const;

    /// @brief Creates horizontal lifts from every 'L' marker in the map.
    void spawnMovingPlatforms();

    /// @brief Moves lifts between their horizontal endpoints and carries Mario.
    void updateMovingPlatforms(sf::Time dt);

    /// @brief Draws the three-tile moving lifts used by World 1-3.
    void drawMovingPlatforms(sf::RenderWindow& window) const;

    /// @brief Creates World 2-1 trampolines from 'D' map markers.
    void spawnTrampolines();

    /// @brief Compresses and launches Mario when he lands on a trampoline.
    void updateTrampolines(sf::Time dt);

    /// @brief Draws the normal, compressed, or launch trampoline frame.
    void drawTrampolines(sf::RenderWindow& window) const;

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

    /// @brief Clears event-held controls when a menu overlay takes focus.
    void pause() override;

    /// @brief Resumes with a neutral input state to prevent stuck movement keys.
    void resume() override;

    /**
     * @brief Gets current SaveData progress snapshot.
     */
    SaveData getSaveData() const;

    /**
     * @brief Performs quick load from savegame.txt.
     */
    bool quickLoad();
};
