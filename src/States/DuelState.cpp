#include "States/DuelState.hpp"

#include "Core/Config.hpp"
#include "Core/CharacterType.hpp"
#include "Physics/Broadphase.hpp"
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
}

DuelState::DuelState(GameStateManager& gsm, Systems::AssetManager& assets)
    : State(gsm, assets),
      playerOneInput(PlayerKeyBindings::duelPlayerOne()),
      playerTwoInput(PlayerKeyBindings::duelPlayerTwo()),
      physicsSystem(kGravity, kMaxFallSpeed),
      titleText(assets.getFont("MarioFont")),
      errorText(assets.getFont("MarioFont")),
      hintText(assets.getFont("MarioFont")) {}

void DuelState::init() {
    std::cout << "[Core Engine] DuelState Initialized.\n";
    arenaLoaded = false;
    playerOne.reset();
    playerTwo.reset();
    heldKeys.clear();
    playerOneInput.reset();
    playerTwoInput.reset();

    skyBackground.setSize({Config::kViewWidth, Config::kViewHeight});
    skyBackground.setFillColor(sf::Color(92, 148, 252));

    titleText.setString("DUEL ARENA");
    titleText.setCharacterSize(28);
    titleText.setFillColor(sf::Color::Yellow);
    titleText.setOutlineColor(sf::Color::Black);
    titleText.setOutlineThickness(2.f);
    const sf::FloatRect titleBounds = titleText.getLocalBounds();
    titleText.setOrigin({titleBounds.position.x + titleBounds.size.x / 2.f,
                         titleBounds.position.y + titleBounds.size.y / 2.f});
    titleText.setPosition({Config::kViewWidth / 2.f,
                           Config::kViewHeight * 0.07f});

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
        "P1: A/D + W   |   P2: LEFT/RIGHT + UP   |   B/ESC: BACK");
    hintText.setCharacterSize(18);
    hintText.setFillColor(sf::Color::White);
    hintText.setOutlineColor(sf::Color::Black);
    hintText.setOutlineThickness(2.f);
    const sf::FloatRect hintBounds = hintText.getLocalBounds();
    hintText.setOrigin({hintBounds.position.x + hintBounds.size.x / 2.f,
                        hintBounds.position.y + hintBounds.size.y / 2.f});
    hintText.setPosition({Config::kViewWidth / 2.f,
                          Config::kViewHeight * 0.95f});

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

        heldKeys.insert(key);
    }
}

void DuelState::update(sf::Time dt) {
    const float seconds = dt.asSeconds();
    if (!arenaLoaded || !playerOne || !playerTwo || seconds <= 0.f) {
        return;
    }

    playerOneInput.update(heldKeys);
    playerTwoInput.update(heldKeys);

    const PlayerInput& playerOneControls = playerOneInput.getPlayerInput();
    const PlayerInput& playerTwoControls = playerTwoInput.getPlayerInput();
    playerOne->setInput(playerOneControls);
    playerTwo->setInput(playerTwoControls);
    playerOne->update(seconds);
    playerTwo->update(seconds);

    updatePlayer(*playerOne, seconds);
    updatePlayer(*playerTwo, seconds);

    updatePlayerAnimation(
        *playerOne,
        playerOneControls,
        playerOneAnimator,
        playerOneFacingRight,
        dt
    );
    updatePlayerAnimation(
        *playerTwo,
        playerTwoControls,
        playerTwoAnimator,
        playerTwoFacingRight,
        dt
    );

    tileMap.update(dt);
}

void DuelState::render(sf::RenderWindow& window) {
    window.draw(skyBackground);
    if (arenaLoaded) {
        window.draw(tileMap);
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
    window.draw(hintText);
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
    animator.update(dt);
}

sf::Vector2f DuelState::feetCentre(const entity::Player& player) {
    const physics::AABB bounds = player.getPhysicsBody().getAABB();
    return {bounds.left() + bounds.size.x / 2.f, bounds.bottom()};
}
