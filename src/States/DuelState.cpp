#include "States/DuelState.hpp"

#include "Core/Config.hpp"
#include "Core/CharacterType.hpp"
#include "Items/FireFlower.hpp"
#include "Items/ManaOrb.hpp"
#include "Items/Mushroom.hpp"
#include "Items/Star.hpp"
#include "Physics/Broadphase.hpp"
#include "States/DuelPlayerCollision.hpp"
#include "States/DuelPauseState.hpp"
#include "States/DuelRules.hpp"
#include "States/GameStateManager.hpp"
#include "Systems/ResourcePath.hpp"
#include "Systems/SoundController.hpp"
#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <cmath>
#include <iostream>
#include <memory>
#include <string>
#include <vector>

namespace {
constexpr float kGravity = 2400.f;
constexpr float kMaxFallSpeed = 1400.f;
constexpr float kBroadphaseSafetyMargin = 1.f;
constexpr float kMovementAnimationThreshold = 10.f;
constexpr float kStompBounceSpeed = 650.f;
constexpr float kDamageProtectionDuration = 0.9f;
constexpr float kFireballSpeed = 520.f;
constexpr float kFireballLifetime = 2.5f;
constexpr float kMinimumPowerUpDelay = 4.f;
constexpr float kMaximumPowerUpDelay = 7.f;
constexpr float kFirstPowerUpDelay = 2.f;
constexpr float kVictoryFanfareDelay = 1.f;
constexpr int kRoundsToWinMatch = 2;      // best of three
constexpr float kRoundIntroDuration = 3.f;
constexpr float kFightBannerDuration = 0.7f;
constexpr float kRoundTimeLimit = 90.f;
constexpr float kLowTimeWarning = 10.f;
struct ArenaDefinition {
    const char* file;
    const char* name;
};

// Every arena is mirrored left to right so neither player gets the better side.
constexpr ArenaDefinition kArenas[] = {
    {"assets/maps/duel_arena.txt", "CLASSIC"},
    {"assets/maps/duel_arena_towers.txt", "SKY TOWERS"},
    {"assets/maps/duel_arena_chasm.txt", "CHASM"},
};
constexpr std::size_t kArenaCount = std::size(kArenas);

constexpr unsigned int kResultTextSize = 38;
constexpr float kResultTextPadding = 60.f;
constexpr float kBombBlastRadius = 66.f;
constexpr float kBlastEffectDuration = 0.35f;
constexpr float kBlastKnockbackX = 300.f;
constexpr float kBlastKnockbackY = 340.f;
constexpr float kStarContactKnockback = 260.f;
// Swap this for a dedicated battle track once the group records one.
constexpr const char* kDuelMusicPath = "assets/audio/InfernoThemeWorld1.mp3";
constexpr std::size_t kMaximumActivePowerUps = 6;
constexpr sf::Vector2f kPlayerOneHealthPosition{48.f, 48.f};
constexpr sf::Vector2f kPlayerTwoHealthPosition{
    Config::kViewWidth - 48.f - UI::EnergyBar::width(),
    48.f
};
constexpr sf::Vector2f kPlayerOneEnergyPosition{48.f, 110.f};
constexpr sf::Vector2f kPlayerTwoEnergyPosition{
    Config::kViewWidth - 48.f - UI::EnergyBar::width(),
    110.f
};
}

DuelState::DuelState(GameStateManager& gsm, Systems::AssetManager& assets)
    : State(gsm, assets),
      playerOneInput(PlayerKeyBindings::duelPlayerOne()),
      playerTwoInput(PlayerKeyBindings::duelPlayerTwo()),
      physicsSystem(kGravity, kMaxFallSpeed),
      titleText(assets.getFont("MarioFont")),
      scoreText(assets.getFont("MarioFont")),
      countdownText(assets.getFont("MarioFont")),
      timerText(assets.getFont("MarioFont")),
      controlsText(assets.getFont("MarioFont")),
      errorText(assets.getFont("MarioFont")),
      resultText(assets.getFont("MarioFont")),
      resultReasonText(assets.getFont("MarioFont")),
      resultHintText(assets.getFont("MarioFont")) {}

void DuelState::init() {
    std::uniform_int_distribution<std::size_t> arenaChoice(0, kArenaCount - 1);
    arenaIndex = arenaChoice(randomEngine);
    startMatch();
}

void DuelState::startMatch() {
    roundNumber = 1;
    roundsWonByPlayerOne = 0;
    roundsWonByPlayerTwo = 0;
    matchOver = false;
    startRound();
}

void DuelState::startRound() {
    std::cout << "[Core Engine] DuelState Initialized.\n";
    Systems::SoundController::getInstance().playMusic(
        Systems::resourcePath(kDuelMusicPath));
    arenaLoaded = false;
    playerOne.reset();
    playerTwo.reset();
    heldKeys.clear();
    playerOneInput.reset();
    playerTwoInput.reset();
    playerOneCombat = PlayerCombatState{};
    playerTwoCombat = PlayerCombatState{};
    activePowerUps.clear();
    playerOneFireballs.clear();
    playerTwoFireballs.clear();
    playerOneBomb.reset();
    playerTwoBomb.reset();
    blasts.clear();
    powerUpSpawnPoints.clear();
    roundOver = false;
    victoryFanfareDelay = -1.f;
    winnerPlayer = 0;
    roundTimeRemaining = kRoundTimeLimit;
    roundIntroRemaining = kRoundIntroDuration;
    fightBannerRemaining = 0.f;
    lastIntroTick = static_cast<int>(kRoundIntroDuration) + 1;

    const sf::Font& font = assets.getFont("MarioFont");
    playerOneHealthBar.init(
        font,
        "P1  MARIO",
        kPlayerOneHealthPosition,
        sf::Color(238, 52, 45),
        false,
        UI::MeterIcon::Heart
    );
    playerTwoHealthBar.init(
        font,
        "LUIGI  P2",
        kPlayerTwoHealthPosition,
        sf::Color(64, 205, 79),
        true,
        UI::MeterIcon::Heart
    );
    playerOneEnergyBar.init(
        font,
        "ENERGY",
        kPlayerOneEnergyPosition,
        sf::Color(38, 145, 255),
        false,
        UI::MeterIcon::Lightning,
        UI::MeterSize::Compact
    );
    playerTwoEnergyBar.init(
        font,
        "ENERGY",
        kPlayerTwoEnergyPosition,
        sf::Color(38, 145, 255),
        true,
        UI::MeterIcon::Lightning,
        UI::MeterSize::Compact
    );
    playerOneHealthBar.setEnergy(playerOneCombat.health);
    playerTwoHealthBar.setEnergy(playerTwoCombat.health);
    playerOneEnergyBar.setEnergy(playerOneCombat.energy);
    playerTwoEnergyBar.setEnergy(playerTwoCombat.energy);

    skyBackground.setSize({Config::kViewWidth, Config::kViewHeight});
    skyBackground.setFillColor(sf::Color(92, 148, 252));

    titleText.setString("DUEL ARENA");
    titleText.setCharacterSize(22);
    titleText.setFillColor(sf::Color::Yellow);
    titleText.setOutlineColor(sf::Color::Black);
    titleText.setOutlineThickness(2.f);
    const sf::FloatRect titleBounds = titleText.getLocalBounds();
    titleText.setOrigin({titleBounds.position.x + titleBounds.size.x / 2.f,
                         titleBounds.position.y + titleBounds.size.y / 2.f});
    titleText.setPosition({Config::kViewWidth / 2.f, 42.f});

    scoreText.setCharacterSize(20);
    scoreText.setFillColor(sf::Color::White);
    scoreText.setOutlineColor(sf::Color::Black);
    scoreText.setOutlineThickness(2.f);
    refreshScoreText();

    timerText.setCharacterSize(26);
    timerText.setOutlineColor(sf::Color::Black);
    timerText.setOutlineThickness(3.f);
    refreshTimerText();

    // The arena carries no permanent control legend, so show one while the
    // players are frozen for the countdown.
    controlsText.setString(
        "P1  A/D MOVE   W JUMP   S DUCK   F FIRE   G BOMB\n"
        "P2  ARROWS MOVE   J FIRE   K BOMB\n"
        "ARENA: " + std::string(kArenas[arenaIndex].name) + "   -   T: CHANGE ARENA");
    controlsText.setCharacterSize(16);
    controlsText.setFillColor(sf::Color(200, 230, 255));
    controlsText.setOutlineColor(sf::Color::Black);
    controlsText.setOutlineThickness(2.f);
    const sf::FloatRect controlsBounds = controlsText.getLocalBounds();
    controlsText.setOrigin({
        controlsBounds.position.x + controlsBounds.size.x / 2.f,
        controlsBounds.position.y + controlsBounds.size.y / 2.f
    });
    controlsText.setPosition({Config::kViewWidth / 2.f,
                              Config::kViewHeight / 2.f + 78.f});

    countdownText.setCharacterSize(72);
    countdownText.setFillColor(sf::Color::Yellow);
    countdownText.setOutlineColor(sf::Color::Black);
    countdownText.setOutlineThickness(4.f);

    errorText.setString("FAILED TO LOAD DUEL ARENA");
    errorText.setCharacterSize(24);
    errorText.setFillColor(sf::Color::White);
    errorText.setOutlineColor(sf::Color::Red);
    errorText.setOutlineThickness(2.f);
    const sf::FloatRect errorBounds = errorText.getLocalBounds();
    errorText.setOrigin({errorBounds.position.x + errorBounds.size.x / 2.f,
                         errorBounds.position.y + errorBounds.size.y / 2.f});
    errorText.setPosition({Config::kViewWidth / 2.f,
                           Config::kViewHeight * 0.50f});

    resultOverlay.setSize({820.f, 220.f});
    resultOverlay.setOrigin({410.f, 110.f});
    resultOverlay.setPosition({Config::kViewWidth / 2.f, Config::kViewHeight / 2.f});
    resultOverlay.setFillColor(sf::Color(0, 0, 0, 215));
    resultOverlay.setOutlineColor(sf::Color::White);
    resultOverlay.setOutlineThickness(4.f);

    resultText.setCharacterSize(kResultTextSize);
    resultText.setFillColor(sf::Color::Yellow);
    resultText.setOutlineColor(sf::Color::Black);
    resultText.setOutlineThickness(3.f);

    resultReasonText.setCharacterSize(20);
    resultReasonText.setFillColor(sf::Color::White);
    resultReasonText.setOutlineColor(sf::Color::Black);
    resultReasonText.setOutlineThickness(2.f);

    resultHintText.setString("R: REMATCH   |   ESC: PAUSE");
    resultHintText.setCharacterSize(16);
    resultHintText.setFillColor(sf::Color(180, 220, 255));
    resultHintText.setOutlineColor(sf::Color::Black);
    resultHintText.setOutlineThickness(2.f);
    const sf::FloatRect resultHintBounds = resultHintText.getLocalBounds();
    resultHintText.setOrigin({
        resultHintBounds.position.x + resultHintBounds.size.x / 2.f,
        resultHintBounds.position.y + resultHintBounds.size.y / 2.f
    });
    resultHintText.setPosition({Config::kViewWidth / 2.f,
                                Config::kViewHeight / 2.f + 66.f});

    tileMap.setTileTexture('#', assets.getTexture("GroundTile"));

    const ArenaDefinition& arena = kArenas[arenaIndex];
    const std::string arenaPath = Systems::resourcePath(arena.file);
    if (!mapParser.loadFromFile(arenaPath)) {
        std::cerr << "[DuelState] Failed to load arena map: " << arenaPath << '\n';
        return;
    }

    if (!tileMap.build(mapParser, Config::kZoom)) {
        std::cerr << "[DuelState] Arena map is empty and could not be built.\n";
        return;
    }

    constexpr std::size_t expectedColumns =
        static_cast<std::size_t>(Config::kViewTilesX);
    constexpr std::size_t expectedRows =
        static_cast<std::size_t>(Config::kViewTilesY);
    if (mapParser.getWidth() != expectedColumns
        || mapParser.getHeight() != expectedRows) {
        std::cerr << "[DuelState] Arena must be exactly " << Config::kViewTilesX
                  << 'x' << Config::kViewTilesY << " tiles; loaded "
                  << mapParser.getWidth() << 'x' << mapParser.getHeight() << ".\n";
        return;
    }

    const auto& grid = mapParser.getGrid();
    const bool rowWidthsValid = std::all_of(
        grid.begin(),
        grid.end(),
        [expectedColumns](const std::vector<char>& row) {
            return row.size() == expectedColumns;
        }
    );
    if (!rowWidthsValid) {
        std::cerr << "[DuelState] Every arena row must contain exactly "
                  << expectedColumns << " tiles.\n";
        return;
    }

    const auto& spawns = tileMap.playerSpawns();
    if (spawns.size() != 2) {
        std::cerr << "[DuelState] Arena requires exactly two player spawns; found "
                  << spawns.size() << ".\n";
        return;
    }
    if (spawns[0].x >= spawns[1].x) {
        std::cerr << "[DuelState] Arena spawns must be ordered left to right.\n";
        return;
    }

    spawnPlayers(spawns);
    arenaLoaded = playerOne != nullptr && playerTwo != nullptr;
    if (!arenaLoaded) {
        std::cerr << "[DuelState] Failed to create both duel players.\n";
        return;
    }

    buildPowerUpSpawnPoints();
    powerUpSpawnTimer = kFirstPowerUpDelay;

    std::cout << "[DuelState] Arena \"" << arena.name
              << "\" loaded with Mario and Luigi ready.\n";
}

void DuelState::handleInput(const sf::Event& event) {
    if (event.is<sf::Event::FocusLost>()) {
        heldKeys.clear();
        playerOneInput.reset();
        playerTwoInput.reset();
        return;
    }

    if (const auto* keyReleased = event.getIf<sf::Event::KeyReleased>()) {
        heldKeys.erase(keyReleased->scancode);
        return;
    }

    if (const auto* keyPressed = event.getIf<sf::Event::KeyPressed>()) {
        const auto key = keyPressed->scancode;
        if (key == sf::Keyboard::Scancode::Escape) {
            gsm.pushState(std::make_unique<DuelPauseState>(gsm, assets));
            return;
        }

        if (key == sf::Keyboard::Scancode::B) {
            return;
        }

        if (key == sf::Keyboard::Scancode::T
            && (roundOver || roundIntroRemaining > 0.f)) {
            Systems::SoundController::getInstance().playSound(
                assets.getSoundBuffer("SelectSound"));
            arenaIndex = (arenaIndex + 1) % kArenaCount;
            if (matchOver) {
                startMatch();
            } else {
                startRound();
            }
            return;
        }

        if (roundOver) {
            if (key == sf::Keyboard::Scancode::R) {
                Systems::SoundController::getInstance().playSound(
                    assets.getSoundBuffer("SelectSound"));
                if (matchOver) {
                    init();
                } else {
                    startRound();
                }
            }
            return;
        }

        heldKeys.insert(key);
    }
}

void DuelState::update(sf::Time dt) {
    const float seconds = dt.asSeconds();
    if (roundOver && seconds > 0.f) {
        updateRoundEndAudio(seconds);
        updateBlasts(seconds);
    }
    if (!arenaLoaded || !playerOne || !playerTwo || seconds <= 0.f || roundOver) {
        return;
    }

    playerOneCombat.damageProtectionRemaining = std::max(
        0.f,
        playerOneCombat.damageProtectionRemaining - seconds
    );
    playerTwoCombat.damageProtectionRemaining = std::max(
        0.f,
        playerTwoCombat.damageProtectionRemaining - seconds
    );
    updateStarPower(seconds);

    if (roundIntroRemaining > 0.f) {
        updateRoundIntro(seconds);
        playerOneAnimator.update(dt);
        playerTwoAnimator.update(dt);
        tileMap.update(dt);
        return;
    }
    fightBannerRemaining = std::max(0.f, fightBannerRemaining - seconds);

    updateRoundTimer(seconds);
    if (roundOver) {
        return;
    }

    if (playerOneAnimator.isTransforming() || playerTwoAnimator.isTransforming()) {
        playerOneAnimator.setBlinking(playerOneCombat.damageProtectionRemaining > 0.f);
        playerTwoAnimator.setBlinking(playerTwoCombat.damageProtectionRemaining > 0.f);
        playerOneAnimator.update(dt);
        playerTwoAnimator.update(dt);
        tileMap.update(dt);
        return;
    }

    const physics::AABB previousPlayerOneBounds =
        playerOne->getPhysicsBody().getAABB();
    const physics::AABB previousPlayerTwoBounds =
        playerTwo->getPhysicsBody().getAABB();

    playerOneInput.update(heldKeys);
    playerTwoInput.update(heldKeys);

    const PlayerInput& playerOneControls = playerOneInput.getPlayerInput();
    const PlayerInput& playerTwoControls = playerTwoInput.getPlayerInput();
    const bool playerOneShotFacingRight = playerOneControls.moveAxis != 0.f
        ? playerOneControls.moveAxis > 0.f
        : playerOneFacingRight;
    const bool playerTwoShotFacingRight = playerTwoControls.moveAxis != 0.f
        ? playerTwoControls.moveAxis > 0.f
        : playerTwoFacingRight;
    tryShootFireball(
        *playerOne,
        playerOneShotFacingRight,
        playerOneControls,
        playerOneCombat,
        playerOneEnergyBar,
        playerOneFireballs,
        1
    );
    tryShootFireball(
        *playerTwo,
        playerTwoShotFacingRight,
        playerTwoControls,
        playerTwoCombat,
        playerTwoEnergyBar,
        playerTwoFireballs,
        2
    );
    tryThrowBomb(
        *playerOne,
        playerOneShotFacingRight,
        playerOneControls,
        playerOneCombat,
        playerOneEnergyBar,
        playerOneBomb,
        1
    );
    tryThrowBomb(
        *playerTwo,
        playerTwoShotFacingRight,
        playerTwoControls,
        playerTwoCombat,
        playerTwoEnergyBar,
        playerTwoBomb,
        2
    );
    playerOne->setInput(playerOneControls);
    playerTwo->setInput(playerTwoControls);
    playerOne->update(seconds);
    playerTwo->update(seconds);

    updatePlayer(*playerOne, seconds);
    updatePlayer(*playerTwo, seconds);

    const bool playerOneFell = duel::hasFallenBelow(
        playerOne->getPhysicsBody().getAABB(), Config::kViewHeight);
    const bool playerTwoFell = duel::hasFallenBelow(
        playerTwo->getPhysicsBody().getAABB(), Config::kViewHeight);
    if (playerOneFell && playerTwoFell) {
        finishRound(0, "BOTH PLAYERS FELL!");
        return;
    }
    if (playerOneFell) {
        finishRound(2, "MARIO FELL!");
        return;
    }
    if (playerTwoFell) {
        finishRound(1, "LUIGI FELL!");
        return;
    }

    const bool stompHandled = resolvePlayerStomps(
        previousPlayerOneBounds,
        previousPlayerTwoBounds
    );
    if (roundOver) {
        return;
    }

    const bool starHandled = resolveStarContact();
    if (roundOver) {
        return;
    }

    if (!stompHandled && !starHandled) {
        (void)duel::resolveHorizontalPlayerCollision(
            playerOne->getPhysicsBody(),
            playerTwo->getPhysicsBody(),
            previousPlayerOneBounds,
            previousPlayerTwoBounds
        );
    }

    updateFireballs(dt);
    if (roundOver) {
        return;
    }

    updateBombs(seconds);
    updateBlasts(seconds);
    if (roundOver) {
        return;
    }

    updatePowerUps(dt);
    collectPowerUps();

    updatePlayerAnimation(
        *playerOne,
        playerOneControls,
        playerOneAnimator,
        playerOneFacingRight,
        playerOneCombat.damageProtectionRemaining,
        dt
    );
    updatePlayerAnimation(
        *playerTwo,
        playerTwoControls,
        playerTwoAnimator,
        playerTwoFacingRight,
        playerTwoCombat.damageProtectionRemaining,
        dt
    );

    tileMap.update(dt);
}

void DuelState::render(sf::RenderWindow& window) {
    window.draw(skyBackground);
    if (arenaLoaded) {
        window.draw(tileMap);
        for (const auto& powerUp : activePowerUps) {
            if (powerUp && powerUp->isAlive()) {
                powerUp->render(window);
            }
        }
        drawFireballs(window);
        drawBombs(window);
        if (playerOne) {
            playerOneAnimator.draw(window, feetCentre(*playerOne));
        }
        if (playerTwo) {
            playerTwoAnimator.draw(window, feetCentre(*playerTwo));
        }
    } else {
        window.draw(errorText);
    }
    window.draw(titleText);
    window.draw(scoreText);
    if (arenaLoaded) {
        window.draw(timerText);
    }
    playerOneHealthBar.render(window);
    playerTwoHealthBar.render(window);
    playerOneEnergyBar.render(window);
    playerTwoEnergyBar.render(window);

    if (roundIntroRemaining > 0.f || fightBannerRemaining > 0.f) {
        window.draw(countdownText);
    }
    if (roundIntroRemaining > 0.f) {
        window.draw(controlsText);
    }

    if (roundOver) {
        window.draw(resultOverlay);
        window.draw(resultText);
        window.draw(resultReasonText);
        window.draw(resultHintText);
    }
}

void DuelState::pause() {
    heldKeys.clear();
    playerOneInput.reset();
    playerTwoInput.reset();
}

void DuelState::spawnPlayers(const std::vector<sf::Vector2f>& spawns) {
    if (spawns.size() < 2) {
        return;
    }

    const float tileSize = tileMap.tileSize();
    const sf::Vector2f colliderSize{tileSize * 0.7f, tileSize * 0.95f};
    const entity::PlayerMovementConfig movementConfig{};

    const auto positionAtSpawn = [tileSize, colliderSize](sf::Vector2f spawn) {
        return sf::Vector2f{
            spawn.x + (tileSize - colliderSize.x) / 2.f,
            spawn.y + tileSize - colliderSize.y
        };
    };

    playerOne = std::make_unique<entity::Player>(
        positionAtSpawn(spawns[0]),
        colliderSize,
        sf::Vector2f{0.f, 0.f},
        movementConfig
    );
    playerTwo = std::make_unique<entity::Player>(
        positionAtSpawn(spawns[1]),
        colliderSize,
        sf::Vector2f{0.f, 0.f},
        movementConfig
    );

    playerOne->getPhysicsBody().setGrounded(true);
    playerTwo->getPhysicsBody().setGrounded(true);

    playerOneAnimator.init(assets, CharacterType::Mario);
    playerTwoAnimator.init(assets, CharacterType::Luigi);
    playerOneAnimator.reset(entity::PlayerForm::Small);
    playerTwoAnimator.reset(entity::PlayerForm::Small);

    playerOneFacingRight = true;
    playerTwoFacingRight = false;
    playerOneAnimator.setFacingRight(playerOneFacingRight);
    playerTwoAnimator.setFacingRight(playerTwoFacingRight);
}

std::vector<physics::AABB> DuelState::solidAABBsOverlapping(
    const sf::FloatRect& bounds
) const {
    std::vector<physics::AABB> solids;
    for (const sf::FloatRect& rect : tileMap.solidTilesOverlapping(bounds)) {
        solids.emplace_back(rect.position, rect.size);
    }
    return solids;
}

void DuelState::updatePlayer(entity::Player& player, float seconds) {
    physics::PhysicsBody& body = player.getPhysicsBody();
    const physics::AABB sweptBounds = physics::sweptBroadphaseBounds(
        body,
        seconds,
        kGravity,
        kMaxFallSpeed,
        kBroadphaseSafetyMargin
    );
    const std::vector<physics::AABB> solids = solidAABBsOverlapping(
        sf::FloatRect(sweptBounds.position, sweptBounds.size)
    );
    physicsSystem.update(body, solids, seconds);
}

void DuelState::updatePlayerAnimation(
    entity::Player& player,
    const PlayerInput& input,
    entity::PlayerAnimator& animator,
    bool& facingRight,
    float damageProtectionRemaining,
    sf::Time dt
) {
    const physics::PhysicsBody& body = player.getPhysicsBody();
    const sf::Vector2f velocity = body.getVelocity();
    const float horizontalSpeed = std::abs(velocity.x);

    if (input.moveAxis != 0.f) {
        facingRight = input.moveAxis > 0.f;
    } else if (horizontalSpeed > kMovementAnimationThreshold) {
        facingRight = velocity.x > 0.f;
    }

    entity::PlayerAction action = entity::PlayerAction::Idle;
    if (!body.isGrounded()) {
        action = velocity.y < 0.f ? entity::PlayerAction::Jump
                                  : entity::PlayerAction::Fall;
    } else if (player.isCrouching()) {
        action = entity::PlayerAction::Crouch;
    } else if (horizontalSpeed > kMovementAnimationThreshold) {
        action = entity::PlayerAction::Walk;
    }

    const float topSpeed = player.getMovementConfig().moveSpeed;
    animator.setAction(action);
    animator.setFacingRight(facingRight);
    animator.setSpeedRatio(topSpeed > 0.f ? horizontalSpeed / topSpeed : 0.f);
    animator.setForm(formOf(player));
    animator.setBlinking(damageProtectionRemaining > 0.f);
    animator.update(dt);
}

bool DuelState::resolvePlayerStomps(
    const physics::AABB& previousPlayerOneBounds,
    const physics::AABB& previousPlayerTwoBounds
) {
    if (tryResolveStomp(
            *playerOne,
            *playerTwo,
            previousPlayerOneBounds,
            previousPlayerTwoBounds,
            playerTwoCombat,
            playerTwoHealthBar,
            1
        )) {
        return true;
    }

    return tryResolveStomp(
        *playerTwo,
        *playerOne,
        previousPlayerTwoBounds,
        previousPlayerOneBounds,
        playerOneCombat,
        playerOneHealthBar,
        2
    );
}

bool DuelState::tryResolveStomp(
    entity::Player& attacker,
    entity::Player& victim,
    const physics::AABB& previousAttackerBounds,
    const physics::AABB& previousVictimBounds,
    PlayerCombatState& victimCombat,
    UI::EnergyBar& victimHealthBar,
    int attackerNumber
) {
    physics::PhysicsBody& attackerBody = attacker.getPhysicsBody();
    physics::PhysicsBody& victimBody = victim.getPhysicsBody();
    const physics::AABB attackerBounds = attackerBody.getAABB();
    const physics::AABB victimBounds = victimBody.getAABB();
    if (!duel::isValidStomp(
            previousAttackerBounds,
            previousVictimBounds,
            attackerBounds,
            victimBounds,
            attackerBody.getVelocity().y,
            victimBody.getVelocity().y,
            duel::damageProtectionOf(
                victimCombat.damageProtectionRemaining,
                victimCombat.starPowerRemaining
            )
        )) {
        return false;
    }

    sf::Vector2f attackerPosition = attackerBody.getPosition();
    attackerPosition.y = victimBounds.top() - attackerBounds.size.y - 1.f;
    attackerBody.setPosition(attackerPosition);
    sf::Vector2f attackerVelocity = attackerBody.getVelocity();
    attackerVelocity.y = -kStompBounceSpeed;
    attackerBody.setVelocity(attackerVelocity);
    attackerBody.setGrounded(false);

    sf::Vector2f victimVelocity = victimBody.getVelocity();
    const float attackerCentreX = attackerBounds.left() + attackerBounds.size.x / 2.f;
    const float victimCentreX = victimBounds.left() + victimBounds.size.x / 2.f;
    victimVelocity.x = attackerCentreX < victimCentreX ? 220.f : -220.f;
    victimBody.setVelocity(victimVelocity);

    victimCombat.health = duel::healthAfterStomp(victimCombat.health);
    victimCombat.damageProtectionRemaining = kDamageProtectionDuration;
    victimHealthBar.setEnergy(victimCombat.health);
    std::cout << "[DuelState] P" << attackerNumber << " stomped P"
              << (attackerNumber == 1 ? 2 : 1) << "; health now "
              << static_cast<int>(victimCombat.health) << ".\n";
    Systems::SoundController::getInstance().playSound(
        assets.getSoundBuffer("StompSound"));

    if (victimCombat.health <= 0.f) {
        finishRound(attackerNumber, "KNOCKOUT!");
    }
    return true;
}

void DuelState::refreshScoreText() {
    scoreText.setString(
        "ROUND " + std::to_string(roundNumber)
        + "     MARIO " + std::to_string(roundsWonByPlayerOne)
        + " - " + std::to_string(roundsWonByPlayerTwo) + " LUIGI"
        + "     FIRST TO " + std::to_string(kRoundsToWinMatch)
    );
    const sf::FloatRect bounds = scoreText.getLocalBounds();
    scoreText.setOrigin({bounds.position.x + bounds.size.x / 2.f,
                         bounds.position.y + bounds.size.y / 2.f});
    scoreText.setPosition({Config::kViewWidth / 2.f,
                           Config::kViewHeight - 43.f});
}

void DuelState::refreshTimerText() {
    const int wholeSeconds = static_cast<int>(std::ceil(roundTimeRemaining));
    const int minutes = wholeSeconds / 60;
    const int seconds = wholeSeconds % 60;
    const std::string padded = seconds < 10
        ? "0" + std::to_string(seconds)
        : std::to_string(seconds);
    timerText.setString("TIME  " + std::to_string(minutes) + ":" + padded);
    timerText.setFillColor(roundTimeRemaining <= kLowTimeWarning
        ? sf::Color(255, 90, 60)
        : sf::Color::White);

    const sf::FloatRect bounds = timerText.getLocalBounds();
    timerText.setOrigin({bounds.position.x + bounds.size.x / 2.f,
                         bounds.position.y + bounds.size.y / 2.f});
    timerText.setPosition({Config::kViewWidth / 2.f,
                           Config::kViewHeight - 80.f});
}

void DuelState::updateRoundTimer(float seconds) {
    roundTimeRemaining = std::max(0.f, roundTimeRemaining - seconds);
    refreshTimerText();
    if (roundTimeRemaining > 0.f) {
        return;
    }

    // Out of time: whoever is left standing with more health takes the round.
    if (playerOneCombat.health > playerTwoCombat.health) {
        finishRound(1, "TIME UP - MARIO HEALTHIER!");
    } else if (playerTwoCombat.health > playerOneCombat.health) {
        finishRound(2, "TIME UP - LUIGI HEALTHIER!");
    } else {
        finishRound(0, "TIME UP - DEAD HEAT!");
    }
}

void DuelState::setCountdownString(const std::string& text) {
    countdownText.setString(text);
    const sf::FloatRect bounds = countdownText.getLocalBounds();
    countdownText.setOrigin({bounds.position.x + bounds.size.x / 2.f,
                             bounds.position.y + bounds.size.y / 2.f});
    countdownText.setPosition({Config::kViewWidth / 2.f,
                               Config::kViewHeight / 2.f - 20.f});
}

void DuelState::updateRoundIntro(float seconds) {
    roundIntroRemaining = std::max(0.f, roundIntroRemaining - seconds);

    if (roundIntroRemaining <= 0.f) {
        fightBannerRemaining = kFightBannerDuration;
        setCountdownString("FIGHT!");
        Systems::SoundController::getInstance().playSound(
            assets.getSoundBuffer("FightSound"));
        std::cout << "[DuelState] Round " << roundNumber << " start!\n";
        return;
    }

    // One beep per whole second left on the clock.
    const int tick = static_cast<int>(std::ceil(roundIntroRemaining));
    if (tick < lastIntroTick) {
        lastIntroTick = tick;
        Systems::SoundController::getInstance().playSound(
            assets.getSoundBuffer("SelectSound"));
    }

    setCountdownString(std::to_string(tick));
}

void DuelState::finishRound(int winningPlayer, const std::string& reason) {
    if (roundOver) {
        return;
    }

    roundOver = true;
    winnerPlayer = winningPlayer;
    heldKeys.clear();
    playerOneInput.reset();
    playerTwoInput.reset();
    playerOne->getPhysicsBody().setVelocity({0.f, 0.f});
    playerTwo->getPhysicsBody().setVelocity({0.f, 0.f});

    if (winnerPlayer == 1) {
        ++roundsWonByPlayerOne;
        playerTwoAnimator.setAction(entity::PlayerAction::Dead);
    } else if (winnerPlayer == 2) {
        ++roundsWonByPlayerTwo;
        playerOneAnimator.setAction(entity::PlayerAction::Dead);
    }
    matchOver = roundsWonByPlayerOne >= kRoundsToWinMatch
        || roundsWonByPlayerTwo >= kRoundsToWinMatch;

    if (winnerPlayer == 0) {
        resultText.setString("DRAW!");
    } else {
        const std::string winnerName = winnerPlayer == 1 ? "MARIO" : "LUIGI";
        resultText.setString(
            matchOver ? winnerName + " WINS THE MATCH!"
                      : winnerName + " WINS ROUND " + std::to_string(roundNumber));
    }
    resultReasonText.setString(reason);
    resultHintText.setString(
        matchOver ? "R: NEW MATCH   T: ARENA   ESC: PAUSE"
                  : "R: NEXT ROUND   T: ARENA   ESC: PAUSE");
    refreshScoreText();
    std::cout << "[DuelState] Round finished: " << resultText.getString().toAnsiString()
              << " (" << reason << ")\n";

    const float maxResultWidth = resultOverlay.getSize().x - kResultTextPadding;
    unsigned int resultSize = kResultTextSize;
    resultText.setCharacterSize(resultSize);
    while (resultSize > 22u
           && resultText.getLocalBounds().size.x > maxResultWidth) {
        resultSize -= 2u;
        resultText.setCharacterSize(resultSize);
    }

    const auto centreTextAt = [](sf::Text& text, sf::Vector2f position) {
        const sf::FloatRect bounds = text.getLocalBounds();
        text.setOrigin({bounds.position.x + bounds.size.x / 2.f,
                        bounds.position.y + bounds.size.y / 2.f});
        text.setPosition(position);
    };
    centreTextAt(resultText, {Config::kViewWidth / 2.f,
                              Config::kViewHeight / 2.f - 45.f});
    centreTextAt(resultReasonText, {Config::kViewWidth / 2.f,
                                    Config::kViewHeight / 2.f + 10.f});
    centreTextAt(resultHintText, {Config::kViewWidth / 2.f,
                                  Config::kViewHeight / 2.f + 66.f});

    auto& sound = Systems::SoundController::getInstance();
    if (winnerPlayer == 0) {
        sound.playSound(assets.getSoundBuffer("GameOverSound"));
    } else if (matchOver) {
        sound.playSound(assets.getSoundBuffer(
            winnerPlayer == 1 ? "MarioWinsSound" : "LuigiWinsSound"));
        victoryFanfareDelay = kVictoryFanfareDelay;
    } else {
        sound.playSound(assets.getSoundBuffer("VictorySound"));
    }

    ++roundNumber;
}

void DuelState::updateRoundEndAudio(float seconds) {
    if (victoryFanfareDelay < 0.f) {
        return;
    }

    victoryFanfareDelay -= seconds;
    if (victoryFanfareDelay <= 0.f) {
        victoryFanfareDelay = -1.f;
        Systems::SoundController::getInstance().playSound(
            assets.getSoundBuffer("VictorySound"));
    }
}

void DuelState::tryShootFireball(
    entity::Player& player,
    bool facingRight,
    const PlayerInput& input,
    PlayerCombatState& combatState,
    UI::EnergyBar& energyBar,
    std::vector<entity::Bullet>& fireballs,
    int playerNumber
) {
    if (!input.shootPressed
        || !duel::canShootFireball(player.hasFirePower(), combatState.energy)) {
        return;
    }

    const physics::AABB bounds = player.getPhysicsBody().getAABB();
    const float fireballX = facingRight
        ? bounds.right()
        : bounds.left() - entity::Bullet::kSize;
    const float fireballY = bounds.top() + bounds.size.y * 0.45f
        - entity::Bullet::kSize / 2.f;
    fireballs.emplace_back(
        assets.getTexture("Bullet"),
        sf::Vector2f{fireballX, fireballY},
        sf::Vector2f{facingRight ? kFireballSpeed : -kFireballSpeed, 0.f},
        kFireballLifetime
    );

    combatState.energy = duel::energyAfterFireball(combatState.energy);
    energyBar.setEnergy(combatState.energy);
    std::cout << "[DuelState] P" << playerNumber
              << " fired; energy now "
              << static_cast<int>(combatState.energy) << ".\n";
    Systems::SoundController::getInstance().playSound(
        assets.getSoundBuffer("FireSound"));
}

void DuelState::updateFireballs(sf::Time dt) {
    updateFireballGroup(
        playerOneFireballs,
        *playerTwo,
        playerTwoCombat,
        playerTwoHealthBar,
        1,
        dt
    );
    if (roundOver) {
        return;
    }
    updateFireballGroup(
        playerTwoFireballs,
        *playerOne,
        playerOneCombat,
        playerOneHealthBar,
        2,
        dt
    );
}

void DuelState::updateFireballGroup(
    std::vector<entity::Bullet>& fireballs,
    entity::Player& target,
    PlayerCombatState& targetCombat,
    UI::EnergyBar& targetHealthBar,
    int shooterNumber,
    sf::Time dt
) {
    const sf::FloatRect targetBounds = playerBounds(target);
    for (entity::Bullet& fireball : fireballs) {
        fireball.update(dt);
        if (!fireball.isActive()) {
            continue;
        }

        const sf::FloatRect fireballBounds = fireball.bounds();
        if (fireballBounds.findIntersection(targetBounds).has_value()) {
            fireball.deactivate();
            Systems::SoundController::getInstance().playSound(
                assets.getSoundBuffer("ExplodeSound"));

            if (targetCombat.damageProtectionRemaining <= 0.f
                && targetCombat.starPowerRemaining <= 0.f) {
                targetCombat.health = duel::healthAfterFireball(targetCombat.health);
                targetCombat.damageProtectionRemaining = kDamageProtectionDuration;
                targetHealthBar.setEnergy(targetCombat.health);
                std::cout << "[DuelState] P" << shooterNumber
                          << " hit P" << (shooterNumber == 1 ? 2 : 1)
                          << " with a fireball; health now "
                          << static_cast<int>(targetCombat.health) << ".\n";
                if (targetCombat.health <= 0.f) {
                    finishRound(shooterNumber, "FIREBALL KNOCKOUT!");
                    break;
                }
            }
            continue;
        }

        const bool outsideArena = fireball.position().x + entity::Bullet::kSize < 0.f
            || fireball.position().x > Config::kViewWidth;
        if (outsideArena || tileMap.intersectsSolid(fireballBounds)) {
            fireball.deactivate();
            Systems::SoundController::getInstance().playSound(
                assets.getSoundBuffer("ExplodeSound"));
        }
    }

    fireballs.erase(
        std::remove_if(
            fireballs.begin(),
            fireballs.end(),
            [](const entity::Bullet& fireball) {
                return !fireball.isActive();
            }
        ),
        fireballs.end()
    );
}

void DuelState::drawFireballs(sf::RenderWindow& window) const {
    for (const entity::Bullet& fireball : playerOneFireballs) {
        fireball.draw(window);
    }
    for (const entity::Bullet& fireball : playerTwoFireballs) {
        fireball.draw(window);
    }
}

bool DuelState::damagePlayer(
    int victimNumber,
    float damage,
    int creditedWinner,
    const std::string& knockoutReason
) {
    PlayerCombatState& victimCombat =
        victimNumber == 1 ? playerOneCombat : playerTwoCombat;
    UI::EnergyBar& victimHealthBar =
        victimNumber == 1 ? playerOneHealthBar : playerTwoHealthBar;
    if (victimCombat.damageProtectionRemaining > 0.f
        || victimCombat.starPowerRemaining > 0.f) {
        return false;
    }

    victimCombat.health = std::clamp(
        victimCombat.health - damage,
        0.f,
        duel::kMaximumHealth
    );
    victimCombat.damageProtectionRemaining = kDamageProtectionDuration;
    victimHealthBar.setEnergy(victimCombat.health);
    std::cout << "[DuelState] P" << victimNumber << " took "
              << static_cast<int>(damage) << " damage; health now "
              << static_cast<int>(victimCombat.health) << ".\n";

    if (victimCombat.health <= 0.f) {
        finishRound(creditedWinner, knockoutReason);
    }
    return true;
}

void DuelState::updateStarPower(float seconds) {
    playerOneCombat.starPowerRemaining = std::max(
        0.f,
        playerOneCombat.starPowerRemaining - seconds
    );
    playerTwoCombat.starPowerRemaining = std::max(
        0.f,
        playerTwoCombat.starPowerRemaining - seconds
    );
    playerOneAnimator.setStarPower(playerOneCombat.starPowerRemaining > 0.f);
    playerTwoAnimator.setStarPower(playerTwoCombat.starPowerRemaining > 0.f);
}

bool DuelState::resolveStarContact() {
    const bool playerOneStarred = playerOneCombat.starPowerRemaining > 0.f;
    const bool playerTwoStarred = playerTwoCombat.starPowerRemaining > 0.f;
    // Nobody starred, or both starred, means nobody wins the collision.
    if (playerOneStarred == playerTwoStarred) {
        return false;
    }

    const sf::FloatRect attackerBox =
        playerBounds(playerOneStarred ? *playerOne : *playerTwo);
    const sf::FloatRect victimBox =
        playerBounds(playerOneStarred ? *playerTwo : *playerOne);
    if (!attackerBox.findIntersection(victimBox).has_value()) {
        return false;
    }

    const int attackerNumber = playerOneStarred ? 1 : 2;
    const int victimNumber = playerOneStarred ? 2 : 1;
    entity::Player& victim = playerOneStarred ? *playerTwo : *playerOne;
    if (!damagePlayer(
            victimNumber,
            duel::kStarContactDamage,
            attackerNumber,
            "STAR SMASH!"
        )) {
        return false;
    }

    const float attackerCentreX = attackerBox.position.x + attackerBox.size.x / 2.f;
    const float victimCentreX = victimBox.position.x + victimBox.size.x / 2.f;
    physics::PhysicsBody& victimBody = victim.getPhysicsBody();
    sf::Vector2f victimVelocity = victimBody.getVelocity();
    victimVelocity.x = attackerCentreX < victimCentreX
        ? kStarContactKnockback
        : -kStarContactKnockback;
    victimBody.setVelocity(victimVelocity);
    Systems::SoundController::getInstance().playSound(
        assets.getSoundBuffer("StompSound"));
    return true;
}

void DuelState::tryThrowBomb(
    entity::Player& player,
    bool facingRight,
    const PlayerInput& input,
    PlayerCombatState& combatState,
    UI::EnergyBar& energyBar,
    std::optional<combat::Bomb>& bombSlot,
    int playerNumber
) {
    // One bomb in the air per player keeps the arena readable.
    if (!input.bombPressed
        || bombSlot.has_value()
        || !duel::canThrowBomb(combatState.energy)) {
        return;
    }

    const physics::AABB body = player.getPhysicsBody().getAABB();
    const float bombX = facingRight
        ? body.right()
        : body.left() - combat::Bomb::kSize;
    const float bombY = body.top() + body.size.y * 0.5f;
    bombSlot.emplace(sf::Vector2f{bombX, bombY}, facingRight);

    combatState.energy = duel::energyAfterBomb(combatState.energy);
    energyBar.setEnergy(combatState.energy);
    std::cout << "[DuelState] P" << playerNumber << " threw a bomb; energy now "
              << static_cast<int>(combatState.energy) << ".\n";
    Systems::SoundController::getInstance().playSound(
        assets.getSoundBuffer("FireSound"));
}

void DuelState::updateBombs(float seconds) {
    updateBombSlot(playerOneBomb, 1, seconds);
    if (roundOver) {
        return;
    }
    updateBombSlot(playerTwoBomb, 2, seconds);
}

void DuelState::updateBombSlot(
    std::optional<combat::Bomb>& bombSlot,
    int throwerNumber,
    float seconds
) {
    if (!bombSlot) {
        return;
    }

    bombSlot->update(seconds);
    const sf::FloatRect bounds = bombSlot->getBounds();
    const bool outsideArena = bounds.position.y > Config::kViewHeight
        || bounds.position.x + bounds.size.x < 0.f
        || bounds.position.x > Config::kViewWidth;
    if (outsideArena) {
        bombSlot.reset();
        return;
    }

    if (tileMap.intersectsSolid(bounds) || bombSlot->fuseExpired()) {
        const sf::Vector2f centre = bombSlot->getPosition()
            + sf::Vector2f{combat::Bomb::kSize / 2.f, combat::Bomb::kSize / 2.f};
        bombSlot.reset();
        explodeBomb(centre, throwerNumber);
    }
}

void DuelState::explodeBomb(sf::Vector2f centre, int throwerNumber) {
    blasts.push_back({centre, kBlastEffectDuration});
    Systems::SoundController::getInstance().playSound(
        assets.getSoundBuffer("ExplodeSound"));

    const sf::FloatRect blast(
        centre - sf::Vector2f{kBombBlastRadius, kBombBlastRadius},
        {kBombBlastRadius * 2.f, kBombBlastRadius * 2.f}
    );
    const int otherPlayer = throwerNumber == 1 ? 2 : 1;

    // The blast is blind: it hurts whoever stands in it, thrower included.
    for (int victimNumber : {1, 2}) {
        entity::Player& victim = victimNumber == 1 ? *playerOne : *playerTwo;
        const sf::FloatRect victimBox = playerBounds(victim);
        if (!blast.findIntersection(victimBox).has_value()) {
            continue;
        }

        // Blowing yourself up hands the round to the other player.
        const int creditedWinner = victimNumber == throwerNumber
            ? otherPlayer
            : throwerNumber;
        const std::string reason = victimNumber == throwerNumber
            ? "OWN BOMB!"
            : "BOMB KNOCKOUT!";
        if (!damagePlayer(victimNumber, duel::kBombDamage, creditedWinner, reason)) {
            continue;
        }

        const float victimCentreX = victimBox.position.x + victimBox.size.x / 2.f;
        physics::PhysicsBody& victimBody = victim.getPhysicsBody();
        sf::Vector2f velocity = victimBody.getVelocity();
        velocity.x = victimCentreX < centre.x ? -kBlastKnockbackX : kBlastKnockbackX;
        velocity.y = -kBlastKnockbackY;
        victimBody.setVelocity(velocity);
        victimBody.setGrounded(false);
    }
}

void DuelState::updateBlasts(float seconds) {
    for (Blast& blast : blasts) {
        blast.remaining -= seconds;
    }
    blasts.erase(
        std::remove_if(
            blasts.begin(),
            blasts.end(),
            [](const Blast& blast) { return blast.remaining <= 0.f; }
        ),
        blasts.end()
    );
}

void DuelState::drawBombs(sf::RenderWindow& window) const {
    if (playerOneBomb) {
        playerOneBomb->render(window);
    }
    if (playerTwoBomb) {
        playerTwoBomb->render(window);
    }

    for (const Blast& blast : blasts) {
        const float progress = 1.f - blast.remaining / kBlastEffectDuration;
        const float radius = kBombBlastRadius * (0.35f + 0.65f * progress);
        sf::CircleShape flash(radius);
        flash.setOrigin({radius, radius});
        flash.setPosition(blast.centre);
        const auto alpha = static_cast<std::uint8_t>(
            std::clamp(220.f * (1.f - progress), 0.f, 255.f));
        flash.setFillColor(sf::Color(255, 190, 60, alpha));
        flash.setOutlineThickness(3.f);
        flash.setOutlineColor(sf::Color(255, 90, 30, alpha));
        window.draw(flash);
    }
}

void DuelState::resetPowerUpTimer() {
    std::uniform_real_distribution<float> delay(
        kMinimumPowerUpDelay,
        kMaximumPowerUpDelay
    );
    powerUpSpawnTimer = delay(randomEngine);
}

void DuelState::buildPowerUpSpawnPoints() {
    powerUpSpawnPoints.clear();
    const auto& grid = mapParser.getGrid();
    if (grid.size() < 2) {
        return;
    }

    const float tile = tileMap.tileSize();
    // The top platform sits behind the HUD. Spawn on the lower platforms so
    // every item remains visible and reachable by both players.
    for (std::size_t row = 5; row < grid.size(); ++row) {
        if (grid[row].size() < 3 || grid[row - 1].size() != grid[row].size()) {
            continue;
        }
        for (std::size_t column = 1; column + 1 < grid[row].size(); ++column) {
            const bool topSurface = grid[row][column] == '#'
                && grid[row - 1][column] != '#';
            const bool awayFromEdge = grid[row][column - 1] == '#'
                && grid[row][column + 1] == '#';
            if (topSurface && awayFromEdge) {
                powerUpSpawnPoints.push_back({
                    static_cast<float>(column) * tile,
                    static_cast<float>(row) * tile
                });
            }
        }
    }
}

void DuelState::spawnRandomPowerUp() {
    if (powerUpSpawnPoints.empty()
        || activePowerUps.size() >= kMaximumActivePowerUps) {
        return;
    }

    const float tile = tileMap.tileSize();
    const sf::FloatRect playerOneBox = playerBounds(*playerOne);
    const sf::FloatRect playerTwoBox = playerBounds(*playerTwo);
    std::vector<sf::Vector2f> safePositions;
    safePositions.reserve(powerUpSpawnPoints.size());
    for (sf::Vector2f position : powerUpSpawnPoints) {
        const sf::FloatRect arrivalArea(
            {position.x - tile, position.y - tile * 2.f},
            {tile * 3.f, tile * 3.f}
        );
        if (!arrivalArea.findIntersection(playerOneBox).has_value()
            && !arrivalArea.findIntersection(playerTwoBox).has_value()) {
            safePositions.push_back(position);
        }
    }
    if (safePositions.empty()) {
        return;
    }

    std::uniform_int_distribution<std::size_t> positionChoice(
        0,
        safePositions.size() - 1
    );
    std::uniform_int_distribution<int> powerUpChoice(0, 3);
    const sf::Vector2f blockPosition = safePositions[positionChoice(randomEngine)];

    switch (powerUpChoice(randomEngine)) {
    case 0:
        activePowerUps.push_back(std::make_unique<items::FireFlower>(
            blockPosition,
            &assets.getTexture("FireFlower"),
            Config::kZoom
        ));
        std::cout << "[DuelState] Spawned Fire Flower at ("
                  << static_cast<int>(blockPosition.x) << ", "
                  << static_cast<int>(blockPosition.y) << ").\n";
        break;
    case 1:
        activePowerUps.push_back(std::make_unique<items::Mushroom>(
            blockPosition,
            items::MushroomKind::Super,
            &assets.getTexture("SuperMushroom"),
            Config::kZoom
        ));
        std::cout << "[DuelState] Spawned Super Mushroom at ("
                  << static_cast<int>(blockPosition.x) << ", "
                  << static_cast<int>(blockPosition.y) << ").\n";
        break;
    case 2:
        activePowerUps.push_back(std::make_unique<items::Star>(
            blockPosition,
            &assets.getTexture("SuperStar"),
            Config::kZoom
        ));
        std::cout << "[DuelState] Spawned Super Star at ("
                  << static_cast<int>(blockPosition.x) << ", "
                  << static_cast<int>(blockPosition.y) << ").\n";
        break;
    default:
        activePowerUps.push_back(std::make_unique<items::ManaOrb>(
            blockPosition,
            &assets.getTexture("ManaOrb"),
            Config::kZoom
        ));
        std::cout << "[DuelState] Spawned Mana Orb at ("
                  << static_cast<int>(blockPosition.x) << ", "
                  << static_cast<int>(blockPosition.y) << ").\n";
        break;
    }
}

void DuelState::updatePowerUps(sf::Time dt) {
    const float seconds = dt.asSeconds();
    powerUpSpawnTimer -= seconds;
    if (powerUpSpawnTimer <= 0.f) {
        spawnRandomPowerUp();
        resetPowerUpTimer();
    }

    const float tile = tileMap.tileSize();
    for (const auto& powerUp : activePowerUps) {
        if (!powerUp || !powerUp->isAlive()) {
            continue;
        }

        if (auto* mushroom = dynamic_cast<items::Mushroom*>(powerUp.get())) {
            sf::FloatRect query = mushroom->getBounds();
            query.position -= sf::Vector2f{tile, tile};
            query.size += sf::Vector2f{tile * 2.f, tile * 2.f};
            mushroom->update(
                seconds,
                tile,
                physicsSystem,
                solidAABBsOverlapping(query)
            );
        } else if (auto* flower = dynamic_cast<items::FireFlower*>(powerUp.get())) {
            flower->update(seconds, tile);
        } else if (auto* manaOrb = dynamic_cast<items::ManaOrb*>(powerUp.get())) {
            manaOrb->update(seconds, tile);
        } else if (auto* star = dynamic_cast<items::Star*>(powerUp.get())) {
            sf::FloatRect query = star->getBounds();
            query.position -= sf::Vector2f{tile, tile};
            query.size += sf::Vector2f{tile * 2.f, tile * 2.f};
            star->update(seconds, tile, solidAABBsOverlapping(query));
        }
    }

    activePowerUps.erase(
        std::remove_if(
            activePowerUps.begin(),
            activePowerUps.end(),
            [](const std::unique_ptr<items::Item>& powerUp) {
                if (!powerUp || !powerUp->isAlive()) {
                    return true;
                }
                if (const auto* mushroom = dynamic_cast<const items::Mushroom*>(
                        powerUp.get())) {
                    return mushroom->hasFallenOut(Config::kViewHeight);
                }
                if (const auto* star = dynamic_cast<const items::Star*>(
                        powerUp.get())) {
                    return star->hasFallenOut(Config::kViewHeight);
                }
                return false;
            }
        ),
        activePowerUps.end()
    );
}

void DuelState::collectPowerUps() {
    const sf::FloatRect playerOneBox = playerBounds(*playerOne);
    const sf::FloatRect playerTwoBox = playerBounds(*playerTwo);

    for (const auto& powerUp : activePowerUps) {
        if (!powerUp || !powerUp->isAlive() || !powerUp->isCollectible()) {
            continue;
        }

        const sf::FloatRect itemBounds = powerUp->getBounds();
        const bool playerOneTouches =
            itemBounds.findIntersection(playerOneBox).has_value();
        const bool playerTwoTouches =
            itemBounds.findIntersection(playerTwoBox).has_value();
        if (!playerOneTouches && !playerTwoTouches) {
            continue;
        }

        entity::Player* collector = playerOne.get();
        entity::PlayerAnimator* collectorAnimator = &playerOneAnimator;
        PlayerCombatState* collectorCombat = &playerOneCombat;
        UI::EnergyBar* collectorEnergyBar = &playerOneEnergyBar;
        if (playerTwoTouches && !playerOneTouches) {
            collector = playerTwo.get();
            collectorAnimator = &playerTwoAnimator;
            collectorCombat = &playerTwoCombat;
            collectorEnergyBar = &playerTwoEnergyBar;
        } else if (playerOneTouches && playerTwoTouches) {
            const float itemCentreX = itemBounds.position.x + itemBounds.size.x / 2.f;
            const float playerOneCentreX =
                playerOneBox.position.x + playerOneBox.size.x / 2.f;
            const float playerTwoCentreX =
                playerTwoBox.position.x + playerTwoBox.size.x / 2.f;
            if (std::abs(playerTwoCentreX - itemCentreX)
                < std::abs(playerOneCentreX - itemCentreX)) {
                collector = playerTwo.get();
                collectorAnimator = &playerTwoAnimator;
                collectorCombat = &playerTwoCombat;
                collectorEnergyBar = &playerTwoEnergyBar;
            }
        }

        applyPowerUp(
            *collector,
            *collectorAnimator,
            *collectorCombat,
            *collectorEnergyBar,
            *powerUp
        );
    }

    activePowerUps.erase(
        std::remove_if(
            activePowerUps.begin(),
            activePowerUps.end(),
            [](const std::unique_ptr<items::Item>& powerUp) {
                return !powerUp || !powerUp->isAlive();
            }
        ),
        activePowerUps.end()
    );
}

void DuelState::applyPowerUp(
    entity::Player& player,
    entity::PlayerAnimator& animator,
    PlayerCombatState& combatState,
    UI::EnergyBar& energyBar,
    items::Item& powerUp
) {
    bool changesPlayerForm = false;
    const char* collectedName = "power-up";
    if (dynamic_cast<items::Mushroom*>(&powerUp)) {
        player.applyPower(entity::PowerType::Super);
        changesPlayerForm = true;
        collectedName = "Super Mushroom";
    } else if (dynamic_cast<items::FireFlower*>(&powerUp)) {
        player.applyPower(entity::PowerType::Super);
        player.applyPower(entity::PowerType::Fire);
        changesPlayerForm = true;
        collectedName = "Fire Flower";
    } else if (dynamic_cast<items::Star*>(&powerUp)) {
        combatState.starPowerRemaining = duel::kStarPowerDuration;
        animator.setStarPower(true);
        collectedName = "Super Star (10s invincible)";
    } else if (dynamic_cast<items::ManaOrb*>(&powerUp)) {
        combatState.energy = duel::energyAfterManaPickup(combatState.energy);
        energyBar.setEnergy(combatState.energy);
        collectedName = "Mana Orb (+40 energy)";
    } else {
        return;
    }

    powerUp.setAlive(false);
    if (changesPlayerForm) {
        animator.setForm(formOf(player));
    }
    std::cout << "[DuelState] "
              << (&player == playerOne.get() ? "Mario" : "Luigi")
              << " collected " << collectedName << ".\n";
    Systems::SoundController::getInstance().playSound(
        assets.getSoundBuffer("PowerUpSound"));
}

entity::PlayerForm DuelState::formOf(const entity::Player& player) {
    if (!player.isSuper()) {
        return entity::PlayerForm::Small;
    }
    return player.hasFirePower() ? entity::PlayerForm::Fire
                                 : entity::PlayerForm::Super;
}

sf::FloatRect DuelState::playerBounds(const entity::Player& player) {
    const physics::AABB bounds = player.getPhysicsBody().getAABB();
    return {bounds.position, bounds.size};
}

sf::Vector2f DuelState::feetCentre(const entity::Player& player) {
    const physics::AABB bounds = player.getPhysicsBody().getAABB();
    return {bounds.left() + bounds.size.x / 2.f, bounds.bottom()};
}
