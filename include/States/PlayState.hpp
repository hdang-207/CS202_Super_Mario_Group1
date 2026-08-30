#pragma once
#include "Core/CharacterType.hpp"
#include "Combat/Bomb.hpp"
#include "Entities/Bullet.hpp"
#include "Entities/Hammer.hpp"
#include "Entities/Entity.hpp"
#include "Entities/EntityFactory.hpp"
#include "Entities/EntityManager.hpp"
#include "Input/InputHandler.hpp"
#include "States/State.hpp"
#include "Systems/CameraSystem.hpp"
#include "Systems/MapParser.hpp"
#include "Systems/TileMap.hpp"
#include "Systems/SaveManager.hpp"
#include "Physics/PhysicsSystem.hpp"
#include "Physics/PhysicsBody.hpp"
#include "Player/Player.hpp"
#include "Player/Mario.hpp"
#include "Player/PlayerAnimator.hpp"
#include "Player/Luigi.hpp"
#include "UI/HUD.hpp"
#include <cstddef>
#include <random>
#include <optional>
#include <set>
#include <vector>
#include "UI/ConsoleOverlay.hpp"
#include "Core/CommandParser.hpp"

/**
 * @class PlayState
 * @brief Active gameplay state: Scene Coordinator between TileMap, EntityManager, PhysicsSystem, CameraSystem, and HUD.
 */
class PlayState : public State {
public:
    friend class Core::CommandParser;
private:
    CharacterType selectedCharacter;
    MapParser mapParser;
    TileMap tileMap;
    int currentLevel{1};
    int score{0};
    int coins{0};
    int lives{3};
    int levelCoinsCollected{0};
    bool world23AllCoinsCollected{false};
    UI::HUD hud;

    InputHandler inputHandler;

    // Subsystems
    physics::PhysicsSystem m_physicsSystem;
    entity::EntityManager m_entityManager;
    Systems::CameraSystem m_cameraSystem;
    
    // Encapsulated Player entity
    std::unique_ptr<entity::Player> m_player;

    sf::RectangleShape avatar;

    /// Owns the avatar's artwork: which form, which pose, and every flash.
    entity::PlayerAnimator animator;

    bool facingRight{true};
    float invincibleTimer{0.f};
    float starPowerRemaining{0.f};
    float damageProtectionRemaining{0.f};
    bool swimButtonHeld{false};

    /// Four source frames make the World 2-2 surface ripple instead of being
    /// displayed side-by-side as four different neighbouring water tiles.
    sf::Time waterAnimationElapsed{sf::Time::Zero};
    int waterAnimationFrame{0};

    /// World 2-3 launches red fish from below the bridge instead of placing
    /// static enemy markers in the map file.
    float flyingCheepSpawnTimer{0.8f};

    /**
     * The original game does not cut away the instant Mario is hit: he stops,
     * hangs there for a beat, hops, and only then drops off the bottom of the
     * screen. Everything else is frozen while this plays.
     */
    struct DeathSequence {
        bool active{false};
        float elapsed{0.f};
        float velocityY{0.f};
    };
    DeathSequence death;

    std::optional<combat::Bomb> activeBomb;

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

    struct CoinBlockUsage {
        int col;
        int row;
        int coinsReleased;
    };
    /// Tracks repeat hits on 'b' blocks; each one can release up to ten coins.
    std::vector<CoinBlockUsage> coinBlockUsages;

    struct BrickDebris {
        sf::Vector2f position;
        sf::Vector2f velocity;
        float elapsed;
        int frame;
    };
    std::vector<BrickDebris> brickDebris;

    struct GrowingVineEntity {
        sf::Vector2f blockPosition;
        float elapsed;
    };
    std::vector<GrowingVineEntity> growingVines;

    /// The climb is scripted rather than driven by input: the vine is a fixed
    /// ride up, playing the sheet's two gripping poses, and the paired C+
    /// marker receives the player at the top in Coin Heaven.
    bool climbingCoinHeavenVine{false};
    bool insideCoinHeaven{false};
    float coinHeavenClimbElapsed{0.f};
    sf::Vector2f coinHeavenClimbStart;
    sf::Vector2f coinHeavenVinePosition;

    /**
     * @brief How a lift travels.
     *
     * World 1-3's four lifts ride up and down. World 3-3 adds two pairs slung
     * over a pulley: standing on one end lowers it
     * and hauls the other end up by the same amount, until the rising end
     * reaches its wheel.
     */
    enum class LiftMotion { Horizontal, Vertical, Balance };

    struct MovingPlatform {
        sf::Vector2f position;
        sf::Vector2f previousPosition;
        sf::Vector2f origin;
        sf::Vector2f velocity;
        LiftMotion motion{LiftMotion::Horizontal};
        int partner{-1};      ///< Other end of the pulley, -1 when it has none.
        float ropeTopY{0.f};  ///< Row the pulley's rope hangs from.
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

    bool isPaused{false};
    bool transitionPending{false};

    /// True while the avatar is inside the stage's hidden room, which swaps the
    /// music and decides which of the two warp pipes is listening for input.
    bool insideSecretRoom{false};
    std::set<sf::Keyboard::Scancode> heldKeys;

    UI::ConsoleOverlay m_console;
    std::unique_ptr<Core::CommandParser> m_commandParser;
    bool m_flyMode{false};
    bool m_godMode{false};
    bool m_destroyerMode{false};
    bool m_musicLocked{false};

    std::vector<physics::AABB> getSolidAABBsOverlapping(const sf::FloatRect& bounds) const;
    sf::FloatRect avatarBounds() const;
    void syncAvatarPowerVisuals();

    /// @brief Which sprite sheet the current power stack should be drawn from.
    [[nodiscard]] entity::PlayerForm currentPlayerForm() const;

    /// @brief World position the avatar's feet stand on, for the animator.
    [[nodiscard]] sf::Vector2f avatarFeetCentre() const;

    /// @brief Chooses this frame's pose from the physics body and surroundings.
    void updateAvatarAnimation(sf::Time dt, bool underwater);

    /// @brief Starts the death animation; the level keeps running until it ends.
    void handlePlayerDeath();

    /// @brief Advances the death animation and leaves the level at its end.
    void updateDeathSequence(sf::Time dt);

    void playLevelMusic();
    void respawnAvatar();
    bool loadLevel(int level);
    bool tryEnterWorld22WaterPipe();

    /// @brief Ducking on the marked pipe drops the avatar into the hidden room.
    bool tryEnterSecretRoom();

    /// @brief Walking into the hidden room's side pipe returns to the stage.
    bool tryLeaveSecretRoom();

    /// @brief Starts climbing a fully-grown World 3-1 vine while Up/Jump is held.
    bool tryStartCoinHeavenClimb();

    /// @brief Advances the short climb and warps to the C+ vine when complete.
    bool updateCoinHeavenClimb(sf::Time dt);

    /// @brief Returns from the D+ fall at Coin Heaven's open right edge.
    bool tryLeaveCoinHeaven();

    /// @brief Drops the avatar into a map cell and takes the camera with it.
    void warpAvatarTo(sf::Vector2f cell);

    bool tryEnterNextLevel();

    void spawnCoinPop(sf::Vector2f blockPosition);
    void prepareItemBlockRewards();
    BlockReward takeNextItemBlockReward();
    void spawnMushroom(sf::Vector2f blockPosition, items::MushroomKind kind = items::MushroomKind::Super);
    void spawnFireFlower(sf::Vector2f blockPosition);
    void spawnStar(sf::Vector2f blockPosition);
    bool spawnGrowingVine(sf::Vector2f blockPosition);
    void updateGrowingVines(sf::Time dt);
    void drawGrowingVines(sf::RenderWindow& window) const;

    struct ExplosionEntity {
        sf::Vector2f position;
        float elapsed;
        int currentFrame;
    };
    std::vector<entity::Bullet> bullets;

    /// Hammers in flight. They are the Bros' half of the fight, so the level
    /// owns them beside the fireballs rather than as entities.
    std::vector<entity::Hammer> hammers;
    std::vector<ExplosionEntity> explosions;

    void spawnBullet();
    void updateBullets(sf::Time dt);
    void drawBullets(sf::RenderWindow& window) const;
    void spawnExplosion(sf::Vector2f position);
    void updateExplosions(sf::Time dt);
    void drawExplosions(sf::RenderWindow& window) const;

    void spawnBomb();
    void updateBomb(sf::Time dt);
    void explodeBomb(sf::Vector2f center);
    void drawBomb(sf::RenderWindow& window) const;

    void updateBlocks(sf::Time dt);
    void drawBlocks(sf::RenderWindow& window) const;

    /// @brief Enemies that react to where the avatar is: Piranha Plants stay
    ///        down while he stands on their pipe, Hammer Bros turn to face him
    ///        and hand the level a hammer to throw.
    void updateEnemyReactions();

    void updateHammers(sf::Time dt);
    void drawHammers(sf::RenderWindow& window) const;

    void spawnWalkingEnemies();
    void spawnAquaticEnemies();
    void updateAquaticEnemyTargets();
    void updateFlyingCheepSpawner(sf::Time dt);
    void spawnPiranhas();
    void spawnMovingPlatforms();
    void updateMovingPlatforms(sf::Time dt);
    void drawMovingPlatforms(sf::RenderWindow& window) const;

    /// @brief True while the avatar's feet rest on a lift standing at @p platformPos.
    [[nodiscard]] bool isStandingOnPlatform(sf::Vector2f platformPos) const;

    /// @brief Height the rope of the pulley above @p platformPos hangs from.
    [[nodiscard]] float pulleyRopeTop(sf::Vector2f platformPos) const;
    void spawnTrampolines();
    void updateTrampolines(sf::Time dt);
    void drawTrampolines(sf::RenderWindow& window) const;

    bool moveAvatar(sf::Time dt);

    void registerEvents();
public:
    PlayState(GameStateManager& gsm, Systems::AssetManager& assets, CharacterType character);
    PlayState(GameStateManager& gsm, Systems::AssetManager& assets, const SaveData& data);
    ~PlayState() override;

    void init() override;
    void handleInput(const sf::Event& event) override;
    void update(sf::Time dt) override;
    void render(sf::RenderWindow& window) override;
    void pause() override;
    void resume() override;

    SaveData getSaveData() const;
    bool quickLoad();
};
