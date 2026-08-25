#include "States/PlayState.hpp"
#include "Core/Config.hpp"
#include "Core/EventSystem.hpp"
#include "States/GameStateManager.hpp"
#include "States/GameOverState.hpp"
#include "States/IntroMenuState.hpp"
#include "States/LevelCompleteState.hpp"
#include "States/PauseState.hpp"
#include "States/RespawnState.hpp"
#include "States/VictoryState.hpp"
#include "Systems/ResourcePath.hpp"
#include "Systems/SoundController.hpp"

#include <algorithm>
#include <cmath>
#include <iostream>

namespace {
    constexpr float kGravity = 2400.f;
    constexpr float kMaxWalkSpeed = 420.f;
    constexpr float kGroundFriction = 2000.f;
    constexpr float kJumpSpeed = 1000.f;
    constexpr float kMaxFallSpeed = 1400.f;
    constexpr float kWaterGravity = 360.f;
    constexpr float kSwimStrokeSpeed = 430.f;
    constexpr float kMaxSwimFallSpeed = 320.f;
    constexpr float kWaterFrameDuration = 0.18f;
    constexpr int kWaterFrameCount = 4;

    constexpr float kVineGrowDuration = 2.25f;
    constexpr std::size_t kMushroomRewardDivisor = 4;
    constexpr float kStarPowerDuration = 10.f;

    constexpr float kTrampolineCompressDuration = 0.12f;
    constexpr float kTrampolineLaunchDuration = 0.18f;
    constexpr float kTrampolineLaunchSpeed = 1400.f;
    constexpr float kGoombaStompBounce = 550.f;
    constexpr float kDamageProtectionDuration = 0.75f;

    /// Death animation: hold still, hop, then drop off the bottom of the screen.
    constexpr float kDeathPauseDuration = 0.5f;
    constexpr float kDeathHopSpeed = 900.f;
    constexpr float kDeathSequenceTimeout = 3.5f;

    constexpr float kBombExplosionRadius = 72.f;
    constexpr float kBulletSpeed = 420.f;
    constexpr float kBulletLifetime = 1.2f;

    constexpr float kMovingPlatformSpeed = 90.f;
    constexpr float kMovingPlatformRangeTiles = 3.f;
    constexpr float kMovingPlatformWidthTiles = 3.f;

    constexpr int kOutdoorGoalColumns = 41;
    constexpr int kWorld21BonusStartColumn = 293;
    constexpr int kWorld22EntrancePipeColumn = 15;
    constexpr int kWorld22WaterStartColumn = 37;
    constexpr int kWorld22WaterEndColumn = 197;
    constexpr int kWorld22WaterSurfaceRow = 2;
    constexpr int kWorld22WaterSpawnColumn = 39;
    constexpr int kWorld22WaterSpawnRow = 9;
}

PlayState::PlayState(GameStateManager& gsm, Systems::AssetManager& assets, CharacterType character)
    : State(gsm, assets), selectedCharacter(character),
      m_physicsSystem(kGravity, kMaxFallSpeed) {}

PlayState::PlayState(GameStateManager& gsm, Systems::AssetManager& assets, const SaveData& data)
    : State(gsm, assets), selectedCharacter(data.selectedCharacter),
      m_physicsSystem(kGravity, kMaxFallSpeed)
{
    this->currentLevel = data.currentLevel;
    this->score = data.score;
    this->coins = data.coins;
    this->lives = data.lives;
}

PlayState::~PlayState() = default;

void PlayState::init() {
    Core::EventSystem::getInstance().clearAllListeners();

    std::string charName = (selectedCharacter == CharacterType::Mario) ? "Mario" : "Luigi";
    std::cout << "[Core Engine] PlayState Initialized with character: " << charName << "\n";
    
    hud.init(assets, selectedCharacter);
    hud.setScore(this->score);
    hud.setCoins(this->coins);
    hud.setLives(this->lives);
    hud.setWorld(this->currentLevel);

    registerEvents();
    tileMap.setTileTexture('#', assets.getTexture("GroundTile"));
    tileMap.setTileTexture('C', assets.getTexture("CloudBlock"));
    tileMap.setTileTexture('B', assets.getTexture("BrickTile"));
    tileMap.setTileTexture('A', assets.getTexture("BrickTile"));
    tileMap.setTileTexture('^', assets.getTexture("BrickTile"));
    tileMap.setTileTexture('b', assets.getTexture("BrickTile"));
    tileMap.setTileTexture('S', assets.getTexture("HardBlockTile"));
    tileMap.setTileTexture('[', assets.getTexture("PipeTopLeft"));
    tileMap.setTileTexture(']', assets.getTexture("PipeTopRight"));
    tileMap.setTileTexture('{', assets.getTexture("PipeBodyLeft"));
    tileMap.setTileTexture('}', assets.getTexture("PipeBodyRight"));
    tileMap.setTileTexture('?', assets.getTexture("QuestionBlock"), 4, sf::seconds(0.15f));
    tileMap.setTileTexture('U', assets.getTexture("EmptyBlock"));
    tileMap.setTileTexture('o', assets.getTexture("Coin"), 4, sf::seconds(0.12f));
    tileMap.setTileTexture('(', assets.getTexture("IslandTopLeft"));
    tileMap.setTileTexture('-', assets.getTexture("IslandTopMiddle"));
    tileMap.setTileTexture(')', assets.getTexture("IslandTopRight"));
    tileMap.setTileTexture('|', assets.getTexture("IslandTrunk"));
    tileMap.setTileTexture('g', assets.getTexture("GroundUndergroundTile"));
    tileMap.setTileTexture('r', assets.getTexture("BrickUndergroundTile"));
    tileMap.setTileTexture('O', assets.getTexture("GroundTile"));
    tileMap.setTileTexture('w', assets.getTexture("UnderwaterRock"));
    tileMap.setTileTexture('s', assets.getTexture("HardBlockTile"));

    tileMap.setDecorationTexture('M', assets.getTexture("HillBig"));
    tileMap.setDecorationTexture('m', assets.getTexture("HillSmall"));
    tileMap.setDecorationTexture('V', assets.getTexture("BushBig"));
    tileMap.setDecorationTexture('v', assets.getTexture("BushSmall"));
    tileMap.setDecorationTexture('l', assets.getTexture("CloudBig"));
    tileMap.setDecorationTexture('c', assets.getTexture("CloudSmall"));
    tileMap.setDecorationTexture('I', assets.getTexture("Island"));
    tileMap.setDecorationTexture('Y', assets.getTexture("CastleWorld1_3"));
    tileMap.setDecorationTexture('Z', assets.getTexture("CastleWorld2_1"));
    tileMap.setDecorationTexture('T', assets.getTexture("HorsetailTall"));
    tileMap.setDecorationTexture('t', assets.getTexture("HorsetailShort"));
    tileMap.setDecorationTexture('f', assets.getTexture("FenceWorld2_1Group"));
    tileMap.setDecorationTexture('q', assets.getTexture("FenceWorld2_1"));
    tileMap.setDecorationTexture('N', assets.getTexture("VineTop"));
    tileMap.setDecorationTexture('a', assets.getTexture("CoralTall"));
    tileMap.setDecorationTexture('F', assets.getTexture("Flagpole"));
    tileMap.setDecorationTexture('X', assets.getTexture("Castle"));
    tileMap.setDecorationTexture('W', assets.getTexture("WarpPipeForked"));
    tileMap.setDecorationTexture('Q', assets.getTexture("WarpPipeForked"));

    if (!loadLevel(currentLevel)) {
        return;
    }

    playLevelMusic();

    float tile = tileMap.tileSize();
    avatar.setSize({tile * 0.7f, tile * 0.95f});
    avatar.setFillColor(sf::Color::Transparent);
    animator.init(assets, selectedCharacter);
    respawnAvatar();
    m_cameraSystem.followTarget(m_player ? m_player->getPosition() : avatar.getPosition(), tileMap.pixelWidth(), tileMap.pixelHeight());

    std::cout << "[Core Engine] Controls: Left/Right (or A/D) to move, Space/Up/W to jump, Down/S to duck, X to shoot, C to throw a Fire bomb, Esc to pause.\n";
    std::cout << "[Core Engine] Press F for free look: the camera detaches so you can scroll through the level with A/D (hold Shift to go faster).\n";
}

void PlayState::registerEvents() {
    auto& events = Core::EventSystem::getInstance();
    
    events.subscribe(Core::EventType::CoinCollected, [this](const Core::Event&) {
        auto& sounds = Systems::SoundController::getInstance();
        sounds.playSound(assets.getSoundBuffer("CoinSound"));
        this->coins += 1;
        this->score += 200;
        if (this->coins >= 100) {
            this->coins -= 100;
            Core::EventSystem::getInstance().broadcast(Core::Event(Core::EventType::OneMoreLife));
        }
        this->hud.setCoins(this->coins);
        this->hud.setScore(this->score);
    });
    
    events.subscribe(Core::EventType::MushroomCollected, [this](const Core::Event&) {
        auto& sounds = Systems::SoundController::getInstance();
        sounds.playSound(assets.getSoundBuffer("PowerUpSound"));
        this->score += 1000;
        this->hud.setScore(this->score);
    });
    
    events.subscribe(Core::EventType::PlayerJumped, [this](const Core::Event&) {
        auto& sounds = Systems::SoundController::getInstance();
        sounds.playSound(assets.getSoundBuffer("JumpSound"));
    });
    
    events.subscribe(Core::EventType::PlayerDied, [this](const Core::Event&) {
        auto& sounds = Systems::SoundController::getInstance();
        sounds.playSound(assets.getSoundBuffer("DieSound"));
        this->lives = std::max(0, this->lives - 1);
        this->hud.setLives(this->lives);
    });

    events.subscribe(Core::EventType::OneMoreLife, [this](const Core::Event&) {
        auto& sounds = Systems::SoundController::getInstance();
        sounds.playSound(assets.getSoundBuffer("OneMoreLifeSound"));
        this->lives += 1;
        this->hud.setLives(this->lives);
    });

    events.subscribe(Core::EventType::GameSaved, [this](const Core::Event&) {
        this->hud.showToast("GAME SAVED!");
    });

    events.subscribe(Core::EventType::GameLoaded, [this](const Core::Event&) {
        this->hud.showToast("GAME LOADED!");
    });
}

void PlayState::handleInput(const sf::Event& event) {
    if (transitionPending || death.active) {
        return;
    }

    if (event.is<sf::Event::FocusLost>()) {
        heldKeys.clear();
        inputHandler.reset();
        return;
    }

    if (const auto* keyReleased = event.getIf<sf::Event::KeyReleased>()) {
        heldKeys.erase(keyReleased->scancode);
        return;
    }

    if (const auto* keyPressed = event.getIf<sf::Event::KeyPressed>()) {
        heldKeys.insert(keyPressed->scancode);

        if (keyPressed->scancode == sf::Keyboard::Scancode::X) {
            if (m_player && m_player->hasFirePower()) {
                spawnBullet();
            }
        } else if (keyPressed->scancode == sf::Keyboard::Scancode::C) {
            spawnBomb();
        } else if (keyPressed->scancode == sf::Keyboard::Scancode::P || keyPressed->scancode == sf::Keyboard::Scancode::Escape) {
            std::cout << "[Core Engine] Pause requested. Pushing PauseState...\n";
            gsm.pushState(std::make_unique<PauseState>(gsm, assets, *this));
            return;
        } else if (keyPressed->scancode == sf::Keyboard::Scancode::F) {
            m_cameraSystem.toggleFreeLook();
        } else if (keyPressed->scancode == sf::Keyboard::Scancode::F5) {
            if (SaveManager::saveProgress("savegame.txt", getSaveData())) {
                Core::EventSystem::getInstance().broadcast({Core::EventType::GameSaved});
                std::cout << "[Core Engine] Quick Save successful (World "
                          << Config::worldNumber(currentLevel) << "-"
                          << Config::stageNumber(currentLevel) << ").\n";
            }
        } else if (keyPressed->scancode == sf::Keyboard::Scancode::F9) {
            if (quickLoad()) {
                Core::EventSystem::getInstance().broadcast({Core::EventType::GameLoaded});
                std::cout << "[Core Engine] Quick Load successful (World "
                          << Config::worldNumber(currentLevel) << "-"
                          << Config::stageNumber(currentLevel) << ").\n";
            }
        }
    }
}

void PlayState::update(sf::Time dt) {
    if (isPaused || transitionPending || !m_player) {
        return;
    }

    waterAnimationElapsed += dt;
    while (waterAnimationElapsed.asSeconds() >= kWaterFrameDuration) {
        waterAnimationElapsed -= sf::seconds(kWaterFrameDuration);
        waterAnimationFrame = (waterAnimationFrame + 1) % kWaterFrameCount;
    }

    inputHandler.update(heldKeys);

    // Dying and growing both take the level out of the player's hands for a
    // moment. Everything else stays frozen so the animation reads clearly and
    // so a mushroom taken under a low ceiling cannot shove Mario through it.
    if (death.active) {
        updateDeathSequence(dt);
        Systems::SoundController::getInstance().update();
        return;
    }

    if (animator.isTransforming()) {
        animator.update(dt);
        tileMap.update(dt);
        Systems::SoundController::getInstance().update();
        return;
    }

    damageProtectionRemaining = std::max(0.f, damageProtectionRemaining - dt.asSeconds());
    starPowerRemaining = std::max(0.f, starPowerRemaining - dt.asSeconds());

    if (m_cameraSystem.isFreeLook()) {
        float moveAxis = 0.f;
        if (heldKeys.count(sf::Keyboard::Scancode::A) > 0 || heldKeys.count(sf::Keyboard::Scancode::Left) > 0) {
            moveAxis = -1.f;
        } else if (heldKeys.count(sf::Keyboard::Scancode::D) > 0 || heldKeys.count(sf::Keyboard::Scancode::Right) > 0) {
            moveAxis = 1.f;
        }
        bool boost = heldKeys.count(sf::Keyboard::Scancode::LShift) > 0 || heldKeys.count(sf::Keyboard::Scancode::RShift) > 0;
        m_cameraSystem.pan(moveAxis, boost, dt, tileMap.pixelWidth(), tileMap.pixelHeight());
        tileMap.update(dt);
        Systems::SoundController::getInstance().update();
        return;
    }

    updateMovingPlatforms(dt);
    if (tryEnterWorld22WaterPipe()) {
        tileMap.update(dt);
        Systems::SoundController::getInstance().update();
        return;
    }
    if (!moveAvatar(dt)) {
        tileMap.update(dt);
        Systems::SoundController::getInstance().update();
        return;
    }
    updateTrampolines(dt);
    if (tryEnterNextLevel()) {
        tileMap.update(dt);
        return;
    }

    // Update all managed entities (Enemies, Items, CoinPops) with full Physics & Wall Collision
    m_entityManager.update(
        dt,
        m_physicsSystem,
        [this](const sf::FloatRect& bounds) { return getSolidAABBsOverlapping(bounds); },
        tileMap.pixelWidth(),
        tileMap.pixelHeight(),
        tileMap.tileSize(),
        m_cameraSystem.getCenter().x
    );

    m_cameraSystem.followTarget(m_player->getPhysicsBody().getPosition(), tileMap.pixelWidth(), tileMap.pixelHeight());

    updateGrowingVines(dt);
    updateBomb(dt);
    updateBullets(dt);
    updateExplosions(dt);
    updateBlocks(dt);

    if (invincibleTimer > 0.f) {
        invincibleTimer -= dt.asSeconds();
    }
    tileMap.update(dt);
    hud.update(dt);

    if (hud.getTime() <= 0.f) {
        handlePlayerDeath();
    }
}

void PlayState::render(sf::RenderWindow& window) {
    const sf::View screenView = window.getView();
    m_cameraSystem.setViewport(screenView.getViewport());

    sf::RectangleShape sky({Config::kViewWidth, Config::kViewHeight});
    const int outdoorStartColumn = std::max(0, static_cast<int>(mapParser.getWidth()) - kOutdoorGoalColumns);
    const bool world21 = Config::worldNumber(currentLevel) == 2 && Config::stageNumber(currentLevel) == 1;
    const bool world22 = Config::worldNumber(currentLevel) == 2 && Config::stageNumber(currentLevel) == 2;
    bool underground = Config::stageNumber(currentLevel) == 2 && !world22
                    && (m_player ? m_player->getPhysicsBody().getPosition().x < outdoorStartColumn * Config::kTileSize : false);
    const sf::Color outdoorSky = (world21 || world22) ? sf::Color(146, 144, 255) : sf::Color(92, 148, 252);
    sky.setFillColor(underground ? sf::Color(0, 0, 0) : outdoorSky);
    window.draw(sky);

    window.setView(m_cameraSystem.getView());
    if (world22) {
        const float waterLeft = kWorld22WaterStartColumn * tileMap.tileSize();
        const float waterRight = kWorld22WaterEndColumn * tileMap.tileSize();
        const float waterTop = kWorld22WaterSurfaceRow * tileMap.tileSize();
        sf::RectangleShape waterBody(
            {waterRight - waterLeft, tileMap.pixelHeight() - waterTop});
        waterBody.setPosition({waterLeft, waterTop});
        // Matches the flat underwater palette in the supplied NES sheet and
        // avoids visible artificial depth bands behind the surface tiles.
        waterBody.setFillColor(sf::Color(66, 66, 255));
        window.draw(waterBody);

        sf::Sprite waterSurface(assets.getTexture("UnderwaterTiles"));
        waterSurface.setScale({Config::kZoom, Config::kZoom});
        waterSurface.setTextureRect(sf::IntRect(
            {waterAnimationFrame * TileMap::kSourceTileSize, 0},
            {TileMap::kSourceTileSize, 32}
        ));
        for (int column = kWorld22WaterStartColumn;
             column < kWorld22WaterEndColumn; ++column) {
            waterSurface.setPosition({column * tileMap.tileSize(), waterTop});
            window.draw(waterSurface);
        }
    }
    if (world21 && mapParser.getWidth() > kWorld21BonusStartColumn) {
        const float bonusLeft = kWorld21BonusStartColumn * tileMap.tileSize();
        sf::RectangleShape bonusBackdrop({tileMap.pixelWidth() - bonusLeft, tileMap.pixelHeight()});
        bonusBackdrop.setPosition({bonusLeft, 0.f});
        bonusBackdrop.setFillColor(sf::Color::Black);
        window.draw(bonusBackdrop);
    }

    // Draw Piranhas BEFORE tileMap so they slide into the pipes cleanly behind pipe tiles
    m_entityManager.renderPiranhas(window);

    window.draw(tileMap);
    drawGrowingVines(window);
    drawTrampolines(window);
    drawMovingPlatforms(window);

    // Polymorphic rendering of all managed entities
    m_entityManager.render(window);

    drawBomb(window);
    drawBullets(window);
    drawBlocks(window);
    animator.draw(window, avatarFeetCentre());
    drawExplosions(window);

    window.setView(screenView);
    m_cameraSystem.drawFreeLookHint(window, assets.getFont("MarioFont"), currentLevel);
    hud.render(window);
}

void PlayState::pause() {
    isPaused = true;
    heldKeys.clear();
    inputHandler.reset();
}

void PlayState::resume() {
    isPaused = false;
    heldKeys.clear();
    inputHandler.reset();
    registerEvents();
}

SaveData PlayState::getSaveData() const {
    SaveData data;
    data.currentLevel = currentLevel;
    data.score = score;
    data.coins = coins;
    data.lives = lives;
    data.selectedCharacter = selectedCharacter;
    return data;
}

bool PlayState::quickLoad() {
    SaveData data;
    if (SaveManager::loadProgress("savegame.txt", data)) {
        std::cout << "[Core Engine] Quick Load loading Level " << data.currentLevel << "...\n";
        if (loadLevel(data.currentLevel)) {
            score = data.score;
            coins = data.coins;
            lives = data.lives;
            selectedCharacter = data.selectedCharacter;
            hud.setCharacter(selectedCharacter);
            hud.setScore(score);
            hud.setCoins(coins);
            hud.setLives(lives);
            animator.init(assets, selectedCharacter);
            m_cameraSystem.setFreeLook(false);
            isPaused = false;
            heldKeys.clear();
            respawnAvatar();
            m_cameraSystem.followTarget(m_player->getPosition(), tileMap.pixelWidth(), tileMap.pixelHeight());
            playLevelMusic();
            return true;
        }
    }
    return false;
}

std::vector<physics::AABB> PlayState::getSolidAABBsOverlapping(const sf::FloatRect& bounds) const {
    std::vector<physics::AABB> solids;
    for (const sf::FloatRect& rect : tileMap.solidTilesOverlapping(bounds)) {
        solids.emplace_back(rect.position, rect.size);
    }
    return solids;
}

sf::FloatRect PlayState::avatarBounds() const {
    return sf::FloatRect(m_player->getPhysicsBody().getPosition(), avatar.getSize());
}

void PlayState::syncAvatarPowerVisuals() {
    if (!m_player) return;
    const auto& body = m_player->getPhysicsBody();
    avatar.setSize(body.getColliderSize());
    avatar.setPosition(body.getPosition());
    // Starts the grow/shrink flash when the form actually changed, and does
    // nothing at all when it did not.
    animator.setForm(currentPlayerForm());
}

entity::PlayerForm PlayState::currentPlayerForm() const {
    // Height is decided by the collider, never by Fire alone: the two-tile
    // artwork may only be used while the body is the two-tile one.
    if (!m_player->isSuper()) {
        return entity::PlayerForm::Small;
    }
    return m_player->hasFirePower() ? entity::PlayerForm::Fire
                                    : entity::PlayerForm::Super;
}

sf::Vector2f PlayState::avatarFeetCentre() const {
    const physics::AABB bounds = m_player->getPhysicsBody().getAABB();
    return {bounds.left() + bounds.size.x / 2.f, bounds.bottom()};
}

void PlayState::updateAvatarAnimation(sf::Time dt, bool underwater) {
    const physics::PhysicsBody& body = m_player->getPhysicsBody();
    const sf::Vector2f velocity = body.getVelocity();
    const PlayerInput& playerInput = inputHandler.getPlayerInput();
    const float speed = std::abs(velocity.x);
    const float topSpeed = m_player->getMovementConfig().moveSpeed;

    // Steering hard against your own momentum is a skid, not a walk: the legs
    // are still going one way while Mario already leans the other.
    const bool skidding = body.isGrounded()
        && playerInput.moveAxis != 0.f
        && speed > topSpeed * 0.25f
        && (playerInput.moveAxis > 0.f) != (velocity.x > 0.f);

    if (skidding) {
        facingRight = playerInput.moveAxis > 0.f;
    } else if (speed > 10.f) {
        facingRight = velocity.x > 0.f;
    } else if (playerInput.moveAxis != 0.f) {
        facingRight = playerInput.moveAxis > 0.f;
    }

    entity::PlayerAction action = entity::PlayerAction::Idle;
    if (underwater) {
        action = entity::PlayerAction::Swim;
    } else if (!body.isGrounded()) {
        action = velocity.y < 0.f ? entity::PlayerAction::Jump
                                  : entity::PlayerAction::Fall;
    } else if (m_player->isCrouching()) {
        action = entity::PlayerAction::Crouch;
    } else if (skidding) {
        action = entity::PlayerAction::Skid;
    } else if (speed > 10.f) {
        action = entity::PlayerAction::Walk;
    }

    animator.setAction(action);
    animator.setFacingRight(facingRight);
    animator.setSpeedRatio(topSpeed > 0.f ? speed / topSpeed : 0.f);
    animator.setForm(currentPlayerForm());
    // A Starman already makes the avatar unmistakable, so the damage blink
    // stays out of its way rather than fighting it for the same pixels.
    animator.setStarPower(starPowerRemaining > 0.f);
    animator.setBlinking(starPowerRemaining <= 0.f
        && (damageProtectionRemaining > 0.f || invincibleTimer > 0.f));
    animator.update(dt);

    if (animator.consumeFootstep()) {
        Systems::SoundController::getInstance().playSound(
            assets.getSoundBuffer("WalkingSound"));
    }
}

void PlayState::handlePlayerDeath() {
    if (transitionPending || death.active) return;

    Core::EventSystem::getInstance().broadcast(Core::Event(Core::EventType::PlayerDied));
    Systems::SoundController::getInstance().stopMusic();

    auto& body = m_player->getPhysicsBody();
    body.setVelocity({0.f, 0.f});
    body.clearAcceleration();
    body.setGrounded(false);

    starPowerRemaining = 0.f;
    damageProtectionRemaining = 0.f;
    invincibleTimer = 0.f;
    animator.setStarPower(false);
    animator.setBlinking(false);
    animator.setAction(entity::PlayerAction::Dead);

    death.active = true;
    death.elapsed = 0.f;
    death.velocityY = 0.f;

    // Falling into a pit is already the death fall, so it skips straight past
    // the pause and the hop instead of bouncing Mario back up out of the hole.
    if (body.getPosition().y >= tileMap.pixelHeight()) {
        death.elapsed = kDeathPauseDuration;
        death.velocityY = kMaxFallSpeed;
    }

    std::cout << "[Core Engine] Player died in World "
              << Config::worldNumber(currentLevel) << "-"
              << Config::stageNumber(currentLevel)
              << ". Lives remaining: " << lives << "\n";
}

void PlayState::updateDeathSequence(sf::Time dt) {
    const float seconds = dt.asSeconds();
    const float previousElapsed = death.elapsed;
    death.elapsed += seconds;

    animator.setAction(entity::PlayerAction::Dead);
    animator.update(dt);
    tileMap.update(dt);

    // A beat of stillness first: the sting is meant to be heard before Mario
    // moves at all.
    if (death.elapsed < kDeathPauseDuration) {
        return;
    }
    if (previousElapsed < kDeathPauseDuration) {
        death.velocityY = -kDeathHopSpeed;
    }

    death.velocityY = std::min(kMaxFallSpeed, death.velocityY + kGravity * seconds);
    auto& body = m_player->getPhysicsBody();
    sf::Vector2f position = body.getPosition();
    position.y += death.velocityY * seconds;
    body.setPosition(position);

    // No collisions on the way down: Mario drops straight through the level.
    const float cameraBottom = m_cameraSystem.getCenter().y + Config::kViewHeight / 2.f;
    if (position.y > cameraBottom + tileMap.tileSize()
        || death.elapsed > kDeathSequenceTimeout) {
        transitionPending = true;
        if (lives > 0) {
            gsm.changeState(std::make_unique<RespawnState>(gsm, assets, getSaveData()));
        } else {
            gsm.changeState(std::make_unique<GameOverState>(gsm, assets));
        }
    }
}

void PlayState::playLevelMusic() {
    auto& sounds = Systems::SoundController::getInstance();
    const bool isUnderground = Config::stageNumber(currentLevel) == 2;
    const std::string theme = isUnderground ? "assets/audio/Theme2.mp3" : "assets/audio/Theme.mp3";
    sounds.playMusic(Systems::resourcePath(theme));
}

void PlayState::respawnAvatar() {
    const sf::Vector2f spawn = tileMap.playerSpawn();
    const sf::Vector2f position{spawn.x, spawn.y + tileMap.tileSize() - avatar.getSize().y};
    m_cameraSystem.setMaxCameraCenterX(position.x + avatar.getSize().x / 2.f);
    m_cameraSystem.followTarget(position, tileMap.pixelWidth(), tileMap.pixelHeight());

    if (selectedCharacter == CharacterType::Luigi) {
        m_player = std::make_unique<entity::Luigi>(position, avatar.getSize());
    } else {
        m_player = std::make_unique<entity::Mario>(position, avatar.getSize());
    }
    facingRight = true;
    death = DeathSequence{};
    animator.reset(entity::PlayerForm::Small);
    syncAvatarPowerVisuals();
}

bool PlayState::loadLevel(int level) {
    if (level < 1 || level > Config::kFinalLevel) {
        std::cerr << "[Core Engine] Invalid level number: " << level << "\n";
        return false;
    }

    const int world = Config::worldNumber(level);
    const int stage = Config::stageNumber(level);
    const std::string mapName = "assets/maps/level" + std::to_string(world)
        + "-" + std::to_string(stage) + ".txt";
    if (!mapParser.loadFromFile(Systems::resourcePath(mapName))) {
        std::cerr << "[Core Engine] Warning: Failed to load " << mapName << " map!\n";
        return false;
    }

    // The middle stage of each world uses the cyan underground artwork.
    const bool underground = stage == 2;
    tileMap.setTileTexture('#', assets.getTexture(
        underground ? "GroundUndergroundTile" : "GroundTile"));
    tileMap.setTileTexture('B', assets.getTexture(
        underground ? "BrickUndergroundTile" : "BrickTile"));
    tileMap.setTileTexture('A', assets.getTexture(
        underground ? "BrickUndergroundTile" : "BrickTile"));
    tileMap.setTileTexture('^', assets.getTexture(
        underground ? "BrickUndergroundTile" : "BrickTile"));
    tileMap.setTileTexture('b', assets.getTexture(
        underground ? "BrickUndergroundTile" : "BrickTile"));
    tileMap.setTileTexture('S', assets.getTexture(
        underground ? "HardBlockUndergroundTile" : "HardBlockTile"));
    tileMap.setTileTexture('?', assets.getTexture(
        underground ? "QuestionBlockUnderground" : "QuestionBlock"),
        underground ? 6 : 4, sf::seconds(0.15f));

    if (!tileMap.build(mapParser, Config::kZoom)) {
        std::cerr << "[Core Engine] Warning: World " << world << "-" << stage
                  << " map is empty, nothing to play!\n";
        return false;
    }

    currentLevel = level;
    hud.setWorld(currentLevel);
    hud.setTime(world == 1 && stage == 3 ? 300.f : 400.f);

    m_entityManager.clear();
    m_cameraSystem.reset();
    growingVines.clear();
    swimButtonHeld = false;
    waterAnimationElapsed = sf::Time::Zero;
    waterAnimationFrame = 0;
    starPowerRemaining = 0.f;
    activeBomb.reset();
    bouncingBlocks.clear();
    brickDebris.clear();

    spawnWalkingEnemies();
    spawnPiranhas();
    spawnMovingPlatforms();
    spawnTrampolines();
    prepareItemBlockRewards();

    std::cout << "[Core Engine] World " << world << "-" << stage << " loaded: "
              << mapParser.getWidth() << "x" << mapParser.getHeight() << " tiles, "
              << tileMap.enemySpawns().size() << " Goombas, "
              << tileMap.blueKoopaSpawns().size() << " Blue Koopas, "
              << tileMap.greenKoopaSpawns().size() << " Green Koopas, "
              << tileMap.greenParatroopaSpawns().size() << " Green Paratroopas, "
              << tileMap.piranhaSpawns().size() << " Piranha Plants, "
              << tileMap.trampolineSpawns().size() << " trampolines, "
              << tileMap.movingPlatformSpawns().size() << " moving lifts\n";
    return true;
}

bool PlayState::tryEnterWorld22WaterPipe() {
    if (Config::worldNumber(currentLevel) != 2 || Config::stageNumber(currentLevel) != 2) {
        return false;
    }
    if (inputHandler.getPlayerInput().moveAxis <= 0.f && !heldKeys.count(sf::Keyboard::Scancode::D) && !heldKeys.count(sf::Keyboard::Scancode::Right)) {
        return false;
    }

    const float tile = tileMap.tileSize();
    const sf::FloatRect pipeEntrance(
        {(kWorld22EntrancePipeColumn - 1) * tile, 10.f * tile},
        {5.f * tile, 3.f * tile});
    if (!avatarBounds().findIntersection(pipeEntrance).has_value()) {
        return false;
    }

    auto& body = m_player->getPhysicsBody();
    body.setPosition({kWorld22WaterSpawnColumn * tile, kWorld22WaterSpawnRow * tile});
    body.setVelocity({0.f, 0.f});
    body.clearAcceleration();
    body.setGrounded(false);
    swimButtonHeld = false;
    facingRight = true;
    syncAvatarPowerVisuals();

    m_cameraSystem.setMaxCameraCenterX(kWorld22WaterStartColumn * tile + Config::kViewWidth / 2.f);
    m_cameraSystem.centreCamera({m_cameraSystem.getMaxCameraCenterX(), Config::kViewHeight / 2.f}, tileMap.pixelWidth(), tileMap.pixelHeight());

    std::cout << "[Core Engine] World 2-2 entrance pipe: entered underwater room.\n";
    return true;
}

bool PlayState::tryEnterNextLevel() {
    if (!m_player) return false;
    const sf::FloatRect player = avatarBounds();

    const bool reachedLevelExit = !Config::isLastStageOfWorld(currentLevel)
        && currentLevel < Config::kFinalLevel
        && tileMap.hasLevelExit()
        && player.findIntersection(tileMap.levelExitBounds()).has_value();

    const bool reachedStageGoal = tileMap.hasGoal()
        && player.findIntersection(tileMap.goalBounds()).has_value();

    const bool reachedMapEnd = m_player->getPhysicsBody().getPosition().x
        >= tileMap.pixelWidth() - tileMap.tileSize() * 3.f;

    if (!reachedLevelExit && !reachedStageGoal && !reachedMapEnd) {
        return false;
    }

    if (transitionPending) return false;
    transitionPending = true;
    std::cout << "[Core Engine] Level Complete reached! Transitioning to LevelCompleteState...\n";
    gsm.changeState(std::make_unique<LevelCompleteState>(gsm, assets, getSaveData()));
    return true;
}

void PlayState::spawnCoinPop(sf::Vector2f blockPosition) {
    m_entityManager.addEntity(entity::EntityFactory::createCoinPop(blockPosition, tileMap.tileSize(), &assets.getTexture("Coin")));
}

void PlayState::prepareItemBlockRewards() {
    std::size_t itemBlockCount = 0;
    for (const auto& row : mapParser.getGrid()) {
        itemBlockCount += static_cast<std::size_t>(std::count(row.begin(), row.end(), '?'));
        itemBlockCount += static_cast<std::size_t>(std::count(row.begin(), row.end(), 'H'));
        itemBlockCount += static_cast<std::size_t>(std::count(row.begin(), row.end(), 'Q'));
    }

    blockRewards.clear();
    nextBlockReward = 0;

    if (currentLevel == 1) {
        const std::size_t mushroomTarget = std::min<std::size_t>(2, itemBlockCount);
        blockRewards.insert(blockRewards.end(), mushroomTarget, BlockReward::Mushroom);
        blockRewards.insert(blockRewards.end(), itemBlockCount - mushroomTarget, BlockReward::Coin);
        std::shuffle(blockRewards.begin(), blockRewards.end(), rewardRandom);
    } else {
        std::size_t mushroomTarget = std::max<std::size_t>(2, itemBlockCount / kMushroomRewardDivisor);
        mushroomTarget = std::min<std::size_t>(mushroomTarget, itemBlockCount);
        blockRewards.insert(blockRewards.end(), mushroomTarget, BlockReward::Mushroom);
        blockRewards.insert(blockRewards.end(), itemBlockCount - mushroomTarget, BlockReward::Coin);
        std::shuffle(blockRewards.begin(), blockRewards.end(), rewardRandom);
    }

    for (auto& reward : blockRewards) {
        if (reward == BlockReward::Mushroom && rewardRandom() % 4 == 0) {
            reward = BlockReward::FireFlower;
        }
    }
}

PlayState::BlockReward PlayState::takeNextItemBlockReward() {
    if (nextBlockReward >= blockRewards.size()) {
        return BlockReward::Coin;
    }
    return blockRewards[nextBlockReward++];
}

void PlayState::spawnMushroom(sf::Vector2f blockPosition, items::MushroomKind kind) {
    const std::string texName = (kind == items::MushroomKind::OneUp) ? "OneUpMushroom" : "SuperMushroom";
    m_entityManager.addEntity(entity::EntityFactory::createMushroom(blockPosition, kind, &assets.getTexture(texName)));
}

void PlayState::spawnFireFlower(sf::Vector2f blockPosition) {
    m_entityManager.addEntity(entity::EntityFactory::createFireFlower(blockPosition, &assets.getTexture("FireFlower")));
}

void PlayState::spawnStar(sf::Vector2f blockPosition) {
    m_entityManager.addEntity(entity::EntityFactory::createStar(blockPosition, &assets.getTexture("SuperStar")));
}

bool PlayState::spawnGrowingVine(sf::Vector2f blockPosition) {
    growingVines.push_back({blockPosition, 0.f});
    return true;
}

void PlayState::updateGrowingVines(sf::Time dt) {
    float seconds = dt.asSeconds();
    for (auto& vine : growingVines) {
        vine.elapsed += seconds;
    }
}

void PlayState::drawGrowingVines(sf::RenderWindow& window) const {
    sf::Sprite vineSprite(assets.getTexture("VineTop"));
    vineSprite.setScale({Config::kZoom, Config::kZoom});
    for (const auto& vine : growingVines) {
        float progress = std::min(1.f, vine.elapsed / kVineGrowDuration);
        vineSprite.setPosition({vine.blockPosition.x, vine.blockPosition.y - progress * tileMap.tileSize() * 4.f});
        window.draw(vineSprite);
    }
}

void PlayState::spawnBullet() {
    if (!m_player) {
        return;
    }

    Systems::SoundController::getInstance().playSound(assets.getSoundBuffer("FireSound"));

    // Position bullet at character's upper body / gun height (about 1/3 from top)
    const physics::AABB player = m_player->getPhysicsBody().getAABB();
    float bulletX = facingRight ? player.right() : player.left() - entity::Bullet::kSize;
    float bulletY = player.top() + player.size.y * 0.3f;
    bullets.emplace_back(assets.getTexture("Bullet"), sf::Vector2f{bulletX, bulletY},
                         sf::Vector2f{facingRight ? kBulletSpeed : -kBulletSpeed, 0.f},
                         kBulletLifetime);
}

void PlayState::updateBullets(sf::Time dt) {
    const float cameraLeft = m_cameraSystem.getCenter().x - Config::kViewWidth / 2.f;
    const float cameraRight = m_cameraSystem.getCenter().x + Config::kViewWidth / 2.f;

    for (entity::Bullet& bullet : bullets) {
        bullet.update(dt);
        if (!bullet.isActive()) {
            continue;
        }

        const sf::FloatRect bulletBounds = bullet.bounds();
        bool hitEnemy = false;

        auto hitEntities = m_entityManager.queryOverlapping(bulletBounds);
        for (auto* ent : hitEntities) {
            if (!ent || !ent->isAlive() || !ent->isActive()) continue;
            if (dynamic_cast<entity::Character*>(ent)) {
                ent->setAlive(false);
                score += 200;
                hud.setScore(score);
                Systems::SoundController::getInstance().playSound(assets.getSoundBuffer("StompSound"));
                hitEnemy = true;
                break;
            }
        }

        if (hitEnemy) {
            spawnExplosion(bullet.position());
            bullet.deactivate();
            continue;
        }

        bool collided = tileMap.intersectsSolid(bulletBounds)
                     || bullet.position().x + entity::Bullet::kSize < cameraLeft
                     || bullet.position().x > cameraRight;

        if (collided) {
            spawnExplosion(bullet.position());
            bullet.deactivate();
        }
    }

    bullets.erase(std::remove_if(bullets.begin(), bullets.end(),
        [](const entity::Bullet& bullet) { return !bullet.isActive(); }), bullets.end());
}

void PlayState::drawBullets(sf::RenderWindow& window) const {
    for (const entity::Bullet& bullet : bullets) {
        bullet.draw(window);
    }
}

void PlayState::spawnExplosion(sf::Vector2f position) {
    explosions.push_back({position, 0.f, 0});
    Systems::SoundController::getInstance().playSound(assets.getSoundBuffer("ExplodeSound"));
}

void PlayState::updateExplosions(sf::Time dt) {
    float seconds = dt.asSeconds();
    for (auto it = explosions.begin(); it != explosions.end(); ) {
        it->elapsed += seconds;
        if (it->elapsed >= 0.05f) {
            it->elapsed = 0.f;
            it->currentFrame++;
        }
        if (it->currentFrame >= 6) {
            it = explosions.erase(it);
        } else {
            ++it;
        }
    }
}

void PlayState::drawExplosions(sf::RenderWindow& window) const {
    sf::Sprite expSprite(assets.getTexture("Explosion"));
    expSprite.setScale({32.f / 189.f, 32.f / 220.f});

    for (const ExplosionEntity& exp : explosions) {
        int col = exp.currentFrame % 3;
        int row = exp.currentFrame / 3;
        expSprite.setTextureRect(sf::IntRect({col * 189, row * 220}, {189, 220}));
        expSprite.setPosition({exp.position.x - 8.f, exp.position.y - 8.f});
        window.draw(expSprite);
    }
}

void PlayState::spawnBomb() {
    if (!m_player || activeBomb.has_value()) {
        return;
    }

    const physics::AABB player = m_player->getPhysicsBody().getAABB();
    float bombX = facingRight ? player.right() : player.left() - combat::Bomb::kSize;
    float bombY = player.top() + player.size.y * 0.5f;
    activeBomb.emplace(sf::Vector2f{bombX, bombY}, facingRight);
    Systems::SoundController::getInstance().playSound(assets.getSoundBuffer("FireSound"));
}

void PlayState::updateBomb(sf::Time dt) {
    if (activeBomb) {
        activeBomb->update(dt.asSeconds());

        sf::FloatRect bombBounds = activeBomb->getBounds();
        bool hitSolid = tileMap.intersectsSolid(bombBounds);

        if (hitSolid || activeBomb->fuseExpired()) {
            explodeBomb(activeBomb->getPosition());
            activeBomb.reset();
        }
    }
}

void PlayState::explodeBomb(sf::Vector2f center) {
    const sf::FloatRect blast(
        center - sf::Vector2f{kBombExplosionRadius, kBombExplosionRadius},
        {kBombExplosionRadius * 2.f, kBombExplosionRadius * 2.f});

    auto overlapping = m_entityManager.queryOverlapping(blast);
    for (auto* ent : overlapping) {
        ent->setAlive(false);
        score += 200;
    }
    hud.setScore(score);

    const float tileSize = tileMap.tileSize();
    const int firstCol = static_cast<int>(std::floor(blast.position.x / tileSize));
    const int lastCol = static_cast<int>(std::floor((blast.position.x + blast.size.x) / tileSize));
    const int firstRow = static_cast<int>(std::floor(blast.position.y / tileSize));
    const int lastRow = static_cast<int>(std::floor((blast.position.y + blast.size.y) / tileSize));
    for (int row = firstRow; row <= lastRow; ++row) {
        for (int col = firstCol; col <= lastCol; ++col) {
            if (tileMap.typeAt(col, row) == TileType::Brick) {
                tileMap.breakBrick(col, row);
            }
        }
    }
}

void PlayState::drawBomb(sf::RenderWindow& window) const {
    if (activeBomb) {
        activeBomb->render(window);
    }
}

void PlayState::updateBlocks(sf::Time dt) {
    float seconds = dt.asSeconds();
    
    // Update Bouncing Blocks
    for (auto& block : bouncingBlocks) {
        if (!block.active) continue;
        
        block.velocityY += kGravity * seconds;
        block.position.y += block.velocityY * seconds;
        
        // Check if it returned to original position
        if (block.position.y >= block.startY) {
            block.position.y = block.startY;
            block.active = false;
            tileMap.restoreBrick(block.col, block.row, block.originalSymbol);
        }
    }
    
    // Remove inactive blocks
    bouncingBlocks.erase(std::remove_if(bouncingBlocks.begin(), bouncingBlocks.end(),
        [](const BouncingBlock& b) { return !b.active; }), bouncingBlocks.end());
        
    // Update Brick Debris
    for (auto it = brickDebris.begin(); it != brickDebris.end(); ) {
        it->velocity.y += kGravity * seconds;
        it->position += it->velocity * seconds;
        it->elapsed += seconds;
        
        // Spin animation (change frame every 0.1s)
        if (it->elapsed > 0.1f) {
            it->elapsed = 0.f;
            it->frame = (it->frame + 1) % 4;
        }
        
        // Remove if fallen below screen
        if (it->position.y > tileMap.pixelHeight() + 50.f) {
            it = brickDebris.erase(it);
        } else {
            ++it;
        }
    }
}

void PlayState::drawBlocks(sf::RenderWindow& window) const {
    const bool underground = Config::stageNumber(currentLevel) == 2;

    // Draw Bouncing Blocks
    for (const auto& block : bouncingBlocks) {
        char drawSymbol = block.originalSymbol;
        if (drawSymbol == 'b') {
            drawSymbol = 'U';
        }

        const sf::Texture* tex = nullptr;
        sf::IntRect rect;

        if (drawSymbol == 'B' || drawSymbol == '^') {
            tex = &assets.getTexture(underground ? "BrickUndergroundTile" : "BrickTile");
        } else if (drawSymbol == '?') {
            tex = &assets.getTexture(underground ? "QuestionBlockUnderground" : "QuestionBlock");
            rect = sf::IntRect({0, 0}, {16, 16});
        } else if (drawSymbol == 'U') {
            tex = &assets.getTexture("EmptyBlock");
        }
        
        if (tex) {
            sf::Sprite sprite(*tex);
            if (drawSymbol == '?') {
                sprite.setTextureRect(rect);
            }
            float scale = tileMap.tileSize() / 16.f;
            sprite.setScale({scale, scale});
            sprite.setPosition(block.position);
            window.draw(sprite);
        }
    }
    
    // Draw Brick Debris
    if (!brickDebris.empty()) {
        sf::Sprite debrisSprite(assets.getTexture(underground ? "BrickUndergroundTile" : "BrickTile"));
        float scale = (tileMap.tileSize() / 16.f) * 0.5f; // half size
        debrisSprite.setScale({scale, scale});
        debrisSprite.setOrigin({8.f, 8.f}); // center of 16x16 texture
        
        for (const auto& debris : brickDebris) {
            debrisSprite.setPosition({debris.position.x + 8.f * scale, debris.position.y + 8.f * scale});
            debrisSprite.setRotation(sf::degrees(debris.frame * 90.f)); // Spin 90 degrees per frame
            window.draw(debrisSprite);
        }
    }
}

void PlayState::spawnWalkingEnemies() {
    const bool underground = Config::stageNumber(currentLevel) == 2 && Config::worldNumber(currentLevel) != 2;
    const auto& goombaTex = assets.getTexture(underground ? "GoombaUnderground" : "Goomba");
    const auto& blueKoopaTex = assets.getTexture("BlueKoopaUnderground");
    const auto& blueShellTex = assets.getTexture("BlueShell");
    const auto& greenKoopaTex = assets.getTexture("GreenKoopa");
    const auto& shellTex = assets.getTexture("GreenShell");
    const auto& paratroopaTex = assets.getTexture("GreenParatroopa");

    for (sf::Vector2f spawn : tileMap.enemySpawns()) {
        m_entityManager.addEntity(entity::EntityFactory::createGoomba(spawn, tileMap.tileSize(), &goombaTex));
    }
    const float koopaHeightOffset = tileMap.tileSize() * 0.5f;
    for (sf::Vector2f spawn : tileMap.blueKoopaSpawns()) {
        spawn.y -= koopaHeightOffset;
        m_entityManager.addEntity(entity::EntityFactory::createKoopa(
            spawn, tileMap.tileSize(), &blueKoopaTex, &blueShellTex,
            entity::KoopaKind::BlueUnderground));
    }
    for (sf::Vector2f spawn : tileMap.greenKoopaSpawns()) {
        spawn.y -= koopaHeightOffset;
        m_entityManager.addEntity(entity::EntityFactory::createKoopa(spawn, tileMap.tileSize(), &greenKoopaTex, &shellTex, entity::KoopaKind::Green));
    }
    for (sf::Vector2f spawn : tileMap.greenParatroopaSpawns()) {
        spawn.y -= koopaHeightOffset;
        m_entityManager.addEntity(entity::EntityFactory::createParatroopa(spawn, tileMap.tileSize(), &paratroopaTex, &shellTex));
    }
}

void PlayState::spawnPiranhas() {
    const float scale = tileMap.tileSize() / TileMap::kSourceTileSize;
    const auto& piranhaTex = assets.getTexture("PiranhaPlant");
    for (const sf::Vector2f marker : tileMap.piranhaSpawns()) {
        const float pipeTopY = marker.y + tileMap.tileSize() * 2.f;
        const sf::Vector2f shownPosition(marker.x + 8.f * scale,
                                         pipeTopY - 23.f * scale);
        m_entityManager.addEntity(entity::EntityFactory::createPiranhaPlant(shownPosition, pipeTopY, &piranhaTex, scale));
    }
}

void PlayState::spawnMovingPlatforms() {
    movingPlatforms.clear();
    for (sf::Vector2f pos : tileMap.movingPlatformSpawns()) {
        movingPlatforms.push_back({pos, pos, pos.x, kMovingPlatformSpeed});
    }
}

void PlayState::updateMovingPlatforms(sf::Time dt) {
    float seconds = dt.asSeconds();
    float range = kMovingPlatformRangeTiles * tileMap.tileSize();
    for (auto& plat : movingPlatforms) {
        plat.previousPosition = plat.position;
        plat.position.x += plat.velocityX * seconds;
        if (plat.position.x > plat.originX + range) {
            plat.position.x = plat.originX + range;
            plat.velocityX = -kMovingPlatformSpeed;
        } else if (plat.position.x < plat.originX - range) {
            plat.position.x = plat.originX - range;
            plat.velocityX = kMovingPlatformSpeed;
        }
    }
}

void PlayState::drawMovingPlatforms(sf::RenderWindow& window) const {
    const bool coinHeaven = Config::worldNumber(currentLevel) == 2
                         && Config::stageNumber(currentLevel) == 1;
    sf::Sprite sprite(assets.getTexture(
        coinHeaven ? "CoinHeavenLift" : "MovingPlatform"));
    sprite.setScale({Config::kZoom, Config::kZoom});
    for (const auto& plat : movingPlatforms) {
        sf::Vector2f drawPosition = plat.position;
        if (coinHeaven) {
            drawPosition.y += Config::kZoom;
        }
        sprite.setPosition(drawPosition);
        window.draw(sprite);
    }
}

void PlayState::spawnTrampolines() {
    trampolines.clear();
    for (sf::Vector2f pos : tileMap.trampolineSpawns()) {
        trampolines.push_back({pos.x, pos.y + tileMap.tileSize(), TrampolineState::Normal, 0.f, false});
    }
}

void PlayState::updateTrampolines(sf::Time dt) {
    float seconds = dt.asSeconds();
    for (auto& tramp : trampolines) {
        if (tramp.state == TrampolineState::Compressed) {
            tramp.elapsed += seconds;
            if (tramp.elapsed >= kTrampolineCompressDuration) {
                tramp.state = TrampolineState::Launch;
                tramp.elapsed = 0.f;
            }
        } else if (tramp.state == TrampolineState::Launch) {
            tramp.elapsed += seconds;
            if (tramp.elapsed >= kTrampolineLaunchDuration) {
                tramp.state = TrampolineState::Normal;
                tramp.elapsed = 0.f;
            }
        }
    }
}

void PlayState::drawTrampolines(sf::RenderWindow& window) const {
    for (const auto& tramp : trampolines) {
        const std::string key = (tramp.state == TrampolineState::Compressed) ? "TrampolineCompressed" :
                                ((tramp.state == TrampolineState::Launch) ? "TrampolineLaunch" : "TrampolineNormal");
        sf::Sprite sprite(assets.getTexture(key));
        sprite.setScale({Config::kZoom, Config::kZoom});
        sprite.setPosition({tramp.x, tramp.bottomY - tileMap.tileSize()});
        window.draw(sprite);
    }
}

bool PlayState::moveAvatar(sf::Time dt) {
    float seconds = dt.asSeconds();

    const auto& playerInput = inputHandler.getPlayerInput();
    auto& body = m_player->getPhysicsBody();
    const bool world22 = Config::worldNumber(currentLevel) == 2
                      && Config::stageNumber(currentLevel) == 2;
    const float playerColumn = body.getPosition().x / tileMap.tileSize();
    const physics::AABB playerBounds = body.getAABB();
    const float playerCentreY = playerBounds.position.y + playerBounds.size.y * 0.5f;
    const bool underwater = world22
                         && playerColumn >= kWorld22WaterStartColumn
                         && playerColumn < kWorld22WaterEndColumn
                         && playerCentreY >= kWorld22WaterSurfaceRow * tileMap.tileSize();
    const bool wasGrounded = body.isGrounded();
    m_player->setInput(playerInput);
    m_player->update(seconds);

    bool swimStroke = false;
    if (underwater) {
        sf::Vector2f velocity = body.getVelocity();
        swimStroke = playerInput.jumpHeld && !swimButtonHeld;
        if (swimStroke) {
            velocity.y = -kSwimStrokeSpeed;
            body.setGrounded(false);
        }
        body.setVelocity(velocity);
        body.addAcceleration({0.f, -(kGravity - kWaterGravity)});
    }
    swimButtonHeld = underwater && playerInput.jumpHeld;

    const sf::Vector2f size = body.getColliderSize();
    const float previousBottom = body.getAABB().bottom();
    if (swimStroke || (!underwater && wasGrounded && body.getVelocity().y < 0.f)) {
        Core::EventSystem::getInstance().broadcast({Core::EventType::PlayerJumped});
    }

    // Collect solid tile colliders in broadphase area
    sf::FloatRect broadBounds = avatarBounds();
    broadBounds.position.x -= 16.0f;
    broadBounds.position.y -= 16.0f;
    broadBounds.size.x += 32.0f;
    broadBounds.size.y += 32.0f;
    std::vector<physics::AABB> solids = getSolidAABBsOverlapping(broadBounds);

    // Kinematics and collision resolution
    m_physicsSystem.update(body, solids, seconds);
    if (underwater && body.getVelocity().y > kMaxSwimFallSpeed) {
        sf::Vector2f velocity = body.getVelocity();
        velocity.y = kMaxSwimFallSpeed;
        body.setVelocity(velocity);
    }

    // SMB 1985 Camera Lock: Player cannot walk back past the left edge of the screen
    float cameraLeftEdge = m_cameraSystem.getCenter().x - Config::kViewWidth / 2.f;
    sf::Vector2f position = body.getPosition();
    position.x = std::clamp(
        position.x, cameraLeftEdge,
        std::max(cameraLeftEdge, tileMap.pixelWidth() - size.x));
    body.setPosition(position);

    // --- Block Collision Response (hitCeiling) ------------------------------
    if (body.hitCeiling()) {
        sf::FloatRect headBounds = avatarBounds();
        headBounds.position.y -= 4.0f;
        headBounds.size.y = 8.0f;

        for (const sf::FloatRect& tile : tileMap.solidTilesOverlapping(headBounds)) {
            int col = static_cast<int>(tile.position.x / tileMap.tileSize());
            int row = static_cast<int>(tile.position.y / tileMap.tileSize());
            const char blockSymbol = tileMap.symbolAt(col, row);
            const bool starBlock = blockSymbol == 'A';
            const bool oneUpBlock = blockSymbol == '1';
            const bool vineBlock = blockSymbol == '^';
            if (vineBlock) {
                const bool startedGrowing = spawnGrowingVine(tile.position);
                if (!startedGrowing) {
                    Systems::SoundController::getInstance().playSound(
                        assets.getSoundBuffer("BrickCollision"));
                }
                const char symbol = tileMap.hideBrick(col, row);
                if (symbol != '\0') {
                    bouncingBlocks.push_back(
                        {col, row, symbol, tile.position, tile.position.y,
                         -150.f, true});
                }
            } else if (tileMap.activateItemBlock(col, row)) {
                if (starBlock) {
                    spawnStar(tile.position);
                } else if (oneUpBlock) {
                    spawnMushroom(tile.position, items::MushroomKind::OneUp);
                } else {
                    BlockReward reward = takeNextItemBlockReward();
                    if (reward == BlockReward::Mushroom) {
                        spawnMushroom(tile.position);
                    } else if (reward == BlockReward::FireFlower) {
                        spawnFireFlower(tile.position);
                    } else {
                        spawnCoinPop(tile.position);
                        Core::EventSystem::getInstance().broadcast(
                            {Core::EventType::CoinCollected});
                    }
                }
                char symbol = tileMap.hideBrick(col, row);
                if (symbol != '\0') {
                    bouncingBlocks.push_back({col, row, symbol, tile.position, tile.position.y, -150.f, true});
                }
            } else {
                TileType type = tileMap.typeAt(col, row);
                if (type == TileType::Brick || type == TileType::CoinBrick || type == TileType::UsedBlock) {
                    // Break normal bricks if Fire Mario or Super Mario
                    if (type == TileType::Brick && (m_player->hasFirePower() || m_player->isSuper())) {
                        if (tileMap.breakBrick(col, row)) {
                            float tx = tile.position.x;
                            float ty = tile.position.y;
                            brickDebris.push_back({{tx, ty}, {-60.f, -200.f}, 0.f, 0});
                            brickDebris.push_back({{tx + 8.f, ty}, {60.f, -200.f}, 0.f, 0});
                            brickDebris.push_back({{tx, ty + 8.f}, {-40.f, -100.f}, 0.f, 0});
                            brickDebris.push_back({{tx + 8.f, ty + 8.f}, {40.f, -100.f}, 0.f, 0});

                            Systems::SoundController::getInstance().playSound(assets.getSoundBuffer("BrickBreak"));

                            auto overlappingEnemies = m_entityManager.queryOverlapping(sf::FloatRect({tx, ty - tileMap.tileSize() - 2.f}, {tileMap.tileSize(), tileMap.tileSize()}));
                            for (auto* enemy : overlappingEnemies) {
                                if (enemy) enemy->setAlive(false);
                            }
                        }
                    } else {
                        char symbol = tileMap.hideBrick(col, row);
                        if (symbol != '\0') {
                            if (type == TileType::CoinBrick) {
                                spawnCoinPop(tile.position);
                                Core::EventSystem::getInstance().broadcast({Core::EventType::CoinCollected});
                                symbol = 'U';
                                tileMap.changeType(col, row, TileType::UsedBlock);
                            } else {
                                Systems::SoundController::getInstance().playSound(assets.getSoundBuffer("BrickCollision"));
                            }

                            float tx = tile.position.x;
                            float ty = tile.position.y;
                            auto overlappingEnemies = m_entityManager.queryOverlapping(sf::FloatRect({tx, ty - tileMap.tileSize() - 2.f}, {tileMap.tileSize(), tileMap.tileSize()}));
                            for (auto* enemy : overlappingEnemies) {
                                if (enemy) enemy->setAlive(false);
                            }

                            bouncingBlocks.push_back({col, row, symbol, tile.position, tile.position.y, -150.f, true});
                        }
                    }
                }
            }
            sf::Vector2f velocity = body.getVelocity();
            velocity.y = 0.f;
            body.setVelocity(velocity);
            break;
        }
    }

    // World 1-3 lifts are one-way platforms
    if (!body.isGrounded() && body.getVelocity().y >= 0.f) {
        const float currentBottom = body.getAABB().bottom();
        const sf::FloatRect player = avatarBounds();
        for (const MovingPlatform& platform : movingPlatforms) {
            const float platformTop = platform.position.y;
            const float platformRight = platform.position.x
                + tileMap.tileSize() * kMovingPlatformWidthTiles;
            const bool horizontallyOverlapping =
                player.position.x + player.size.x > platform.position.x + 2.f
                && player.position.x < platformRight - 2.f;
            if (horizontallyOverlapping
                && previousBottom <= platformTop + 2.f
                && currentBottom >= platformTop) {
                sf::Vector2f pos = body.getPosition();
                pos.y = platformTop - size.y;
                body.setPosition(pos);
                sf::Vector2f velocity = body.getVelocity();
                velocity.y = 0.f;
                body.setVelocity(velocity);
                body.setGrounded(true);
                break;
            }
        }
    }

    // Trampolines
    if (body.getVelocity().y > 0.f) {
        for (auto& tramp : trampolines) {
            sf::FloatRect trampBounds({tramp.x, tramp.bottomY - tileMap.tileSize()}, {tileMap.tileSize(), tileMap.tileSize()});
            if (tramp.state == TrampolineState::Normal) {
                if (avatarBounds().findIntersection(trampBounds).has_value()) {
                    if (body.getAABB().bottom() <= tramp.bottomY - tileMap.tileSize() + 24.f) {
                        tramp.state = TrampolineState::Compressed;
                        tramp.elapsed = 0.f;
                        sf::Vector2f vel = body.getVelocity();
                        vel.y = -kTrampolineLaunchSpeed;
                        body.setVelocity(vel);
                        body.setGrounded(false);
                        Core::EventSystem::getInstance().broadcast({Core::EventType::PlayerJumped});
                    }
                }
            }
        }
    }

    const int collectedCoins = tileMap.collectCoinsOverlapping(avatarBounds());
    for (int i = 0; i < collectedCoins; ++i) {
        Core::EventSystem::getInstance().broadcast({Core::EventType::CoinCollected});
    }

    // Fell in pit
    if (body.getPosition().y > tileMap.pixelHeight()) {
        std::cout << "[Core Engine] Avatar fell into a pit.\n";
        handlePlayerDeath();
        return false;
    }

    avatar.setPosition(body.getPosition());

    // --- Entity Interactions (Stomp enemy / Pickup Item / Take Damage) ---
    auto overlappingEntities = m_entityManager.queryOverlapping(avatarBounds());
    for (auto* ent : overlappingEntities) {
        if (!ent || !ent->isAlive()) continue;

        // Try collecting Mushroom
        if (auto* mushroom = dynamic_cast<items::Mushroom*>(ent)) {
            if (mushroom->isCollectible()) {
                mushroom->setAlive(false);
                if (mushroom->getKind() == items::MushroomKind::OneUp) {
                    Core::EventSystem::getInstance().broadcast(Core::Event(Core::EventType::OneMoreLife));
                } else {
                    m_player->applyPower(entity::PowerType::Super);
                    syncAvatarPowerVisuals();
                    Core::EventSystem::getInstance().broadcast(Core::Event(Core::EventType::MushroomCollected, 1000));
                }
                continue;
            }
        }
        // Try collecting FireFlower
        if (auto* flower = dynamic_cast<items::FireFlower*>(ent)) {
            if (flower->isCollectible()) {
                flower->setAlive(false);
                m_player->applyPower(entity::PowerType::Super);
                m_player->applyPower(entity::PowerType::Fire);
                syncAvatarPowerVisuals();
                Systems::SoundController::getInstance().playSound(assets.getSoundBuffer("PowerUpSound"));
                Core::EventSystem::getInstance().broadcast(Core::Event(Core::EventType::FireFlowerCollected, 1000));
                continue;
            }
        }
        // Try collecting Star
        if (auto* star = dynamic_cast<items::Star*>(ent)) {
            if (star->isCollectible()) {
                star->setAlive(false);
                starPowerRemaining = kStarPowerDuration;
                Core::EventSystem::getInstance().broadcast(Core::Event(Core::EventType::StarCollected, 1000));
                continue;
            }
        }

        // Star power defeats enemies on contact
        if (starPowerRemaining > 0.f) {
            ent->setAlive(false);
            score += 200;
            hud.setScore(score);
            Systems::SoundController::getInstance().playSound(assets.getSoundBuffer("StompSound"));
            continue;
        }

        // Ignore already squashed Goomba
        if (auto* goomba = dynamic_cast<entity::Goomba*>(ent)) {
            if (goomba->isStomped()) {
                continue;
            }
        }

        // Stomp check for Goomba / Koopa
        if (body.getVelocity().y > 0.f && (body.getAABB().bottom() <= ent->getPosition().y + 24.f)) {
            if (auto* paratroopa = dynamic_cast<entity::Paratroopa*>(ent)) {
                paratroopa->stomp();
                score += 200;
            } else if (auto* koopa = dynamic_cast<entity::Koopa*>(ent)) {
                koopa->stomp();
                score += (koopa->getState() == entity::KoopaState::ShellIdle ? 200 : 100);
            } else if (auto* goomba = dynamic_cast<entity::Goomba*>(ent)) {
                goomba->stomp();
                score += 100;
            } else {
                ent->setAlive(false);
                score += 100;
            }
            sf::Vector2f vel = body.getVelocity();
            vel.y = -kGoombaStompBounce;
            body.setVelocity(vel);
            body.setGrounded(false);
            hud.setScore(score);
            Systems::SoundController::getInstance().playSound(assets.getSoundBuffer("StompSound"));
            break;
        } else if (auto* koopa = dynamic_cast<entity::Koopa*>(ent); koopa && koopa->getState() == entity::KoopaState::ShellIdle) {
            // Kick idle shell from side
            float playerCentre = body.getAABB().left() + body.getAABB().size.x / 2.f;
            float shellCentre = koopa->getPosition().x + koopa->getTileSize() / 2.f;
            koopa->kick(playerCentre < shellCentre);
            damageProtectionRemaining = 0.25f;
            score += 400;
            hud.setScore(score);
            Systems::SoundController::getInstance().playSound(assets.getSoundBuffer("StompSound"));
            break;
        } else if (damageProtectionRemaining <= 0.f && invincibleTimer <= 0.f) {
            // Player takes damage
            if (m_player->removeLatestPower()) {
                syncAvatarPowerVisuals();
                damageProtectionRemaining = kDamageProtectionDuration;
                invincibleTimer = 1.5f;
                Systems::SoundController::getInstance().playSound(assets.getSoundBuffer("DowngradeSound"));
            } else {
                handlePlayerDeath();
                return false;
            }
        }
    }

    updateAvatarAnimation(dt, underwater);

    return true;
}
