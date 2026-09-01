#include "States/DuelState.hpp"

#include "Core/Config.hpp"
#include "Core/CharacterType.hpp"
#include "Items/FireFlower.hpp"
#include "Items/ManaOrb.hpp"
#include "Items/Mushroom.hpp"
#include "Physics/Broadphase.hpp"
#include "States/DuelRules.hpp"
#include "States/GameStateManager.hpp"
#include "Systems/ResourcePath.hpp"
#include "Systems/SoundController.hpp"
#include <algorithm>
#include <cstddef>
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
constexpr float kMinimumPowerUpDelay = 10.f;
constexpr float kMaximumPowerUpDelay = 15.f;
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
      errorText(assets.getFont("MarioFont")),
      hintText(assets.getFont("MarioFont")),
      resultText(assets.getFont("MarioFont")),
      resultReasonText(assets.getFont("MarioFont")),
      resultHintText(assets.getFont("MarioFont")) {}

void DuelState::init() {
    std::cout << "[Core Engine] DuelState Initialized.\n";
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
    powerUpSpawnPoints.clear();
    roundOver = false;
    winnerPlayer = 0;

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

    hintText.setString(
        "P1: A/D + W + F FIRE   |   P2: ARROWS + RCTRL FIRE\n"
        "STOMP: -20 HP   |   POWER-UP: 10-15S   |   FALL = LOSE   |   B/ESC: BACK");
    hintText.setCharacterSize(15);
    hintText.setFillColor(sf::Color::White);
    hintText.setOutlineColor(sf::Color::Black);
    hintText.setOutlineThickness(2.f);
    const sf::FloatRect hintBounds = hintText.getLocalBounds();
    hintText.setOrigin({hintBounds.position.x + hintBounds.size.x / 2.f,
                        hintBounds.position.y + hintBounds.size.y / 2.f});
    hintText.setPosition({Config::kViewWidth / 2.f,
                          Config::kViewHeight - 43.f});

    resultOverlay.setSize({620.f, 210.f});
    resultOverlay.setOrigin({310.f, 105.f});
    resultOverlay.setPosition({Config::kViewWidth / 2.f, Config::kViewHeight / 2.f});
    resultOverlay.setFillColor(sf::Color(0, 0, 0, 215));
    resultOverlay.setOutlineColor(sf::Color::White);
    resultOverlay.setOutlineThickness(4.f);

    resultText.setCharacterSize(38);
    resultText.setFillColor(sf::Color::Yellow);
    resultText.setOutlineColor(sf::Color::Black);
    resultText.setOutlineThickness(3.f);

    resultReasonText.setCharacterSize(20);
    resultReasonText.setFillColor(sf::Color::White);
    resultReasonText.setOutlineColor(sf::Color::Black);
    resultReasonText.setOutlineThickness(2.f);

    resultHintText.setString("R: REMATCH   |   B/ESC: BACK");
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

    const std::string arenaPath = Systems::resourcePath("assets/maps/duel_arena.txt");
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
        [](const std::vector<char>& row) {
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
    resetPowerUpTimer();

    std::cout << "[DuelState] Arena loaded with Mario and Luigi ready.\n";
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
        if (key == sf::Keyboard::Scancode::B
            || key == sf::Keyboard::Scancode::Escape) {
            Systems::SoundController::getInstance().playSound(
                assets.getSoundBuffer("SelectSound"));
            gsm.popState();
            return;
        }

        if (roundOver) {
            if (key == sf::Keyboard::Scancode::R) {
                Systems::SoundController::getInstance().playSound(
                    assets.getSoundBuffer("SelectSound"));
                init();
            }
            return;
        }

        heldKeys.insert(key);
    }
}

void DuelState::update(sf::Time dt) {
    const float seconds = dt.asSeconds();
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

    resolvePlayerStomps(previousPlayerOneBounds, previousPlayerTwoBounds);
    if (roundOver) {
        return;
    }

    updateFireballs(dt);
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
    playerOneHealthBar.render(window);
    playerTwoHealthBar.render(window);
    playerOneEnergyBar.render(window);
    playerTwoEnergyBar.render(window);
    window.draw(hintText);

    if (roundOver) {
        window.draw(resultOverlay);
        window.draw(resultText);
        window.draw(resultReasonText);
        window.draw(resultHintText);
    }
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

void DuelState::resolvePlayerStomps(
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
        return;
    }

    tryResolveStomp(
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
            victimCombat.damageProtectionRemaining
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
        resultText.setString("MARIO WINS!");
        playerTwoAnimator.setAction(entity::PlayerAction::Dead);
    } else if (winnerPlayer == 2) {
        resultText.setString("LUIGI WINS!");
        playerOneAnimator.setAction(entity::PlayerAction::Dead);
    } else {
        resultText.setString("DRAW!");
    }
    resultReasonText.setString(reason);
    std::cout << "[DuelState] Round finished: " << resultText.getString().toAnsiString()
              << " (" << reason << ")\n";

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

    Systems::SoundController::getInstance().playSound(
        assets.getSoundBuffer(winnerPlayer == 0 ? "GameOverSound" : "VictorySound"));
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

            if (targetCombat.damageProtectionRemaining <= 0.f) {
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
    std::uniform_int_distribution<int> powerUpChoice(0, 2);
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
                const auto* mushroom = dynamic_cast<const items::Mushroom*>(
                    powerUp.get());
                return mushroom && mushroom->hasFallenOut(Config::kViewHeight);
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
