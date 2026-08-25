#pragma once
#include "Core/CharacterType.hpp"
#include "Combat/Bomb.hpp"
#include "Entities/Bullet.hpp"
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

/**
 * @class PlayState
 * @brief Active gameplay state: Scene Coordinator between TileMap, EntityManager, PhysicsSystem, CameraSystem, and HUD.
 */
class PlayState : public State {
private:
    CharacterType selectedCharacter;
    MapParser mapParser;
    TileMap tileMap;
    int currentLevel{1};
    int score{0};
    int coins{0};
    int lives{3};
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

    bool isPaused{false};
    bool transitionPending{false};
    std::set<sf::Keyboard::Scancode> heldKeys;

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

    void spawnWalkingEnemies();
    void spawnPiranhas();
    void spawnMovingPlatforms();
    void updateMovingPlatforms(sf::Time dt);
    void drawMovingPlatforms(sf::RenderWindow& window) const;
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
