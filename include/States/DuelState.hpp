#pragma once

#include "Combat/Bomb.hpp"
#include "Input/InputHandler.hpp"
#include "Entities/Bullet.hpp"
#include "Items/Item.hpp"
#include "Physics/PhysicsSystem.hpp"
#include "Player/Player.hpp"
#include "Player/PlayerAnimator.hpp"
#include "States/State.hpp"
#include "Systems/MapParser.hpp"
#include "Systems/TileMap.hpp"
#include "UI/EnergyBar.hpp"
#include <cstddef>
#include <memory>
#include <optional>
#include <random>
#include <set>
#include <string>
#include <vector>

/**
 * @class DuelState
 * @brief Runs the local two-player movement sandbox in the static Duel arena.
 */
class DuelState final : public State {
private:
    MapParser mapParser;
    TileMap tileMap;
    bool arenaLoaded{false};
    std::size_t arenaIndex{0};

    std::unique_ptr<entity::Player> playerOne;
    std::unique_ptr<entity::Player> playerTwo;
    InputHandler playerOneInput;
    InputHandler playerTwoInput;
    std::set<sf::Keyboard::Scancode> heldKeys;
    physics::PhysicsSystem physicsSystem;
    entity::PlayerAnimator playerOneAnimator;
    entity::PlayerAnimator playerTwoAnimator;
    bool playerOneFacingRight{true};
    bool playerTwoFacingRight{false};

    struct PlayerCombatState {
        float health{100.f};
        float energy{100.f};
        float damageProtectionRemaining{0.f};
        float starPowerRemaining{0.f};
    };
    PlayerCombatState playerOneCombat;
    PlayerCombatState playerTwoCombat;

    UI::EnergyBar playerOneHealthBar;
    UI::EnergyBar playerTwoHealthBar;
    UI::EnergyBar playerOneEnergyBar;
    UI::EnergyBar playerTwoEnergyBar;
    std::vector<entity::Bullet> playerOneFireballs;
    std::vector<entity::Bullet> playerTwoFireballs;
    std::optional<combat::Bomb> playerOneBomb;
    std::optional<combat::Bomb> playerTwoBomb;

    struct Blast {
        sf::Vector2f centre;
        float remaining{0.f};
    };
    std::vector<Blast> blasts;

    std::vector<std::unique_ptr<items::Item>> activePowerUps;
    std::vector<sf::Vector2f> powerUpSpawnPoints;
    std::mt19937 randomEngine{std::random_device{}()};
    float powerUpSpawnTimer{0.f};

    bool roundOver{false};
    int winnerPlayer{0};
    float victoryFanfareDelay{-1.f};

    int roundNumber{1};
    int roundsWonByPlayerOne{0};
    int roundsWonByPlayerTwo{0};
    bool matchOver{false};
    float roundTimeRemaining{0.f};
    float roundIntroRemaining{0.f};
    float fightBannerRemaining{0.f};
    int lastIntroTick{0};

    sf::RectangleShape skyBackground;
    sf::RectangleShape resultOverlay;
    sf::Text titleText;
    sf::Text scoreText;
    sf::Text countdownText;
    sf::Text timerText;
    sf::Text controlsText;
    sf::Text errorText;
    sf::Text resultText;
    sf::Text resultReasonText;
    sf::Text resultHintText;

    void spawnPlayers(const std::vector<sf::Vector2f>& spawns);
    std::vector<physics::AABB> solidAABBsOverlapping(
        const sf::FloatRect& bounds
    ) const;
    void updatePlayer(entity::Player& player, float seconds);
    void updatePlayerAnimation(
        entity::Player& player,
        const PlayerInput& input,
        entity::PlayerAnimator& animator,
        bool& facingRight,
        float damageProtectionRemaining,
        sf::Time dt
    );
    bool resolvePlayerStomps(
        const physics::AABB& previousPlayerOneBounds,
        const physics::AABB& previousPlayerTwoBounds
    );
    bool tryResolveStomp(
        entity::Player& attacker,
        entity::Player& victim,
        const physics::AABB& previousAttackerBounds,
        const physics::AABB& previousVictimBounds,
        PlayerCombatState& victimCombat,
        UI::EnergyBar& victimHealthBar,
        int attackerNumber
    );
    void startMatch();
    void startRound();
    void refreshScoreText();
    void refreshTimerText();
    void updateRoundTimer(float seconds);
    void setCountdownString(const std::string& text);
    void updateRoundIntro(float seconds);
    void finishRound(int winningPlayer, const std::string& reason);
    void updateRoundEndAudio(float seconds);
    void tryShootFireball(
        entity::Player& player,
        bool facingRight,
        const PlayerInput& input,
        PlayerCombatState& combatState,
        UI::EnergyBar& energyBar,
        std::vector<entity::Bullet>& fireballs,
        int playerNumber
    );
    void updateFireballs(sf::Time dt);
    void updateFireballGroup(
        std::vector<entity::Bullet>& fireballs,
        entity::Player& target,
        PlayerCombatState& targetCombat,
        UI::EnergyBar& targetHealthBar,
        int shooterNumber,
        sf::Time dt
    );
    void drawFireballs(sf::RenderWindow& window) const;
    void tryThrowBomb(
        entity::Player& player,
        bool facingRight,
        const PlayerInput& input,
        PlayerCombatState& combatState,
        UI::EnergyBar& energyBar,
        std::optional<combat::Bomb>& bombSlot,
        int playerNumber
    );
    void updateBombs(float seconds);
    void updateBombSlot(
        std::optional<combat::Bomb>& bombSlot,
        int throwerNumber,
        float seconds
    );
    void explodeBomb(sf::Vector2f centre, int throwerNumber);
    void updateBlasts(float seconds);
    void drawBombs(sf::RenderWindow& window) const;
    void updateStarPower(float seconds);
    bool resolveStarContact();
    bool damagePlayer(
        int victimNumber,
        float damage,
        int creditedWinner,
        const std::string& knockoutReason
    );
    void resetPowerUpTimer();
    void buildPowerUpSpawnPoints();
    void spawnRandomPowerUp();
    void updatePowerUps(sf::Time dt);
    void collectPowerUps();
    void applyPowerUp(entity::Player& player, entity::PlayerAnimator& animator,
                      PlayerCombatState& combatState,
                      UI::EnergyBar& energyBar,
                      items::Item& powerUp);
    static entity::PlayerForm formOf(const entity::Player& player);
    static sf::FloatRect playerBounds(const entity::Player& player);
    static sf::Vector2f feetCentre(const entity::Player& player);

public:
    DuelState(GameStateManager& gsm, Systems::AssetManager& assets);
    ~DuelState() override = default;

    void init() override;
    void handleInput(const sf::Event& event) override;
    void update(sf::Time dt) override;
    void render(sf::RenderWindow& window) override;
    void pause() override;
};
