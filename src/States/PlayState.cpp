#include "States/PlayState.hpp"
#include "Entities/HammerBro.hpp"
#include "Entities/PiranhaPlant.hpp"
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
#include <array>
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
    constexpr float kWorld22CurrentAcceleration = 1100.f;
    constexpr float kWorld22CurrentMaxFallSpeed = 640.f;
    constexpr float kWaterFrameDuration = 0.18f;
    constexpr int kWaterFrameCount = 4;

    constexpr float kVineGrowDuration = 2.25f;
    constexpr float kVineClimbDuration = 1.15f;
    constexpr float kVineClimbDistanceTiles = 4.f;
    constexpr float kVineLowerOffsetTiles = 0.5f;
    constexpr std::size_t kMushroomRewardDivisor = 4;
    constexpr float kStarPowerDuration = 10.f;
    constexpr int kCoinBlockCapacity = 10;

    constexpr float kTrampolineCompressDuration = 0.12f;
    constexpr float kTrampolineLaunchDuration = 0.18f;
    constexpr float kTrampolineLaunchSpeed = 1400.f;
    constexpr float kGoombaStompBounce = 550.f;
    // A thrown hammer goes up and over: high enough to clear a Super Mario,
    // slow enough forward that standing still is not a safe answer.
    constexpr float kHammerThrowSpeedX = 260.f;
    constexpr float kHammerThrowSpeedY = -880.f;
    /// Piranha Plants hold still while the avatar is this close to their pipe.
    constexpr float kPiranhaSafeTiles = 1.5f;
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
    /// World 3-3's lifts: the up-and-down one, and the pulleys, which take a
    /// beat longer so there is time to hop off before the rope runs out.
    constexpr float kVerticalLiftSpeed = 66.f;
    constexpr float kBalanceLiftSpeed = 54.f;
    constexpr float kBalanceRopeWidth = 2.f;

    /// The castle courses (World 1-4 and 2-4) share one legend, one palette,
    /// one set of hazards and one fake Bowser waiting on a collapsing bridge,
    /// so everything castle-specific keys off this rather than off a world.
    bool isCastleStage(int level) {
        return Config::stageNumber(level) == 4;
    }

    constexpr int kFireBarBallCount = 6;
    constexpr float kFireBarAngularSpeed = 1.8f;
    constexpr float kCastleBossWalkSpeed = 54.f;
    constexpr float kCastleBossJumpSpeed = 620.f;
    constexpr float kCastleBossFireSpeed = 190.f;
    constexpr float kCastleBossFrameDuration = 0.18f;
    constexpr float kCastleBossJumpInterval = 2.1f;
    constexpr float kCastleBossFireInterval = 1.65f;
    /// Podoboos rest in the lava, then clear it by roughly five tiles.
    constexpr float kPodobooJumpSpeed = 1080.f;
    constexpr float kPodobooRestDuration = 2.4f;
    constexpr float kCastleElevatorSpeed = 96.f;
    /// Both castle lift sprites are narrower than the standard three tiles.
    constexpr float kCastleLiftWidthTiles = 2.f;
    constexpr float kCastleElevatorWidthTiles = 1.5f;
    constexpr float kCastleBridgeStepDuration = 0.085f;
    constexpr float kCastleClearFinishDelay = 0.8f;
    constexpr float kTwoPi = 6.28318530718f;

    constexpr int kOutdoorGoalColumns = 41;
    constexpr int kWorld21BonusStartColumn = 293;
    constexpr int kWorld22EntrancePipeColumn = 9;
    constexpr int kWorld22WaterStartColumn = 37;
    constexpr int kWorld22WaterEndColumn = 229;
    constexpr int kWorld22WaterSurfaceRow = 2;
    constexpr int kWorld22WaterFloorRow = 13;
    constexpr int kWorld22WaterSpawnColumn = 39;
    constexpr int kWorld22WaterSpawnRow = 9;
    constexpr int kWorld22ExitMouthColumn = 224;
    constexpr int kWorld22ExitMouthRow = 8;
    constexpr int kWorld22OutdoorPipeColumn = 232;
    constexpr int kWorld22OutdoorPipeArrivalRow = 10;
    constexpr float kWorld22CheepMinDelay = 1.4f;
    constexpr float kWorld22CheepDelayRange = 1.2f;

    struct ColumnSpan {
        int first;
        int pastLast;
    };

    // The three openings in the supplied guide are the stage's strong currents.
    constexpr std::array<ColumnSpan, 3> kWorld22Currents{{
        {103, 108},
        {168, 177},
        {194, 201},
    }};

    bool isWorld22CurrentColumn(float worldX, float tileSize) {
        const int column = static_cast<int>(std::floor(worldX / tileSize));
        return std::any_of(
            kWorld22Currents.begin(), kWorld22Currents.end(),
            [column](const ColumnSpan& current) {
                return column >= current.first && column < current.pastLast;
            });
    }
    constexpr int kWorld23FishStartColumn = 13;
    constexpr int kWorld23FishEndColumn = 208;
    constexpr int kWorld23CoinTotal = 35;
    constexpr int kWorld13OneUpCoinRequirement = 21;
}

PlayState::PlayState(GameStateManager& gsm, Systems::AssetManager& assets, CharacterType character)
    : State(gsm, assets), selectedCharacter(character),
      m_physicsSystem(kGravity, kMaxFallSpeed),
      m_commandParser(std::make_unique<Core::CommandParser>(*this)) {}

PlayState::PlayState(GameStateManager& gsm, Systems::AssetManager& assets, const SaveData& data)
    : State(gsm, assets), selectedCharacter(data.selectedCharacter),
      m_physicsSystem(kGravity, kMaxFallSpeed),
      m_commandParser(std::make_unique<Core::CommandParser>(*this))
{
    this->currentLevel = data.currentLevel;
    this->score = data.score;
    this->coins = data.coins;
    this->lives = data.lives;
    this->world13OneUpUnlocked = data.world13OneUpUnlocked;
    this->world23AllCoinsCollected = data.world23AllCoinsCollected;
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
    tileMap.setTileTexture('*', assets.getTexture("BrickTile"));
    tileMap.setTileTexture('5', assets.getTexture("BrickTile"));
    tileMap.setTileTexture('b', assets.getTexture("BrickTile"));
    tileMap.setTileTexture('S', assets.getTexture("HardBlockTile"));
    tileMap.setTileTexture('[', assets.getTexture("PipeTopLeft"));
    tileMap.setTileTexture(']', assets.getTexture("PipeTopRight"));
    tileMap.setTileTexture('{', assets.getTexture("PipeBodyLeft"));
    tileMap.setTileTexture('}', assets.getTexture("PipeBodyRight"));
    tileMap.setTileTexture('?', assets.getTexture("QuestionBlock"), 4, sf::seconds(0.15f));
    tileMap.setTileTexture('!', assets.getTexture("QuestionBlock"), 4, sf::seconds(0.15f));
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
    tileMap.setTileTexture('=', assets.getTexture("World23BridgeDeck"));
    tileMap.setTileTexture('_', assets.getTexture("World31WaterSurface"));
    tileMap.setTileTexture(',', assets.getTexture("World31Water"));

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
    tileMap.setDecorationTexture('~', assets.getTexture("World23BridgeRail"));
    tileMap.setDecorationTexture('z', assets.getTexture("World32FencePairOffset"));

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
        this->levelCoinsCollected += 1;
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
    if (transitionPending || death.active || castleClear.active) {
        return;
    }

    if (event.is<sf::Event::FocusLost>()) {
        heldKeys.clear();
        inputHandler.reset();
        return;
    }

    if (const auto* textEntered = event.getIf<sf::Event::TextEntered>()) {
        m_console.handleTextInput(textEntered->unicode);
        return;
    }

    if (const auto* keyReleased = event.getIf<sf::Event::KeyReleased>()) {
        if (!m_console.isActive()) {
            heldKeys.erase(keyReleased->scancode);
        }
        return;
    }

    if (const auto* keyPressed = event.getIf<sf::Event::KeyPressed>()) {
        if (keyPressed->scancode == sf::Keyboard::Scancode::Slash) {
            m_console.toggle();
            if (m_console.isActive()) {
                heldKeys.clear();
                inputHandler.reset();
            }
            return;
        }

        if (m_console.isActive()) {
            if (keyPressed->scancode == sf::Keyboard::Scancode::Escape) {
                m_console.toggle();
                return;
            }
            if (m_console.handleKeyPress(keyPressed->scancode)) {
                std::string cmd = m_console.getAndClearInput();
                Core::CommandResult res = m_commandParser->execute(cmd);
                m_console.addOutput(res.message);
            }
            return;
        }

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
            if (!m_cameraSystem.isFreeLook() && m_player) {
                m_cameraSystem.followTarget(m_player->getPhysicsBody().getPosition(), tileMap.pixelWidth(), tileMap.pixelHeight());
            }
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

    if (m_console.isActive()) {
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

    if (castleClear.active) {
        updateCastleClearSequence(dt);
        tileMap.update(dt);
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
    updateGrowingVines(dt);
    if (updateCoinHeavenClimb(dt) || tryStartCoinHeavenClimb()) {
        tileMap.update(dt);
        Systems::SoundController::getInstance().update();
        return;
    }
    if (tryEnterWorld22WaterPipe() || tryLeaveWorld22WaterPipe()
        || tryEnterSecretRoom() || tryLeaveSecretRoom()) {
        tileMap.update(dt);
        Systems::SoundController::getInstance().update();
        return;
    }
    if (!moveAvatar(dt)) {
        tileMap.update(dt);
        Systems::SoundController::getInstance().update();
        return;
    }
    updateCastleHazards(dt);
    if (death.active) {
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
    updateEnemyReactions();
    updateAquaticEnemyTargets();
    updateWorld22CheepSpawner(dt);
    updateFlyingCheepSpawner(dt);
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

    updateHammers(dt);
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
    if (m_godMode && m_player->getPhysicsBody().getPosition().y > tileMap.pixelHeight()) {
        sf::Vector2f pos = m_player->getPhysicsBody().getPosition();
        pos.y = tileMap.pixelHeight() - 100.f;
        m_player->getPhysicsBody().setPosition(pos);
        m_player->getPhysicsBody().setVelocity({0.f, 0.f});
    }
}

void PlayState::render(sf::RenderWindow& window) {
    const sf::View screenView = window.getView();
    m_cameraSystem.setViewport(screenView.getViewport());

    sf::RectangleShape sky({Config::kViewWidth, Config::kViewHeight});
    const int outdoorStartColumn = std::max(0, static_cast<int>(mapParser.getWidth()) - kOutdoorGoalColumns);
    const bool world21 = Config::worldNumber(currentLevel) == 2 && Config::stageNumber(currentLevel) == 1;
    const bool world22 = Config::worldNumber(currentLevel) == 2 && Config::stageNumber(currentLevel) == 2;
    const bool world23 = Config::worldNumber(currentLevel) == 2 && Config::stageNumber(currentLevel) == 3;
    const bool world32 = Config::worldNumber(currentLevel) == 3 && Config::stageNumber(currentLevel) == 2;
    bool underground = Config::stageNumber(currentLevel) == 2 && !world22 && !world32
                    && (m_player ? m_player->getPhysicsBody().getPosition().x < outdoorStartColumn * Config::kTileSize : false);
    // World 3 is played at night, so its sky stays black from the castle all
    // the way to the flagpole - and through the hidden room behind 3-1's pipe.
    const bool world31 = Config::worldNumber(currentLevel) == 3 && Config::stageNumber(currentLevel) == 1;
    const bool world33 = Config::worldNumber(currentLevel) == 3 && Config::stageNumber(currentLevel) == 3;
    const bool castle = isCastleStage(currentLevel);
    const sf::Color outdoorSky = (world21 || world23)
        ? sf::Color(146, 144, 255) : sf::Color(92, 148, 252);
    sky.setFillColor(underground || insideSecretRoom || castle || world31 || world32 || world33
                         ? sf::Color(0, 0, 0) : outdoorSky);
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

    // During the axe sequence Bowser drops behind the bridge and lava tiles.
    // Drawing him before the map lets those foreground surfaces cover him as
    // he sinks, rather than leaving the sprite floating on top of the lava.
    if (castleClear.active && castleClear.bossFalling) {
        drawCastleHazards(window);
    }

    window.draw(tileMap);
    drawGrowingVines(window);
    drawTrampolines(window);
    drawMovingPlatforms(window);

    // Polymorphic rendering of all managed entities
    m_entityManager.render(window);
    if (!castleClear.active || !castleClear.bossFalling) {
        drawCastleHazards(window);
    }

    drawBomb(window);
    drawHammers(window);
    drawBullets(window);
    drawBlocks(window);
    animator.draw(window, avatarFeetCentre());
    drawExplosions(window);

    window.setView(screenView);
    m_cameraSystem.drawFreeLookHint(window, assets.getFont("MarioFont"), currentLevel);
    hud.render(window);
    if (m_console.isActive()) {
        m_console.render(window, assets.getFont("MarioFont"), screenView);
    }
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
    data.world13OneUpUnlocked = world13OneUpUnlocked;
    data.world23AllCoinsCollected = world23AllCoinsCollected;
    return data;
}

bool PlayState::quickLoad() {
    SaveData data;
    if (SaveManager::loadProgress("savegame.txt", data)) {
        std::cout << "[Core Engine] Quick Load loading Level " << data.currentLevel << "...\n";
        const bool previousWorld13Reward = world13OneUpUnlocked;
        const bool previousWorld23Reward = world23AllCoinsCollected;
        world13OneUpUnlocked = data.world13OneUpUnlocked;
        world23AllCoinsCollected = data.world23AllCoinsCollected;
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
        world13OneUpUnlocked = previousWorld13Reward;
        world23AllCoinsCollected = previousWorld23Reward;
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
    if (m_musicLocked) return;
    
    auto& sounds = Systems::SoundController::getInstance();
    // World 3-2 is the one stage 2 played outdoors, so it keeps the overworld
    // theme its stage number would otherwise trade for the underground one.
    const bool world32 = Config::worldNumber(currentLevel) == 3
                      && Config::stageNumber(currentLevel) == 2;
    const bool world23 = Config::worldNumber(currentLevel) == 2
                      && Config::stageNumber(currentLevel) == 3;
    const bool castle = isCastleStage(currentLevel);
    std::string theme = "assets/audio/Theme.mp3";
    if ((Config::stageNumber(currentLevel) == 2 && !world32) || insideSecretRoom) {
        theme = "assets/audio/Theme2.mp3";
    } else if ((Config::stageNumber(currentLevel) == 3 && !world23) || castle) {
        theme = "assets/audio/Theme3.mp3";
    }
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

    // The middle stage of each world uses the cyan underground artwork. World
    // 2-3 has a dedicated palette cropped from its supplied guide image.
    const bool underground = stage == 2;
    const bool world23 = world == 2 && stage == 3;
    const bool castle = isCastleStage(level);
    tileMap.setTileTexture('#', assets.getTexture(
        world23 ? "World23Ground"
                : (underground ? "GroundUndergroundTile" : "GroundTile")));
    tileMap.setTileTexture('B', assets.getTexture(
        underground ? "BrickUndergroundTile" : "BrickTile"));
    tileMap.setTileTexture('A', assets.getTexture(
        underground ? "BrickUndergroundTile" : "BrickTile"));
    tileMap.setTileTexture('^', assets.getTexture(
        underground ? "BrickUndergroundTile" : "BrickTile"));
    tileMap.setTileTexture('*', assets.getTexture(
        underground ? "BrickUndergroundTile" : "BrickTile"));
    tileMap.setTileTexture('5', assets.getTexture(
        underground ? "BrickUndergroundTile" : "BrickTile"));
    tileMap.setTileTexture('b', assets.getTexture(
        underground ? "BrickUndergroundTile" : "BrickTile"));
    tileMap.setTileTexture('S', assets.getTexture(
        world23 ? "World23HardBlock"
                : (underground ? "HardBlockUndergroundTile" : "HardBlockTile")));
    tileMap.setTileTexture('?', assets.getTexture(
        world23 ? "World23QuestionBlock"
                : (underground ? "QuestionBlockUnderground" : "QuestionBlock")),
        underground && !world23 ? 6 : 4, sf::seconds(0.15f));
    tileMap.setTileTexture('!', assets.getTexture(
        world23 ? "World23QuestionBlock"
                : (underground ? "QuestionBlockUnderground" : "QuestionBlock")),
        underground && !world23 ? 6 : 4, sf::seconds(0.15f));
    tileMap.setTileTexture('o', assets.getTexture(world23 ? "World23Coin" : "Coin"),
                           4, sf::seconds(0.12f));
    tileMap.setTileTexture('U', assets.getTexture(
        world23 ? "World23EmptyBlock" : "EmptyBlock"));
    tileMap.setTileTexture('(', assets.getTexture(
        world23 ? "World23IslandLeft" : "IslandTopLeft"));
    tileMap.setTileTexture('-', assets.getTexture(
        world23 ? "World23IslandMiddle" : "IslandTopMiddle"));
    tileMap.setTileTexture(')', assets.getTexture(
        world23 ? "World23IslandRight" : "IslandTopRight"));
    tileMap.setTileTexture('|', assets.getTexture(
        world23 ? "World23IslandTrunk" : "IslandTrunk"));
    tileMap.setDecorationTexture('l', assets.getTexture(
        world23 ? "World23CloudBig" : "CloudBig"));
    tileMap.setDecorationTexture('c', assets.getTexture(
        world23 ? "World23CloudSmall" : "CloudSmall"));
    tileMap.setDecorationTexture('X', assets.getTexture(
        world23 ? "World23StartCastle" : "Castle"));
    tileMap.setDecorationTexture('Z', assets.getTexture(
        world23 ? "World23EndCastle" : "CastleWorld2_1"));
    tileMap.setDecorationTexture('F', assets.getTexture(
        world23 ? "World23GoalPole" : "Flagpole"));

    // World 3-1 is the night stage. Almost nothing it draws is shared with the
    // daylight stages - white pipes, white trees, pink stone, a black sky - so
    // its whole legend is repointed at the crops taken from its own guide.
    const bool world31 = world == 3 && stage == 1;
    if (world31) {
        tileMap.setTileTexture('#', assets.getTexture("World31Ground"));
        tileMap.setTileTexture('C', assets.getTexture("World31CloudBlock"));
        tileMap.setTileTexture('B', assets.getTexture("World31Brick"));
        tileMap.setTileTexture('A', assets.getTexture("World31Brick"));
        tileMap.setTileTexture('^', assets.getTexture("World31Brick"));
        tileMap.setTileTexture('b', assets.getTexture("World31Brick"));
        tileMap.setTileTexture('S', assets.getTexture("World31HardBlock"));
        tileMap.setTileTexture('s', assets.getTexture("World31HardBlock"));
        tileMap.setTileTexture('?', assets.getTexture("World31QuestionBlock"),
                               4, sf::seconds(0.15f));
        tileMap.setTileTexture('U', assets.getTexture("World31EmptyBlock"));
        tileMap.setTileTexture('[', assets.getTexture("World31PipeTopLeft"));
        tileMap.setTileTexture(']', assets.getTexture("World31PipeTopRight"));
        tileMap.setTileTexture('{', assets.getTexture("World31PipeBodyLeft"));
        tileMap.setTileTexture('}', assets.getTexture("World31PipeBodyRight"));
        tileMap.setTileTexture('=', assets.getTexture("World31BridgeDeck"));
        // Only the hidden room uses map coins, so they take its teal palette.
        tileMap.setTileTexture('o', assets.getTexture("World31RoomCoin"),
                               4, sf::seconds(0.12f));
        tileMap.setTileTexture('0', assets.getTexture("World31Coin"),
                               4, sf::seconds(0.12f));
        tileMap.setTileTexture('g', assets.getTexture("World31RoomGround"));
        tileMap.setTileTexture('r', assets.getTexture("World31RoomBrick"));
        tileMap.setDecorationTexture('~', assets.getTexture("World31BridgeRail"));
        tileMap.setDecorationTexture('T', assets.getTexture("World31TreeTall"));
        tileMap.setDecorationTexture('t', assets.getTexture("World31TreeShort"));
        tileMap.setDecorationTexture('q', assets.getTexture("World31Fence"));
        tileMap.setDecorationTexture('l', assets.getTexture("World31CloudBig"));
        tileMap.setDecorationTexture('c', assets.getTexture("World31CloudSmall"));
        tileMap.setDecorationTexture('Z', assets.getTexture("World31StartCastle"));
        tileMap.setDecorationTexture('X', assets.getTexture("World31EndCastle"));
        tileMap.setDecorationTexture('F', assets.getTexture("World31GoalPole"));
        tileMap.setDecorationTexture('Q', assets.getTexture("World31RoomPipe"));
        tileMap.setDecorationTexture('N', assets.getTexture("World31Vine"));
    }

    const bool world32 = world == 3 && stage == 2;
    if (world32) {
        tileMap.setTileTexture('#', assets.getTexture("World32Ground"));
        tileMap.setTileTexture('B', assets.getTexture("World32Brick"));
        tileMap.setTileTexture('A', assets.getTexture("World32Brick"));
        tileMap.setTileTexture('^', assets.getTexture("World32Brick"));
        tileMap.setTileTexture('b', assets.getTexture("World32Brick"));
        tileMap.setTileTexture('S', assets.getTexture("World32HardBlock"));
        tileMap.setTileTexture('s', assets.getTexture("World32HardBlock"));
        tileMap.setTileTexture('?', assets.getTexture("World32QuestionBlock"),
                               4, sf::seconds(0.15f));
        tileMap.setTileTexture('U', assets.getTexture("World32EmptyBlock"));
        tileMap.setTileTexture('o', assets.getTexture("World32Coin"),
                               4, sf::seconds(0.12f));
        tileMap.setTileTexture('[', assets.getTexture("World32PipeTopLeft"));
        tileMap.setTileTexture(']', assets.getTexture("World32PipeTopRight"));
        tileMap.setTileTexture('{', assets.getTexture("World32PipeBodyLeft"));
        tileMap.setTileTexture('}', assets.getTexture("World32PipeBodyRight"));
        tileMap.setDecorationTexture('T', assets.getTexture("World32TreeTall"));
        tileMap.setDecorationTexture('t', assets.getTexture("World32TreeShort"));
        tileMap.setDecorationTexture('q', assets.getTexture("World32Fence"));
        tileMap.setDecorationTexture('f', assets.getTexture("World32FenceGroup"));
        tileMap.setDecorationTexture('z', assets.getTexture("World32FencePairOffset"));
        tileMap.setDecorationTexture('l', assets.getTexture("World32CloudBig"));
        tileMap.setDecorationTexture('c', assets.getTexture("World32CloudSmall"));
        tileMap.setDecorationTexture('X', assets.getTexture("World32StartCastle"));
        tileMap.setDecorationTexture('Z', assets.getTexture("World32EndCastle"));
        tileMap.setDecorationTexture('F', assets.getTexture("World32GoalPole"));
    }

    // World 3-3 hangs over a single bottomless pit. Its ground only exists at
    // the two ends; everything between is green-capped brick pillars, whose
    // caps ('(', '-', ')') are the solid part and whose trunks ('|') are
    // scenery, exactly like the floating islands in World 1-3.
    const bool world33 = world == 3 && stage == 3;
    if (world33) {
        tileMap.setTileTexture('#', assets.getTexture("World33Ground"));
        tileMap.setTileTexture('(', assets.getTexture("World33PlatformLeft"));
        tileMap.setTileTexture('-', assets.getTexture("World33PlatformMiddle"));
        tileMap.setTileTexture(')', assets.getTexture("World33PlatformRight"));
        tileMap.setTileTexture('|', assets.getTexture("World33Pillar"));
        tileMap.setTileTexture('?', assets.getTexture("World33QuestionBlock"),
                               4, sf::seconds(0.15f));
        tileMap.setTileTexture('U', assets.getTexture("World33EmptyBlock"));
        tileMap.setTileTexture('o', assets.getTexture("World33Coin"),
                               4, sf::seconds(0.12f));
        tileMap.setDecorationTexture('l', assets.getTexture("World33CloudBig"));
        tileMap.setDecorationTexture('c', assets.getTexture("World33CloudSmall"));
        tileMap.setDecorationTexture('X', assets.getTexture("World33StartCastle"));
        tileMap.setDecorationTexture('Z', assets.getTexture("World33EndCastle"));
        tileMap.setDecorationTexture('F', assets.getTexture("World33GoalPole"));
        tileMap.setDecorationTexture('@', assets.getTexture("World33PulleyWide"));
        tileMap.setDecorationTexture('&', assets.getTexture("World33PulleyShort"));
    }

    // Both castle courses are lit by the same torchlit palette, so their whole
    // legend is repointed at the shared crops taken from their guides.
    if (castle) {
        tileMap.setTileTexture('#', assets.getTexture("CastleWall"));
        tileMap.setTileTexture('B', assets.getTexture("CastleBrick"));
        tileMap.setTileTexture('A', assets.getTexture("CastleBrick"));
        tileMap.setTileTexture('^', assets.getTexture("CastleBrick"));
        tileMap.setTileTexture('b', assets.getTexture("CastleBrick"));
        tileMap.setTileTexture('7', assets.getTexture("CastleFireBarBlock"));
        tileMap.setTileTexture('8', assets.getTexture("CastleFireBarBlock"));
        tileMap.setTileTexture('9', assets.getTexture("CastleLavaSurface"));
        tileMap.setTileTexture('%', assets.getTexture("CastleLava"));
        tileMap.setTileTexture('=', assets.getTexture("CastleBridge"));
        tileMap.setTileTexture('?', assets.getTexture("CastleQuestionBlock"),
                               4, sf::seconds(0.15f));
        tileMap.setTileTexture('!', assets.getTexture("CastleQuestionBlock"),
                               4, sf::seconds(0.15f));
        tileMap.setTileTexture('U', assets.getTexture("CastleEmptyBlock"));
        tileMap.setTileTexture('o', assets.getTexture("CastleCoin"),
                               4, sf::seconds(0.12f));
        tileMap.setDecorationTexture('$', assets.getTexture("CastleAxe"));
    }

    if (!tileMap.build(mapParser, Config::kZoom)) {
        std::cerr << "[Core Engine] Warning: World " << world << "-" << stage
                  << " map is empty, nothing to play!\n";
        return false;
    }

    // The hidden 1-Ups stay authored in their destination maps, but only become
    // solid after the required coin challenge in the previous world is cleared.
    const bool world21 = world == 2 && stage == 1;
    const bool oneUpLocked = (world21 && !world13OneUpUnlocked)
                          || (world31 && !world23AllCoinsCollected);
    if (oneUpLocked) {
        const auto& grid = mapParser.getGrid();
        for (std::size_t row = 0; row < grid.size(); ++row) {
            for (std::size_t col = 0; col < grid[row].size(); ++col) {
                if (grid[row][col] == '1') {
                    tileMap.changeType(static_cast<int>(col), static_cast<int>(row),
                                       TileType::Empty);
                }
            }
        }
    }

    currentLevel = level;
    hud.setWorld(currentLevel);
    hud.setTime((world == 1 && stage == 3) || castle
                    || world23 || world32 || world33
                    ? 300.f : 400.f);

    m_entityManager.clear();
    m_cameraSystem.reset();
    growingVines.clear();
    insideSecretRoom = false;
    climbingCoinHeavenVine = false;
    insideCoinHeaven = false;
    coinHeavenClimbElapsed = 0.f;
    swimButtonHeld = false;
    waterAnimationElapsed = sf::Time::Zero;
    waterAnimationFrame = 0;
    world22CheepSpawnTimer = 1.2f;
    flyingCheepSpawnTimer = 0.8f;
    levelCoinsCollected = 0;
    starPowerRemaining = 0.f;
    activeBomb.reset();
    bouncingBlocks.clear();
    coinBlockUsages.clear();
    brickDebris.clear();
    hammers.clear();
    fireBars.clear();
    podoboos.clear();
    castleBoss = CastleBossEntity{};
    castleBossFire.clear();
    castleClear = CastleClearSequence{};

    spawnWalkingEnemies();
    spawnAquaticEnemies();
    spawnPiranhas();
    spawnMovingPlatforms();
    spawnTrampolines();
    spawnCastleHazards();
    prepareItemBlockRewards();

    std::cout << "[Core Engine] World " << world << "-" << stage << " loaded: "
              << mapParser.getWidth() << "x" << mapParser.getHeight() << " tiles, "
              << tileMap.enemySpawns().size() << " Goombas, "
              << tileMap.blueKoopaSpawns().size() << " Blue Koopas, "
              << tileMap.greenKoopaSpawns().size() << " Green Koopas, "
              << tileMap.redKoopaSpawns().size() << " Red Koopas, "
              << tileMap.greenParatroopaSpawns().size() << " Green Paratroopas, "
              << tileMap.redParatroopaSpawns().size() << " Red Paratroopas, "
              << tileMap.blooperSpawns().size() << " Bloopers, "
              << tileMap.cheepCheepSpawns().size() << " Cheep-Cheeps, "
              << tileMap.piranhaSpawns().size() << " Piranha Plants, "
              << tileMap.trampolineSpawns().size() << " trampolines, "
              << tileMap.movingPlatformSpawns().size() << " moving lifts, "
              << tileMap.verticalPlatformSpawns().size() << " up-and-down lifts, "
              << (tileMap.risingElevatorSpawns().size()
                  + tileMap.fallingElevatorSpawns().size()) << " castle elevators, "
              << tileMap.fireBarSpawns().size() << " Fire-Bars, "
              << tileMap.podobooSpawns().size() << " Podoboos, "
              << std::min(tileMap.balanceLeftSpawns().size(),
                          tileMap.balanceRightSpawns().size()) << " pulleys\n";
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

bool PlayState::tryLeaveWorld22WaterPipe() {
    if (Config::worldNumber(currentLevel) != 2
        || Config::stageNumber(currentLevel) != 2 || m_player == nullptr) {
        return false;
    }
    if (inputHandler.getPlayerInput().moveAxis <= 0.f
        && !heldKeys.count(sf::Keyboard::Scancode::D)
        && !heldKeys.count(sf::Keyboard::Scancode::Right)) {
        return false;
    }

    const float tile = tileMap.tileSize();
    const sf::FloatRect exitMouth(
        {kWorld22ExitMouthColumn * tile, kWorld22ExitMouthRow * tile},
        {4.f * tile, 4.f * tile});
    if (!avatarBounds().findIntersection(exitMouth).has_value()) {
        return false;
    }

    warpAvatarTo({kWorld22OutdoorPipeColumn * tile,
                  kWorld22OutdoorPipeArrivalRow * tile});
    avatar.setPosition(m_player->getPhysicsBody().getPosition());
    swimButtonHeld = false;
    const float outdoorCameraX = (kWorld22WaterEndColumn
                                + Config::kViewTilesX * 0.5f) * tile;
    m_cameraSystem.setMaxCameraCenterX(outdoorCameraX);
    m_cameraSystem.centreCamera(
        {outdoorCameraX, Config::kViewHeight * 0.5f},
        tileMap.pixelWidth(), tileMap.pixelHeight());
    std::cout << "[Core Engine] World 2-2 exit pipe: returned above water.\n";
    return true;
}

void PlayState::warpAvatarTo(sf::Vector2f cell) {
    auto& body = m_player->getPhysicsBody();
    const sf::Vector2f size = body.getColliderSize();
    const sf::Vector2f position{cell.x + (tileMap.tileSize() - size.x) * 0.5f,
                                cell.y + tileMap.tileSize() - size.y};

    body.setPosition(position);
    body.setVelocity({0.f, 0.f});
    body.clearAcceleration();
    body.setGrounded(false);
    facingRight = true;
    syncAvatarPowerVisuals();

    // The camera lock only ever moves forwards during play, so a warp has to
    // hand it its new anchor itself - otherwise coming back from a room far to
    // the right would leave the stage stuck off-screen.
    m_cameraSystem.setMaxCameraCenterX(position.x);
    m_cameraSystem.centreCamera({position.x, Config::kViewHeight / 2.f},
                                tileMap.pixelWidth(), tileMap.pixelHeight());
}

bool PlayState::tryEnterSecretRoom() {
    const TileMap::SecretRoomWarp& warp = tileMap.secretRoom();
    if (!warp.available || insideSecretRoom || insideCoinHeaven || m_player == nullptr) {
        return false;
    }
    if (!inputHandler.getPlayerInput().crouchHeld
        || !m_player->getPhysicsBody().isGrounded()) {
        return false;
    }
    if (!avatarBounds().findIntersection(warp.entrance).has_value()) {
        return false;
    }

    insideSecretRoom = true;
    warpAvatarTo(warp.arrival);
    playLevelMusic();

    std::cout << "[Core Engine] Warp pipe: dropped into the hidden room.\n";
    return true;
}

bool PlayState::tryLeaveSecretRoom() {
    const TileMap::SecretRoomWarp& warp = tileMap.secretRoom();
    if (!warp.available || !insideSecretRoom || m_player == nullptr) {
        return false;
    }
    if (inputHandler.getPlayerInput().moveAxis <= 0.f) {
        return false;
    }
    if (!avatarBounds().findIntersection(warp.exitMouth).has_value()) {
        return false;
    }

    insideSecretRoom = false;
    warpAvatarTo(warp.returnCell);
    playLevelMusic();

    std::cout << "[Core Engine] Warp pipe: back above ground.\n";
    return true;
}

bool PlayState::tryStartCoinHeavenClimb() {
    const TileMap::CoinHeavenWarp& warp = tileMap.coinHeavenWarp();
    if (!warp.available || insideSecretRoom || insideCoinHeaven
        || climbingCoinHeavenVine || m_player == nullptr
        || !inputHandler.getPlayerInput().jumpHeld) {
        return false;
    }

    const auto grownVine = std::find_if(
        growingVines.begin(), growingVines.end(),
        [&warp](const GrowingVineEntity& vine) {
            return vine.elapsed >= kVineGrowDuration
                && std::abs(vine.blockPosition.x - warp.vineBlock.x) < 1.f
                && std::abs(vine.blockPosition.y - warp.vineBlock.y) < 1.f;
        });
    if (grownVine == growingVines.end()) {
        return false;
    }

    const sf::Texture& vineTexture = assets.getTexture("World31Vine");
    const float vineHeight = static_cast<float>(vineTexture.getSize().y) * Config::kZoom;
    const float tile = tileMap.tileSize();
    const sf::FloatRect grabBounds(
        {grownVine->blockPosition.x - tile * 0.25f,
         grownVine->blockPosition.y + tile - vineHeight
             + tile * kVineLowerOffsetTiles},
        {tile * 1.5f, vineHeight});
    if (!avatarBounds().findIntersection(grabBounds).has_value()) {
        return false;
    }

    climbingCoinHeavenVine = true;
    coinHeavenClimbElapsed = 0.f;
    coinHeavenClimbStart = m_player->getPhysicsBody().getPosition();
    coinHeavenVinePosition = grownVine->blockPosition;

    auto& body = m_player->getPhysicsBody();
    body.setVelocity({0.f, 0.f});
    body.clearAcceleration();
    body.setGrounded(false);
    std::cout << "[Core Engine] Coin Heaven vine: started climbing.\n";
    return true;
}

bool PlayState::updateCoinHeavenClimb(sf::Time dt) {
    if (!climbingCoinHeavenVine || m_player == nullptr) {
        return false;
    }

    coinHeavenClimbElapsed += dt.asSeconds();
    const float progress = std::min(1.f, coinHeavenClimbElapsed / kVineClimbDuration);
    auto& body = m_player->getPhysicsBody();
    const sf::Vector2f size = body.getColliderSize();
    const float centredX = coinHeavenVinePosition.x
                         + (tileMap.tileSize() - size.x) * 0.5f;
    const sf::Vector2f position{
        coinHeavenClimbStart.x + (centredX - coinHeavenClimbStart.x) * progress,
        coinHeavenClimbStart.y
            - progress * tileMap.tileSize() * kVineClimbDistanceTiles};
    body.setPosition(position);
    body.setVelocity({0.f, 0.f});
    body.clearAcceleration();
    body.setGrounded(false);
    avatar.setPosition(position);

    animator.setAction(entity::PlayerAction::Climb);
    animator.setFacingRight(facingRight);
    animator.setForm(currentPlayerForm());
    animator.update(dt);

    if (progress >= 1.f) {
        climbingCoinHeavenVine = false;
        insideCoinHeaven = true;
        warpAvatarTo(tileMap.coinHeavenWarp().arrival);
        avatar.setPosition(m_player->getPhysicsBody().getPosition());
        std::cout << "[Core Engine] Coin Heaven vine: entered the sky bonus.\n";
    }
    return true;
}

bool PlayState::tryLeaveCoinHeaven() {
    const TileMap::CoinHeavenWarp& warp = tileMap.coinHeavenWarp();
    if (!warp.available || !insideCoinHeaven || m_player == nullptr) {
        return false;
    }

    const auto& body = m_player->getPhysicsBody();
    if (body.getPosition().x < warp.exitX
        || body.getPosition().y <= tileMap.pixelHeight()) {
        return false;
    }

    insideCoinHeaven = false;
    warpAvatarTo(warp.returnCell);
    avatar.setPosition(m_player->getPhysicsBody().getPosition());
    std::cout << "[Core Engine] Coin Heaven D+: returned to the main stage.\n";
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

    const bool reachedMapEnd = !insideSecretRoom && !insideCoinHeaven
        && m_player->getPhysicsBody().getPosition().x
            >= tileMap.pixelWidth() - tileMap.tileSize() * 3.f;

    if (!reachedLevelExit && !reachedStageGoal && !reachedMapEnd) {
        return false;
    }

    if (isCastleStage(currentLevel) && reachedStageGoal) {
        startCastleClearSequence();
        return true;
    }

    if (transitionPending) return false;
    transitionPending = true;
    if (Config::worldNumber(currentLevel) == 1
        && Config::stageNumber(currentLevel) == 3) {
        world13OneUpUnlocked = levelCoinsCollected >= kWorld13OneUpCoinRequirement;
        std::cout << "[Core Engine] World 1-3 coin challenge: "
                  << levelCoinsCollected << "/23 collected; World 2-1 hidden 1-Up "
                  << (world13OneUpUnlocked ? "unlocked.\n" : "remains locked.\n");
    } else if (Config::worldNumber(currentLevel) == 2
        && Config::stageNumber(currentLevel) == 3) {
        world23AllCoinsCollected = levelCoinsCollected >= kWorld23CoinTotal;
        std::cout << "[Core Engine] World 2-3 coin challenge: "
                  << levelCoinsCollected << "/" << kWorld23CoinTotal
                  << (world23AllCoinsCollected
                          ? " collected; World 3-1 hidden 1-Up unlocked.\n"
                          : " collected; World 3-1 hidden 1-Up remains locked.\n");
    }
    std::cout << "[Core Engine] Level Complete reached! Transitioning to LevelCompleteState...\n";
    gsm.changeState(std::make_unique<LevelCompleteState>(gsm, assets, getSaveData()));
    return true;
}

void PlayState::spawnCoinPop(sf::Vector2f blockPosition) {
    const bool world23 = Config::worldNumber(currentLevel) == 2
                      && Config::stageNumber(currentLevel) == 3;
    const bool world31 = Config::worldNumber(currentLevel) == 3
                      && Config::stageNumber(currentLevel) == 1;
    const bool world32 = Config::worldNumber(currentLevel) == 3
                      && Config::stageNumber(currentLevel) == 2;
    const char* coinArt = world23 ? "World23Coin"
                        : (world31 ? "World31Coin"
                                   : (world32 ? "World32Coin"
                                              : (isCastleStage(currentLevel)
                                                     ? "CastleCoin" : "Coin")));
    m_entityManager.addEntity(entity::EntityFactory::createCoinPop(
        blockPosition, tileMap.tileSize(), &assets.getTexture(coinArt)));
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
    const bool alreadyGrowing = std::any_of(
        growingVines.begin(), growingVines.end(),
        [blockPosition](const GrowingVineEntity& vine) {
            return std::abs(vine.blockPosition.x - blockPosition.x) < 1.f
                && std::abs(vine.blockPosition.y - blockPosition.y) < 1.f;
        });
    if (alreadyGrowing) {
        return false;
    }
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
    const bool world31 = Config::worldNumber(currentLevel) == 3
                      && Config::stageNumber(currentLevel) == 1;
    const sf::Texture& texture = assets.getTexture(
        world31 ? "World31Vine" : "VineTop");
    sf::Sprite vineSprite(texture);
    const int sourceWidth = static_cast<int>(texture.getSize().x);
    const int sourceHeight = static_cast<int>(texture.getSize().y);
    vineSprite.setScale({Config::kZoom, Config::kZoom});
    for (const auto& vine : growingVines) {
        float progress = std::min(1.f, vine.elapsed / kVineGrowDuration);
        const int visibleHeight = std::max(
            1, static_cast<int>(std::ceil(sourceHeight * progress)));
        vineSprite.setTextureRect(sf::IntRect(
            {0, sourceHeight - visibleHeight}, {sourceWidth, visibleHeight}));
        const float drawWidth = sourceWidth * Config::kZoom;
        vineSprite.setPosition({
            vine.blockPosition.x + (tileMap.tileSize() - drawWidth) * 0.5f,
            vine.blockPosition.y + tileMap.tileSize()
                - visibleHeight * Config::kZoom
                + tileMap.tileSize() * kVineLowerOffsetTiles});
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

        if (castleBoss.available && castleBoss.alive && castleBoss.active
            && bulletBounds.findIntersection(castleBossBounds()).has_value()) {
            --castleBoss.health;
            score += castleBoss.health <= 0 ? 5000 : 200;
            if (castleBoss.health <= 0) {
                castleBoss.alive = false;
                castleBoss.revealedGoomba = true;
                castleBoss.revealTimer = 1.35f;
                castleBoss.revealPosition = castleBoss.position
                    + sf::Vector2f{tileMap.tileSize() * 0.5f,
                                   tileMap.tileSize()};
                castleBoss.revealVelocity = {0.f, -180.f};
                castleBossFire.clear();
            }
            hud.setScore(score);
            Systems::SoundController::getInstance().playSound(
                assets.getSoundBuffer("StompSound"));
            hitEnemy = true;
        }

        if (!hitEnemy) {
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
    // Six 16x16 cells in a 3x2 grid, drawn two tiles across.
    constexpr int kExplosionCell = 16;
    sf::Sprite expSprite(assets.getTexture("Explosion"));
    expSprite.setScale({32.f / kExplosionCell, 32.f / kExplosionCell});

    for (const ExplosionEntity& exp : explosions) {
        int col = exp.currentFrame % 3;
        int row = exp.currentFrame / 3;
        expSprite.setTextureRect(sf::IntRect({col * kExplosionCell, row * kExplosionCell},
                                             {kExplosionCell, kExplosionCell}));
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
    const bool world23 = Config::worldNumber(currentLevel) == 2
                      && Config::stageNumber(currentLevel) == 3;
    const bool world31 = Config::worldNumber(currentLevel) == 3
                      && Config::stageNumber(currentLevel) == 1;
    const bool world32 = Config::worldNumber(currentLevel) == 3
                      && Config::stageNumber(currentLevel) == 2;
    const bool castle = isCastleStage(currentLevel);
    const bool underground = Config::stageNumber(currentLevel) == 2 && !world32;
    const char* brickArt = castle ? "CastleBrick"
                         : (world31 ? "World31Brick"
                                    : (world32 ? "World32Brick"
                                               : (underground ? "BrickUndergroundTile"
                                                              : "BrickTile")));

    // Draw Bouncing Blocks
    for (const auto& block : bouncingBlocks) {
        char drawSymbol = block.originalSymbol;

        const sf::Texture* tex = nullptr;
        sf::IntRect rect;

        if (drawSymbol == 'B' || drawSymbol == '^' || drawSymbol == 'A'
            || drawSymbol == 'b') {
            tex = &assets.getTexture(brickArt);
        } else if (drawSymbol == '?' || drawSymbol == '!') {
            const char* questionArt = world23 ? "World23QuestionBlock"
                                    : (world31 ? "World31QuestionBlock"
                                               : (world32 ? "World32QuestionBlock"
                                                          : (castle ? "CastleQuestionBlock"
                                                                    : (underground ? "QuestionBlockUnderground" : "QuestionBlock"))));
            tex = &assets.getTexture(questionArt);
            rect = sf::IntRect({0, 0}, {16, 16});
        } else if (drawSymbol == 'U') {
            const char* emptyArt = world23 ? "World23EmptyBlock"
                                 : (world31 ? "World31EmptyBlock"
                                            : (world32 ? "World32EmptyBlock"
                                                       : (castle ? "CastleEmptyBlock" : "EmptyBlock")));
            tex = &assets.getTexture(emptyArt);
        }
        
        if (tex) {
            sf::Sprite sprite(*tex);
            if (drawSymbol == '?' || drawSymbol == '!') {
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
        sf::Sprite debrisSprite(assets.getTexture(brickArt));
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
    const bool world32 = Config::worldNumber(currentLevel) == 3
                      && Config::stageNumber(currentLevel) == 2;
    const bool underground = Config::stageNumber(currentLevel) == 2
                          && Config::worldNumber(currentLevel) != 2 && !world32;
    const auto& goombaTex = assets.getTexture(underground ? "GoombaUnderground" : "Goomba");
    const auto& blueKoopaTex = assets.getTexture("BlueKoopaUnderground");
    const auto& blueShellTex = assets.getTexture("BlueShell");
    // World 1-2's three Green Koopas use the underground cyan palette while
    // keeping Green Koopa behavior (unlike Red Koopas, they walk off ledges).
    const auto& greenKoopaTex = assets.getTexture(
        underground ? "BlueKoopaUnderground" : "GreenKoopa");
    const auto& shellTex = assets.getTexture(
        underground ? "BlueShell" : "GreenShell");
    const auto& paratroopaTex = assets.getTexture("GreenParatroopa");
    const auto& redKoopaTex = assets.getTexture("RedKoopa");
    const auto& redParatroopaTex = assets.getTexture("RedParatroopa");
    const auto& redShellTex = assets.getTexture("RedShell");

    // Anything past the opening screen waits until the camera reaches it, the
    // way the original spawns its enemies. On a stage of separate islands like
    // World 3-3 they would otherwise walk off their platform and into the pit
    // long before the player got there.
    auto place = [this](std::unique_ptr<entity::Entity> enemy, sf::Vector2f spawn) {
        if (spawn.x > Config::kViewWidth) {
            enemy->setActive(false);
        }
        m_entityManager.addEntity(std::move(enemy));
    };

    for (sf::Vector2f spawn : tileMap.enemySpawns()) {
        place(entity::EntityFactory::createGoomba(spawn, tileMap.tileSize(), &goombaTex), spawn);
    }
    const float koopaHeightOffset = tileMap.tileSize() * 0.5f;
    for (sf::Vector2f spawn : tileMap.blueKoopaSpawns()) {
        spawn.y -= koopaHeightOffset;
        place(entity::EntityFactory::createKoopa(
            spawn, tileMap.tileSize(), &blueKoopaTex, &blueShellTex,
            entity::KoopaKind::BlueUnderground), spawn);
    }
    for (sf::Vector2f spawn : tileMap.greenKoopaSpawns()) {
        spawn.y -= koopaHeightOffset;
        place(entity::EntityFactory::createKoopa(spawn, tileMap.tileSize(), &greenKoopaTex, &shellTex, entity::KoopaKind::Green), spawn);
    }
    for (sf::Vector2f spawn : tileMap.redKoopaSpawns()) {
        spawn.y -= koopaHeightOffset;
        place(entity::EntityFactory::createKoopa(
            spawn, tileMap.tileSize(), &redKoopaTex, &redShellTex,
            entity::KoopaKind::Red), spawn);
    }
    const float hammerBroHeightOffset = tileMap.tileSize() * 0.5f;
    for (sf::Vector2f spawn : tileMap.hammerBroSpawns()) {
        // The marker is a floor cell but a Bro is a tile and a half tall.
        spawn.y -= hammerBroHeightOffset;
        place(entity::EntityFactory::createHammerBro(
            spawn, tileMap.tileSize(), &assets.getTexture("HammerBro")), spawn);
    }
    for (sf::Vector2f spawn : tileMap.greenParatroopaSpawns()) {
        // The lone 3-2 Paratroopa starts two and a half tiles above the path.
        spawn.y -= world32 ? tileMap.tileSize() : koopaHeightOffset;
        place(entity::EntityFactory::createParatroopa(spawn, tileMap.tileSize(), &paratroopaTex, &shellTex), spawn);
    }
    for (sf::Vector2f spawn : tileMap.redParatroopaSpawns()) {
        spawn.y -= koopaHeightOffset;
        // Red Paratroopas hover vertically; after losing their wings they use
        // the same red shell and ledge-aware walking behaviour as Red Koopas.
        place(entity::EntityFactory::createParatroopa(
            spawn, tileMap.tileSize(), &redParatroopaTex, &redShellTex,
            0.f, 600.f, entity::KoopaKind::Red), spawn);
    }

}

void PlayState::updateEnemyReactions() {
    if (!m_player) {
        return;
    }

    const sf::FloatRect player = avatarBounds();
    const float playerCentreX = player.position.x + player.size.x * 0.5f;
    const float safeDistance = kPiranhaSafeTiles * tileMap.tileSize();

    m_entityManager.forEach([&](entity::Entity& target) {
        if (!target.isAlive()) {
            return;
        }
        if (auto* piranha = dynamic_cast<entity::PiranhaPlant*>(&target)) {
            // Standing on a pipe is how the avatar reaches the warp below, so
            // the plant inside waits underground until he has moved off it.
            const float pipeCentreX = piranha->getPosition().x + tileMap.tileSize() * 0.5f;
            piranha->setPlayerNearby(std::abs(playerCentreX - pipeCentreX) < safeDistance);
            return;
        }
        if (auto* bro = dynamic_cast<entity::HammerBro*>(&target)) {
            if (!bro->isActive()) {
                return;
            }
            bro->faceTowards(playerCentreX);
            if (bro->takePendingThrow()) {
                const float direction = bro->isFacingRight() ? 1.f : -1.f;
                hammers.emplace_back(
                    assets.getTexture("Hammer"), bro->hammerSpawnPoint(),
                    sf::Vector2f{kHammerThrowSpeedX * direction, kHammerThrowSpeedY},
                    Config::kZoom);
            }
        }
    });
}

void PlayState::updateHammers(sf::Time dt) {
    const bool shielded = starPowerRemaining > 0.f || damageProtectionRemaining > 0.f
                       || invincibleTimer > 0.f;
    const sf::FloatRect player = avatarBounds();

    for (auto& hammer : hammers) {
        hammer.update(dt, kGravity);
        if (shielded || death.active) {
            continue;
        }
        if (!hammer.bounds().findIntersection(player).has_value()) {
            continue;
        }
        // Hammers cannot be jumped on, only avoided: contact always costs a form.
        if (m_player->removeLatestPower()) {
            syncAvatarPowerVisuals();
            damageProtectionRemaining = kDamageProtectionDuration;
            invincibleTimer = 1.5f;
            Systems::SoundController::getInstance().playSound(
                assets.getSoundBuffer("DowngradeSound"));
        } else {
            handlePlayerDeath();
        }
        break;
    }

    const float dropOut = tileMap.pixelHeight() + tileMap.tileSize();
    const float behindCamera = m_cameraSystem.getCenter().x - Config::kViewWidth;
    hammers.erase(
        std::remove_if(hammers.begin(), hammers.end(), [&](const entity::Hammer& hammer) {
            return hammer.position().y > dropOut || hammer.position().x < behindCamera;
        }),
        hammers.end());
}

void PlayState::drawHammers(sf::RenderWindow& window) const {
    for (const auto& hammer : hammers) {
        hammer.draw(window);
    }
}

void PlayState::spawnAquaticEnemies() {
    if (tileMap.blooperSpawns().empty() && tileMap.cheepCheepSpawns().empty()) {
        return;
    }

    const float tile = tileMap.tileSize();
    const sf::FloatRect swimBounds(
        {kWorld22WaterStartColumn * tile, kWorld22WaterSurfaceRow * tile},
        {(kWorld22WaterEndColumn - kWorld22WaterStartColumn) * tile,
         (kWorld22WaterFloorRow - kWorld22WaterSurfaceRow) * tile}
    );
    const auto& blooperTexture = assets.getTexture("Blooper");
    const auto& cheepCheepTexture = assets.getTexture("CheepCheep");

    for (const sf::Vector2f spawn : tileMap.blooperSpawns()) {
        m_entityManager.addEntity(entity::EntityFactory::createBlooper(
            spawn, tile, swimBounds, &blooperTexture));
    }
    for (const sf::Vector2f spawn : tileMap.cheepCheepSpawns()) {
        m_entityManager.addEntity(entity::EntityFactory::createCheepCheep(
            spawn, tile, swimBounds, &cheepCheepTexture));
    }
}

void PlayState::updateAquaticEnemyTargets() {
    if (!m_player) {
        return;
    }

    const physics::AABB playerBounds = m_player->getPhysicsBody().getAABB();
    const sf::Vector2f target(
        playerBounds.position.x + playerBounds.size.x * 0.5f,
        playerBounds.position.y + playerBounds.size.y * 0.5f
    );
    m_entityManager.forEach([target](entity::Entity& managed) {
        if (auto* blooper = dynamic_cast<entity::Blooper*>(&managed)) {
            blooper->setTarget(target);
        }
    });
}

void PlayState::updateWorld22CheepSpawner(sf::Time dt) {
    if (Config::worldNumber(currentLevel) != 2
        || Config::stageNumber(currentLevel) != 2 || m_player == nullptr) {
        return;
    }

    const float tile = tileMap.tileSize();
    const physics::AABB player = m_player->getPhysicsBody().getAABB();
    const float playerCentreX = player.position.x + player.size.x * 0.5f;
    const float playerCentreY = player.position.y + player.size.y * 0.5f;
    const float waterTop = kWorld22WaterSurfaceRow * tile;
    const float waterRight = kWorld22WaterEndColumn * tile;
    if (playerCentreX < kWorld22WaterStartColumn * tile
        || playerCentreX >= waterRight - tile * 4.f
        || playerCentreY < waterTop) {
        return;
    }

    world22CheepSpawnTimer -= dt.asSeconds();
    if (world22CheepSpawnTimer > 0.f) {
        return;
    }

    const sf::FloatRect swimBounds(
        {kWorld22WaterStartColumn * tile, waterTop},
        {(kWorld22WaterEndColumn - kWorld22WaterStartColumn) * tile,
         (kWorld22WaterFloorRow - kWorld22WaterSurfaceRow) * tile});
    const float cameraRight = m_cameraSystem.getCenter().x
                            + Config::kViewWidth * 0.5f;
    const float spawnX = std::min(waterRight - tile,
                                  cameraRight + tile * 0.75f);
    constexpr int firstLaneRow = kWorld22WaterSurfaceRow + 2;
    constexpr int laneCount = kWorld22WaterFloorRow - firstLaneRow - 1;
    const int lane = static_cast<int>(rewardRandom() % laneCount);
    const float spawnY = (firstLaneRow + lane) * tile;
    const float speed = 84.f + static_cast<float>(rewardRandom() % 49U);

    auto fish = entity::EntityFactory::createCheepCheep(
        {spawnX, spawnY}, tile, swimBounds, &assets.getTexture("CheepCheep"), speed);
    fish->setActive(true);
    m_entityManager.addEntity(std::move(fish));

    const float randomDelay = static_cast<float>(rewardRandom() % 121U) / 100.f;
    world22CheepSpawnTimer = kWorld22CheepMinDelay
                           + std::min(randomDelay, kWorld22CheepDelayRange);
}

void PlayState::updateFlyingCheepSpawner(sf::Time dt) {
    if (Config::worldNumber(currentLevel) != 2
        || Config::stageNumber(currentLevel) != 3 || !m_player) {
        return;
    }

    flyingCheepSpawnTimer -= dt.asSeconds();
    const float tile = tileMap.tileSize();
    const auto& body = m_player->getPhysicsBody().getAABB();
    const float playerCenterX = body.position.x + body.size.x * 0.5f;
    if (playerCenterX < kWorld23FishStartColumn * tile
        || playerCenterX > kWorld23FishEndColumn * tile
        || flyingCheepSpawnTimer > 0.f) {
        return;
    }

    // At running speed the fish usually erupts behind Mario; while walking or
    // standing it usually approaches from the front. A one-in-four exception
    // keeps the stream unpredictable without losing that original bias.
    const float playerVelocityX = m_player->getPhysicsBody().getVelocity().x;
    const bool playerRunning = std::abs(playerVelocityX) >= kMaxWalkSpeed * 0.65f;
    const unsigned approachRoll = rewardRandom() % 4U;
    const bool spawnAhead = playerRunning ? approachRoll == 0U
                                          : approachRoll != 0U;
    const float travelDirection = playerVelocityX < -1.f ? -1.f : 1.f;
    const float horizontalSpeed = 135.f
        + static_cast<float>(rewardRandom() % 91U);
    const float horizontalOffset = (spawnAhead ? 5.f : -3.f)
                                 * tile * travelDirection;
    const float jitter = static_cast<float>(static_cast<int>(rewardRandom() % 5U) - 2)
                       * tile * 0.5f;
    const float spawnX = std::clamp(
        playerCenterX + horizontalOffset + jitter,
        tile, tileMap.pixelWidth() - tile * 2.f);
    const sf::Vector2f spawnPosition(
        spawnX, tileMap.pixelHeight() + tile * 0.25f);
    const sf::Vector2f launchVelocity(
        (spawnAhead ? -horizontalSpeed : horizontalSpeed) * travelDirection,
        -920.f - static_cast<float>(rewardRandom() % 121U));
    const sf::FloatRect flightBounds(
        {0.f, 0.f}, {tileMap.pixelWidth(), tileMap.pixelHeight()});

    m_entityManager.addEntity(entity::EntityFactory::createFlyingCheepCheep(
        spawnPosition, launchVelocity, tile, flightBounds,
        &assets.getTexture("FlyingCheepCheep")));
    flyingCheepSpawnTimer = 0.7f
        + static_cast<float>(rewardRandom() % 61U) / 100.f;
}

void PlayState::spawnPiranhas() {
    const float tile = tileMap.tileSize();
    const float scale = tile / TileMap::kSourceTileSize;
    const auto& piranhaTex = assets.getTexture("PiranhaPlant");
    for (const sf::Vector2f marker : tileMap.piranhaSpawns()) {
        const int markerColumn = static_cast<int>(marker.x / tile);
        const int pipeRow = static_cast<int>(marker.y / tile) + 2;

        // The marker only names one column of the pipe, so measure the whole
        // mouth and grow the plant out of its middle. Marking either column of
        // a two-tile pipe - or a wider one - then lands in the same place.
        int firstColumn = markerColumn;
        int lastColumn = markerColumn;
        while (tileMap.symbolAt(firstColumn - 1, pipeRow) == '[') {
            --firstColumn;
        }
        while (tileMap.symbolAt(lastColumn + 1, pipeRow) == ']') {
            ++lastColumn;
        }
        const float mouthCentreX = (firstColumn + lastColumn + 1) * 0.5f * tile;

        const float pipeTopY = marker.y + tile * 2.f;
        const sf::Vector2f shownPosition(mouthCentreX - 8.f * scale,
                                         pipeTopY - 23.f * scale);
        m_entityManager.addEntity(entity::EntityFactory::createPiranhaPlant(shownPosition, pipeTopY, &piranhaTex, scale));
    }
}

void PlayState::spawnMovingPlatforms() {
    movingPlatforms.clear();
    const bool castle = isCastleStage(currentLevel);
    const float tile = tileMap.tileSize();
    for (sf::Vector2f pos : tileMap.movingPlatformSpawns()) {
        MovingPlatform platform{
            pos, pos, pos, {kMovingPlatformSpeed, 0.f},
            LiftMotion::Horizontal, -1, 0.f,
            castle ? kCastleLiftWidthTiles : kMovingPlatformWidthTiles};
        if (castle) {
            // Both guides catch the bridge-room lift at the right end of its
            // pass. Centre its normal six-tile range three cells to the left
            // so it stays above the bridge instead of running into the axe.
            platform.origin.x -= kMovingPlatformRangeTiles * tile;
            platform.velocity.x = -kMovingPlatformSpeed;
        }
        movingPlatforms.push_back(platform);
    }
    for (sf::Vector2f pos : tileMap.verticalPlatformSpawns()) {
        movingPlatforms.push_back({pos, pos, pos, {0.f, kVerticalLiftSpeed},
                                   LiftMotion::Vertical, -1, 0.f});
    }

    // The castle elevators are narrower than their marker cell and hang
    // centred on it, and they run one way for the whole stage.
    const float elevatorInset = (1.f - kCastleElevatorWidthTiles) * 0.5f * tile;
    auto addElevator = [&](sf::Vector2f pos, float speed) {
        pos.x += elevatorInset;
        movingPlatforms.push_back({pos, pos, pos, {0.f, speed},
                                   LiftMotion::Elevator, -1, 0.f,
                                   kCastleElevatorWidthTiles});
    };
    for (sf::Vector2f pos : tileMap.risingElevatorSpawns()) {
        addElevator(pos, -kCastleElevatorSpeed);
    }
    for (sf::Vector2f pos : tileMap.fallingElevatorSpawns()) {
        addElevator(pos, kCastleElevatorSpeed);
    }

    // A stage may carry several pulleys, so the two ends are matched left to
    // right: the leftmost '/' belongs to the leftmost '\', and so on.
    std::vector<sf::Vector2f> lefts = tileMap.balanceLeftSpawns();
    std::vector<sf::Vector2f> rights = tileMap.balanceRightSpawns();
    auto leftToRight = [](const sf::Vector2f& a, const sf::Vector2f& b) {
        return a.x < b.x;
    };
    std::sort(lefts.begin(), lefts.end(), leftToRight);
    std::sort(rights.begin(), rights.end(), leftToRight);

    const std::size_t pulleys = std::min(lefts.size(), rights.size());
    for (std::size_t i = 0; i < pulleys; ++i) {
        const int first = static_cast<int>(movingPlatforms.size());
        movingPlatforms.push_back({lefts[i], lefts[i], lefts[i], {},
                                   LiftMotion::Balance, first + 1,
                                   pulleyRopeTop(lefts[i])});
        movingPlatforms.push_back({rights[i], rights[i], rights[i], {},
                                   LiftMotion::Balance, first,
                                   pulleyRopeTop(rights[i])});
    }
}

bool PlayState::isStandingOnPlatform(sf::Vector2f position,
                                     float widthTiles) const {
    if (!m_player || death.active) {
        return false;
    }
    const sf::FloatRect player = avatarBounds();
    const float feet = player.position.y + player.size.y;
    const float right = position.x + tileMap.tileSize() * widthTiles;
    return player.position.x + player.size.x > position.x + 2.f
        && player.position.x < right - 2.f
        && feet >= position.y - 4.f
        && feet <= position.y + tileMap.tileSize() * 0.5f;
}

float PlayState::pulleyRopeTop(sf::Vector2f platformPos) const {
    const float tile = tileMap.tileSize();
    const float centre = platformPos.x + tile * kMovingPlatformWidthTiles * 0.5f;
    const auto& grid = mapParser.getGrid();

    for (std::size_t row = 0; row < grid.size(); ++row) {
        for (std::size_t col = 0; col < grid[row].size(); ++col) {
            const char symbol = grid[row][col];
            if (symbol != '@' && symbol != '&') {
                continue;
            }
            const auto& header = assets.getTexture(
                symbol == '@' ? "World33PulleyWide" : "World33PulleyShort");
            const float left = col * tile;
            const float width = static_cast<float>(header.getSize().x) * Config::kZoom;
            if (centre >= left && centre <= left + width) {
                // The wheels fill the header's own row; the rope starts below it.
                return (row + 1) * tile;
            }
        }
    }
    return 0.f;
}

void PlayState::updateMovingPlatforms(sf::Time dt) {
    const float seconds = dt.asSeconds();
    const float range = kMovingPlatformRangeTiles * tileMap.tileSize();

    for (auto& plat : movingPlatforms) {
        plat.previousPosition = plat.position;
    }

    for (std::size_t i = 0; i < movingPlatforms.size(); ++i) {
        MovingPlatform& plat = movingPlatforms[i];
        switch (plat.motion) {
        case LiftMotion::Horizontal:
            plat.position.x += plat.velocity.x * seconds;
            if (plat.position.x > plat.origin.x + range) {
                plat.position.x = plat.origin.x + range;
                plat.velocity.x = -kMovingPlatformSpeed;
            } else if (plat.position.x < plat.origin.x - range) {
                plat.position.x = plat.origin.x - range;
                plat.velocity.x = kMovingPlatformSpeed;
            }
            break;

        case LiftMotion::Vertical:
            plat.position.y += plat.velocity.y * seconds;
            if (plat.position.y > plat.origin.y + range) {
                plat.position.y = plat.origin.y + range;
                plat.velocity.y = -kVerticalLiftSpeed;
            } else if (plat.position.y < plat.origin.y - range) {
                plat.position.y = plat.origin.y - range;
                plat.velocity.y = kVerticalLiftSpeed;
            }
            break;

        case LiftMotion::Elevator: {
            // An elevator never turns around: it drops off one edge of the
            // stage and reappears at the other. Its previous position is
            // carried across the seam too, so a wrap does not drag a rider
            // the whole height of the map with it - it just leaves them.
            plat.position.y += plat.velocity.y * seconds;
            const float loop = tileMap.pixelHeight() + tileMap.tileSize();
            if (plat.velocity.y < 0.f && plat.position.y < -tileMap.tileSize()) {
                plat.position.y += loop;
                plat.previousPosition.y += loop;
            } else if (plat.velocity.y > 0.f
                       && plat.position.y > tileMap.pixelHeight()) {
                plat.position.y -= loop;
                plat.previousPosition.y -= loop;
            }
            break;
        }

        case LiftMotion::Balance: {
            // Each pulley is stepped once, from whichever end comes first.
            if (plat.partner < 0 || static_cast<std::size_t>(plat.partner) < i) {
                break;
            }
            MovingPlatform& other = movingPlatforms[plat.partner];
            const bool ridingThis = isStandingOnPlatform(
                plat.previousPosition, plat.widthTiles);
            const bool ridingOther = isStandingOnPlatform(
                other.previousPosition, other.widthTiles);
            if (ridingThis == ridingOther) {
                break; // empty, or weighed down at both ends: the pair hangs still
            }

            MovingPlatform& sinking = ridingThis ? plat : other;
            MovingPlatform& rising = ridingThis ? other : plat;
            // The rope has a fixed length, so the rising end stops the pair
            // once it has been hauled all the way up to its wheel.
            const float step = std::min(kBalanceLiftSpeed * seconds,
                                        rising.position.y - rising.ropeTopY);
            if (step <= 0.f) {
                break;
            }
            sinking.position.y += step;
            rising.position.y -= step;
            break;
        }
        }
    }

    // Whoever is aboard travels with the lift instead of being slid out from
    // under - that is the only way across the pits in World 3-3.
    if (!m_player || death.active) {
        return;
    }
    for (const MovingPlatform& plat : movingPlatforms) {
        const sf::Vector2f delta = plat.position - plat.previousPosition;
        if (delta.x == 0.f && delta.y == 0.f) {
            continue;
        }
        if (!isStandingOnPlatform(plat.previousPosition, plat.widthTiles)) {
            continue;
        }
        auto& body = m_player->getPhysicsBody();
        body.setPosition(body.getPosition() + delta);
        break;
    }
}

void PlayState::drawMovingPlatforms(sf::RenderWindow& window) const {
    const bool world21 = Config::worldNumber(currentLevel) == 2
                      && Config::stageNumber(currentLevel) == 1;
    const bool world31 = Config::worldNumber(currentLevel) == 3
                      && Config::stageNumber(currentLevel) == 1;
    const bool world33 = Config::worldNumber(currentLevel) == 3
                      && Config::stageNumber(currentLevel) == 3;
    const bool castle = isCastleStage(currentLevel);
    const bool coinHeaven = world21 || world31;
    const char* textureKey = world31 ? "World31CoinHeavenLift"
                           : (world21 ? "CoinHeavenLift"
                                      : (world33 ? "World33Lift"
                                                 : (castle ? "CastleLift" : "MovingPlatform")));

    // The rope pays out and reels in as the pulley turns, so it is drawn to
    // whatever length the platform hangs at rather than baked into the map.
    sf::RectangleShape rope({kBalanceRopeWidth * Config::kZoom, 0.f});
    rope.setFillColor(sf::Color(252, 188, 176));
    for (const auto& plat : movingPlatforms) {
        if (plat.motion != LiftMotion::Balance || plat.position.y <= plat.ropeTopY) {
            continue;
        }
        const float centre = plat.position.x
                           + tileMap.tileSize() * plat.widthTiles * 0.5f;
        rope.setSize({kBalanceRopeWidth * Config::kZoom, plat.position.y - plat.ropeTopY});
        rope.setPosition({centre - Config::kZoom, plat.ropeTopY});
        window.draw(rope);
    }

    sf::Sprite sprite(assets.getTexture(textureKey));
    sprite.setScale({Config::kZoom, Config::kZoom});
    // World 2-4's looping elevators are a narrower girder than the lift in
    // its own bridge room, so they carry their own sprite.
    sf::Sprite elevator(assets.getTexture("CastleElevator"));
    elevator.setScale({Config::kZoom, Config::kZoom});
    for (const auto& plat : movingPlatforms) {
        sf::Vector2f drawPosition = plat.position;
        if (coinHeaven) {
            drawPosition.y += Config::kZoom;
        }
        sf::Sprite& art = plat.motion == LiftMotion::Elevator ? elevator : sprite;
        art.setPosition(drawPosition);
        window.draw(art);
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

void PlayState::spawnCastleHazards() {
    fireBars.clear();
    const auto& pivots = tileMap.fireBarSpawns();
    fireBars.reserve(pivots.size());
    for (std::size_t index = 0; index < pivots.size(); ++index) {
        const float phase = static_cast<float>(index % 4) * kTwoPi * 0.25f;
        const float direction = index % 2 == 0 ? 1.f : -1.f;
        fireBars.push_back({pivots[index], phase,
                            direction * kFireBarAngularSpeed});
    }

    // A Podoboo's marker sits in the sky right above the lava it lives in, so
    // its home is one row below the cell that placed it.
    podoboos.clear();
    const auto& lavaSpouts = tileMap.podobooSpawns();
    for (std::size_t index = 0; index < lavaSpouts.size(); ++index) {
        const sf::Vector2f home{lavaSpouts[index].x,
                                lavaSpouts[index].y + tileMap.tileSize()};
        // Stagger the pit's Podoboos so they never leap in lockstep.
        const float offset = kPodobooRestDuration
                           * static_cast<float>(index % 2) * 0.5f;
        podoboos.push_back({home, home.y, 0.f, offset, false});
    }

    castleBoss = CastleBossEntity{};
    castleBossFire.clear();
    if (tileMap.castleBossSpawns().empty()) {
        return;
    }

    const float tile = tileMap.tileSize();
    const sf::Vector2f spawn = tileMap.castleBossSpawns().front();
    castleBoss.available = true;
    castleBoss.alive = true;
    castleBoss.position = spawn;
    castleBoss.velocity = {-kCastleBossWalkSpeed, 0.f};
    castleBoss.leftLimit = spawn.x - tile * 7.f;
    castleBoss.rightLimit = spawn.x + tile * 4.f;
    castleBoss.groundY = spawn.y;
    castleBoss.jumpTimer = kCastleBossJumpInterval * 0.5f;
    castleBoss.fireTimer = kCastleBossFireInterval * 0.45f;
}

sf::FloatRect PlayState::podobooBounds(const PodobooEntity& fire) const {
    const float side = tileMap.tileSize();
    return sf::FloatRect({fire.home.x, fire.y}, {side, side});
}

sf::FloatRect PlayState::castleBossBounds() const {
    if (!castleBoss.available || !castleBoss.alive) {
        return sf::FloatRect();
    }
    const float side = tileMap.tileSize() * 2.f;
    return sf::FloatRect(castleBoss.position, {side, side});
}

bool PlayState::damagePlayerFromCastleHazard() {
    if (!m_player || death.active || m_godMode || starPowerRemaining > 0.f
        || damageProtectionRemaining > 0.f || invincibleTimer > 0.f) {
        return false;
    }

    if (m_player->removeLatestPower()) {
        syncAvatarPowerVisuals();
        damageProtectionRemaining = kDamageProtectionDuration;
        invincibleTimer = 1.5f;
        Systems::SoundController::getInstance().playSound(
            assets.getSoundBuffer("DowngradeSound"));
        return false;
    }

    handlePlayerDeath();
    return true;
}

void PlayState::updateCastleHazards(sf::Time dt) {
    if (!isCastleStage(currentLevel) || !m_player) {
        return;
    }

    const float seconds = dt.asSeconds();
    const float fireballSize = 8.f * Config::kZoom;
    const sf::FloatRect player = avatarBounds();

    for (FireBarEntity& bar : fireBars) {
        bar.angle = std::fmod(bar.angle + bar.angularSpeed * seconds + kTwoPi,
                              kTwoPi);
        for (int index = 1; index <= kFireBarBallCount; ++index) {
            const float distance = fireballSize * static_cast<float>(index);
            const sf::Vector2f centre(
                bar.pivot.x + std::cos(bar.angle) * distance,
                bar.pivot.y + std::sin(bar.angle) * distance);
            const sf::FloatRect ball(
                centre - sf::Vector2f{fireballSize * 0.5f, fireballSize * 0.5f},
                {fireballSize, fireballSize});
            if (ball.findIntersection(player).has_value()) {
                if (damagePlayerFromCastleHazard()) {
                    return;
                }
                break;
            }
        }
    }

    for (PodobooEntity& fire : podoboos) {
        if (fire.airborne) {
            fire.velocityY = std::min(kMaxFallSpeed,
                                      fire.velocityY + kGravity * seconds);
            fire.y += fire.velocityY * seconds;
            if (fire.y >= fire.home.y) {
                // Back under the surface: sink out of sight and wait again.
                fire.y = fire.home.y;
                fire.airborne = false;
                fire.restTimer = 0.f;
            }
        } else {
            fire.restTimer += seconds;
            if (fire.restTimer >= kPodobooRestDuration) {
                fire.restTimer = 0.f;
                fire.airborne = true;
                fire.velocityY = -kPodobooJumpSpeed;
            }
            continue;
        }
        if (podobooBounds(fire).findIntersection(player).has_value()
            && damagePlayerFromCastleHazard()) {
            return;
        }
    }

    if (!castleBoss.available) {
        castleBossFire.clear();
        return;
    }

    if (!castleBoss.alive) {
        castleBossFire.clear();
        if (castleBoss.revealedGoomba && castleBoss.revealTimer > 0.f) {
            castleBoss.revealTimer = std::max(
                0.f, castleBoss.revealTimer - seconds);
            castleBoss.revealVelocity.y = std::min(
                kMaxFallSpeed,
                castleBoss.revealVelocity.y + kGravity * seconds);
            castleBoss.revealPosition += castleBoss.revealVelocity * seconds;
        }
        return;
    }

    if (!castleBoss.active) {
        castleBoss.active = player.position.x
            >= castleBoss.leftLimit - Config::kViewWidth * 0.75f;
    }
    if (!castleBoss.active) {
        return;
    }

    castleBoss.animationElapsed += seconds;
    while (castleBoss.animationElapsed >= kCastleBossFrameDuration) {
        castleBoss.animationElapsed -= kCastleBossFrameDuration;
        castleBoss.frame = (castleBoss.frame + 1) % 4;
    }

    castleBoss.jumpTimer += seconds;
    castleBoss.fireTimer += seconds;
    castleBoss.velocity.y = std::min(
        kMaxFallSpeed, castleBoss.velocity.y + kGravity * seconds);
    castleBoss.position += castleBoss.velocity * seconds;

    if (castleBoss.position.x < castleBoss.leftLimit) {
        castleBoss.position.x = castleBoss.leftLimit;
        castleBoss.velocity.x = kCastleBossWalkSpeed;
    } else if (castleBoss.position.x > castleBoss.rightLimit) {
        castleBoss.position.x = castleBoss.rightLimit;
        castleBoss.velocity.x = -kCastleBossWalkSpeed;
    }

    if (castleBoss.position.y >= castleBoss.groundY) {
        castleBoss.position.y = castleBoss.groundY;
        castleBoss.velocity.y = 0.f;
        if (castleBoss.jumpTimer >= kCastleBossJumpInterval) {
            castleBoss.velocity.y = -kCastleBossJumpSpeed;
            castleBoss.jumpTimer = 0.f;
        }
    }

    if (castleBoss.fireTimer >= kCastleBossFireInterval) {
        castleBoss.fireTimer = 0.f;
        const sf::Vector2f start(
            castleBoss.position.x,
            castleBoss.position.y + tileMap.tileSize() * 0.75f);
        const float playerCentreY = player.position.y + player.size.y * 0.5f;
        const float verticalSpeed = std::clamp(
            (playerCentreY - start.y) * 0.6f, -90.f, 90.f);
        castleBossFire.push_back(
            {start, {-kCastleBossFireSpeed, verticalSpeed}});
    }

    for (CastleBossFire& fire : castleBossFire) {
        fire.position += fire.velocity * seconds;
        const sf::FloatRect bounds(
            fire.position, {fireballSize, fireballSize});
        if (bounds.findIntersection(player).has_value()) {
            if (damagePlayerFromCastleHazard()) {
                return;
            }
        }
    }

    const float cameraLeft = m_cameraSystem.getCenter().x - Config::kViewWidth;
    castleBossFire.erase(
        std::remove_if(castleBossFire.begin(), castleBossFire.end(),
            [cameraLeft](const CastleBossFire& fire) {
                return fire.position.x < cameraLeft;
            }),
        castleBossFire.end());

    if (castleBossBounds().findIntersection(player).has_value()) {
        if (starPowerRemaining > 0.f || m_destroyerMode) {
            castleBoss.alive = false;
            castleBossFire.clear();
            score += 5000;
            hud.setScore(score);
            Systems::SoundController::getInstance().playSound(
                assets.getSoundBuffer("StompSound"));
        } else {
            damagePlayerFromCastleHazard();
        }
    }
}

void PlayState::drawCastleHazards(sf::RenderWindow& window) const {
    if (!isCastleStage(currentLevel)) {
        return;
    }

    const float fireballSize = 8.f * Config::kZoom;
    sf::Sprite fireball(assets.getTexture("Bullet"));
    fireball.setScale({Config::kZoom, Config::kZoom});
    for (const FireBarEntity& bar : fireBars) {
        for (int index = 1; index <= kFireBarBallCount; ++index) {
            const float distance = fireballSize * static_cast<float>(index);
            const sf::Vector2f centre(
                bar.pivot.x + std::cos(bar.angle) * distance,
                bar.pivot.y + std::sin(bar.angle) * distance);
            fireball.setPosition(
                centre - sf::Vector2f{fireballSize * 0.5f, fireballSize * 0.5f});
            window.draw(fireball);
        }
    }

    for (const CastleBossFire& fire : castleBossFire) {
        fireball.setPosition(fire.position);
        window.draw(fireball);
    }

    // A falling Podoboo is the rising sprite turned upside down.
    sf::Sprite podoboo(assets.getTexture("CastlePodoboo"));
    for (const PodobooEntity& fire : podoboos) {
        if (!fire.airborne) {
            continue;
        }
        const bool falling = fire.velocityY > 0.f;
        podoboo.setScale({Config::kZoom, falling ? -Config::kZoom : Config::kZoom});
        podoboo.setPosition({fire.home.x,
                             falling ? fire.y + tileMap.tileSize() : fire.y});
        window.draw(podoboo);
    }

    if (castleBoss.available && castleBoss.alive) {
        sf::Sprite bowser(assets.getTexture("CastleBowser"));
        bowser.setTextureRect(sf::IntRect(
            {castleBoss.frame * 32, 0}, {32, 32}));
        bowser.setScale({Config::kZoom, Config::kZoom});
        bowser.setPosition(castleBoss.position);
        window.draw(bowser);
    } else if (castleBoss.available && castleBoss.revealedGoomba
               && castleBoss.revealTimer > 0.f) {
        // World 1-4's Bowser was a Little Goomba; World 2-4's was a green
        // Koopa Troopa, which stands a half-tile taller.
        const sf::Texture& art = assets.getTexture(
            Config::worldNumber(currentLevel) == 1 ? "CastleGoomba"
                                                   : "CastleKoopa");
        const int height = static_cast<int>(art.getSize().y);
        sf::Sprite unmasked(art);
        const int frame = static_cast<int>(castleBoss.revealTimer / 0.12f) % 2;
        unmasked.setTextureRect(sf::IntRect({frame * 16, 0}, {16, height}));
        unmasked.setScale({Config::kZoom, Config::kZoom});
        unmasked.setPosition(castleBoss.revealPosition);
        window.draw(unmasked);
    }
}

void PlayState::startCastleClearSequence() {
    if (castleClear.active || !m_player) {
        return;
    }

    castleClear = CastleClearSequence{};
    castleClear.active = true;

    // Locate the authored bridge rather than baking its coordinates into the
    // sequence. This keeps the collapse tied to the map's '=' tiles.
    for (int row = 0; row < static_cast<int>(mapParser.getHeight()); ++row) {
        for (int col = 0; col < static_cast<int>(mapParser.getWidth()); ++col) {
            if (tileMap.symbolAt(col, row) != '=') {
                continue;
            }
            if (castleClear.bridgeRow < 0) {
                castleClear.bridgeRow = row;
                castleClear.firstBridgeColumn = col;
            }
            if (row == castleClear.bridgeRow) {
                castleClear.firstBridgeColumn = std::min(
                    castleClear.firstBridgeColumn, col);
                castleClear.nextBridgeColumn = std::max(
                    castleClear.nextBridgeColumn, col);
            }
        }
    }

    heldKeys.clear();
    inputHandler.reset();
    auto& body = m_player->getPhysicsBody();
    body.setVelocity({0.f, 0.f});
    body.clearAcceleration();
    body.setGrounded(true);
    facingRight = true;
    animator.setFacingRight(true);
    animator.setAction(entity::PlayerAction::Idle);
    animator.setSpeedRatio(0.f);

    tileMap.hideDecoration('$');
    castleBossFire.clear();
    castleBoss.revealedGoomba = false;
    castleBoss.revealTimer = 0.f;
    if (castleBoss.alive) {
        castleBoss.velocity = {0.f, 0.f};
    }

    Systems::SoundController::getInstance().stopMusic();
    Systems::SoundController::getInstance().playSound(
        assets.getSoundBuffer("BrickCollision"));
    std::cout << "[Core Engine] Castle axe touched; collapsing the bridge.\n";
}

void PlayState::updateCastleClearSequence(sf::Time dt) {
    if (!castleClear.active || !m_player) {
        return;
    }

    const float seconds = dt.asSeconds();
    auto& body = m_player->getPhysicsBody();
    body.setVelocity({0.f, 0.f});
    body.clearAcceleration();
    body.setGrounded(true);
    avatar.setPosition(body.getPosition());
    animator.setAction(entity::PlayerAction::Idle);
    animator.setFacingRight(true);
    animator.setSpeedRatio(0.f);
    animator.update(dt);

    if (!castleClear.bridgeCollapsed) {
        castleClear.bridgeTimer += seconds;
        while (castleClear.bridgeTimer >= kCastleBridgeStepDuration
               && castleClear.nextBridgeColumn
                    >= castleClear.firstBridgeColumn) {
            castleClear.bridgeTimer -= kCastleBridgeStepDuration;
            if (tileMap.removeTile(castleClear.nextBridgeColumn,
                                   castleClear.bridgeRow)) {
                Systems::SoundController::getInstance().playSound(
                    assets.getSoundBuffer("BrickBreak"));
            }
            --castleClear.nextBridgeColumn;
        }

        if (castleClear.nextBridgeColumn < castleClear.firstBridgeColumn) {
            castleClear.bridgeCollapsed = true;
            castleClear.bossFalling = castleBoss.alive;
            castleBoss.velocity = {0.f, 0.f};
        }
        if (!castleClear.bridgeCollapsed) {
            return;
        }
    }

    if (castleClear.bossFalling) {
        castleBoss.animationElapsed += seconds;
        while (castleBoss.animationElapsed >= kCastleBossFrameDuration) {
            castleBoss.animationElapsed -= kCastleBossFrameDuration;
            castleBoss.frame = (castleBoss.frame + 1) % 4;
        }
        castleBoss.velocity.y = std::min(
            kMaxFallSpeed, castleBoss.velocity.y + kGravity * seconds);
        castleBoss.position += castleBoss.velocity * seconds;

        if (castleBoss.position.y > tileMap.pixelHeight()) {
            castleBoss.alive = false;
            castleClear.bossFalling = false;
        }
    }

    if (!castleClear.bossFalling && !castleClear.victoryPlayed) {
        castleClear.victoryPlayed = true;
        castleClear.finishTimer = 0.f;
        Systems::SoundController::getInstance().playSound(
            assets.getSoundBuffer("VictorySound"));
    }

    if (!castleClear.victoryPlayed) {
        return;
    }

    castleClear.finishTimer += seconds;
    if (castleClear.finishTimer < kCastleClearFinishDelay) {
        return;
    }

    transitionPending = true;
    std::cout << "[Core Engine] Castle bridge clear complete; opening results.\n";
    gsm.changeState(std::make_unique<LevelCompleteState>(
        gsm, assets, getSaveData()));
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
    const float playerCentreX = playerBounds.position.x + playerBounds.size.x * 0.5f;
    const bool inStrongCurrent = underwater
                              && isWorld22CurrentColumn(playerCentreX, tileMap.tileSize());
    const bool wasGrounded = body.isGrounded();
    m_player->setInput(playerInput);
    m_player->update(seconds);

    bool swimStroke = false;
    if (m_flyMode) {
        sf::Vector2f velocity = body.getVelocity();
        if (heldKeys.count(sf::Keyboard::Scancode::W) || heldKeys.count(sf::Keyboard::Scancode::Up)) {
            velocity.y = -300.f;
        } else if (heldKeys.count(sf::Keyboard::Scancode::S) || heldKeys.count(sf::Keyboard::Scancode::Down)) {
            velocity.y = 300.f;
        } else {
            velocity.y = 0.f;
        }
        body.setVelocity(velocity);
        body.addAcceleration({0.f, -kGravity});
        body.setGrounded(false);
    } else if (underwater) {
        sf::Vector2f velocity = body.getVelocity();
        swimStroke = playerInput.jumpHeld && !swimButtonHeld;
        if (swimStroke) {
            velocity.y = -kSwimStrokeSpeed;
            body.setGrounded(false);
        }
        body.setVelocity(velocity);
        body.addAcceleration({0.f, -(kGravity - kWaterGravity)});
        if (inStrongCurrent) {
            body.addAcceleration({0.f, kWorld22CurrentAcceleration});
        }
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
    const float swimFallLimit = inStrongCurrent
                              ? kWorld22CurrentMaxFallSpeed : kMaxSwimFallSpeed;
    if (underwater && body.getVelocity().y > swimFallLimit) {
        sf::Vector2f velocity = body.getVelocity();
        velocity.y = swimFallLimit;
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
            const bool oneUpBlock = blockSymbol == '1' || blockSymbol == '5';
            const bool vineBlock = blockSymbol == '^';
            const bool fixedPowerUpBlock = blockSymbol == '*'
                                        || blockSymbol == '!'
                                        || blockSymbol == '4';
            const bool world11 = Config::worldNumber(currentLevel) == 1
                              && Config::stageNumber(currentLevel) == 1;
            const bool world12 = Config::worldNumber(currentLevel) == 1
                              && Config::stageNumber(currentLevel) == 2;
            const bool world21 = Config::worldNumber(currentLevel) == 2
                              && Config::stageNumber(currentLevel) == 1;
            const bool castle = isCastleStage(currentLevel);
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
                } else if (fixedPowerUpBlock) {
                    // Authored power-up blocks are deterministic: small Mario
                    // gets a mushroom and Super Mario gets a flower.
                    if (m_player->isSuper()) {
                        spawnFireFlower(tile.position);
                    } else {
                        spawnMushroom(tile.position);
                    }
                } else if (world11 || world12 || world21 || castle) {
                    // These maps author every power-up explicitly; all
                    // remaining ?/hidden item blocks therefore contain coins.
                    spawnCoinPop(tile.position);
                    Core::EventSystem::getInstance().broadcast(
                        {Core::EventType::CoinCollected});
                } else {
                    BlockReward reward = takeNextItemBlockReward();
                    if (reward == BlockReward::Mushroom
                        || reward == BlockReward::FireFlower) {
                        // A power-up block adapts to Mario's current form just
                        // like SMB: small Mario gets a mushroom, while Super
                        // Mario gets a Fire Flower in the same block.
                        if (m_player->isSuper()) {
                            spawnFireFlower(tile.position);
                        } else {
                            spawnMushroom(tile.position);
                        }
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

                                auto usage = std::find_if(
                                    coinBlockUsages.begin(), coinBlockUsages.end(),
                                    [col, row](const CoinBlockUsage& value) {
                                        return value.col == col && value.row == row;
                                    });
                                int coinsReleased = 1;
                                if (usage == coinBlockUsages.end()) {
                                    coinBlockUsages.push_back({col, row, 1});
                                } else {
                                    coinsReleased = ++usage->coinsReleased;
                                }

                                if (coinsReleased >= kCoinBlockCapacity) {
                                    symbol = 'U';
                                    tileMap.changeType(col, row, TileType::UsedBlock);
                                }
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
                + tileMap.tileSize() * platform.widthTiles;
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

    if (tryLeaveCoinHeaven()) {
        updateAvatarAnimation(dt, false);
        return true;
    }

    // Fell in pit
    if (body.getPosition().y > tileMap.pixelHeight()) {
        std::cout << "[Core Engine] Avatar fell into a pit.\n";
        handlePlayerDeath();
        return false;
    }

    avatar.setPosition(body.getPosition());

    // Lava is not a platform: entering even its animated surface immediately
    // starts the castle death fall. God mode remains useful for map inspection.
    if (!m_godMode && tileMap.intersectsHazard(avatarBounds())) {
        std::cout << "[Core Engine] Avatar touched castle lava.\n";
        handlePlayerDeath();
        return false;
    }

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

        // Star power or Destroyer mode defeats enemies on contact
        if (starPowerRemaining > 0.f || m_destroyerMode || m_godMode) {
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
        if (!underwater && body.getVelocity().y > 0.f
            && (body.getAABB().bottom() <= ent->getPosition().y + 24.f)) {
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
        } else if (damageProtectionRemaining <= 0.f && invincibleTimer <= 0.f && !m_godMode) {
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
