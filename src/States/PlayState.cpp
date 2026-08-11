#include "States/PlayState.hpp"
#include "Core/Config.hpp"
#include "States/GameStateManager.hpp"
#include "States/IntroMenuState.hpp"
#include "States/GameOverState.hpp"
#include "States/VictoryState.hpp"
#include "States/PauseState.hpp"
#include "States/RespawnState.hpp"
#include "States/LevelCompleteState.hpp"
#include "Systems/ResourcePath.hpp"
#include "Core/EventSystem.hpp"
#include "Systems/SoundController.hpp"
#include <algorithm>
#include <cmath>
#include <iostream>

namespace {
    // Kinematics, in world pixels per second. Tuned against a 48px tile so the jump
    // clears roughly three and a half tiles, like Super Mario Bros.
    constexpr float kGravity = 2400.f;
    constexpr float kMaxWalkSpeed = 420.f;
    constexpr float kGroundFriction = 2000.f;
    constexpr float kJumpSpeed = 1000.f;
    constexpr float kMaxFallSpeed = 1400.f;

    /// Coin released by a question block: rises, falls back, then vanishes.
    constexpr float kCoinPopSpeed = -480.f;
    constexpr float kCoinPopGravity = 1400.f;
    constexpr float kCoinPopLifetime = 0.7f;
    constexpr float kVineGrowDuration = 2.25f;
    constexpr std::size_t kMushroomRewardDivisor = 4; ///< About 25% mushroom rewards.
    constexpr float kStarPowerDuration = 10.f;

    /// Goombas wake up when their spawn enters the camera, then walk until defeated.
    constexpr float kGoombaSpeed = 72.f;
    constexpr float kGoombaFrameDuration = 0.3f;
    constexpr float kBlueKoopaSpeed = 60.f;
    constexpr float kBlueKoopaFrameDuration = 0.2f;
    constexpr float kGreenKoopaSpeed = 60.f;
    constexpr float kParatroopaBounceSpeed = 600.f;
    constexpr float kShellSpeed = 360.f;
    constexpr float kPiranhaVisibleDuration = 1.f;
    constexpr float kPiranhaMoveDuration = 0.55f;
    constexpr float kPiranhaHiddenDuration = 1.f;
    constexpr float kTrampolineCompressDuration = 0.12f;
    constexpr float kTrampolineLaunchDuration = 0.18f;
    constexpr float kTrampolineLaunchSpeed = 1400.f;
    constexpr float kGoombaStompBounce = 550.f;
    constexpr float kDamageProtectionDuration = 0.75f;

    constexpr float kBulletSpeed = 500.f;
    constexpr float kBulletLifetime = 2.f;
    constexpr float kShootCooldown = 0.25f;
    constexpr float kBulletRechargeTime = 10.f;
    constexpr int kMaxBullets = 3;

    constexpr float kMovingPlatformSpeed = 90.f;
    constexpr float kMovingPlatformRangeTiles = 3.f;
    constexpr float kMovingPlatformWidthTiles = 3.f;

    /// How fast free-look scrolls the level, and the multiplier while Shift is held.
    constexpr float kFreeLookSpeed = 900.f;
    constexpr float kFreeLookBoost = 3.f;

    /// Every underground map reserves its final columns for an outdoor goal area.
    constexpr int kOutdoorGoalColumns = 41;

}

PlayState::PlayState(GameStateManager& gsm, Systems::AssetManager& assets, CharacterType character)
    : State(gsm, assets), selectedCharacter(character),
      camera(sf::FloatRect({0.f, 0.f}, {Config::kViewWidth, Config::kViewHeight})),
      m_physicsSystem(kGravity, kMaxFallSpeed),
      avatarSprite(assets.getTexture(character == CharacterType::Mario ? "MarioIdle" : "LuigiIdle")) {}

PlayState::PlayState(GameStateManager& gsm, Systems::AssetManager& assets, const SaveData& data)
    : State(gsm, assets), selectedCharacter(data.selectedCharacter),
      camera(sf::FloatRect({0.f, 0.f}, {Config::kViewWidth, Config::kViewHeight})),
      m_physicsSystem(kGravity, kMaxFallSpeed),
      avatarSprite(assets.getTexture(data.selectedCharacter == CharacterType::Mario ? "MarioIdle" : "LuigiIdle"))
{
    this->currentLevel = data.currentLevel;
    this->score = data.score;
    this->coins = data.coins;
    this->lives = data.lives;
}

PlayState::~PlayState() {
    Core::EventSystem::getInstance().clearAllListeners();
}

void PlayState::init() {
    Core::EventSystem::getInstance().clearAllListeners();

    std::string charName = (selectedCharacter == CharacterType::Mario) ? "Mario" : "Luigi";
    std::cout << "[Core Engine] PlayState Initialized with character: " << charName << "\n";
    
    hud.init(assets, selectedCharacter);
    hud.setScore(this->score);
    hud.setCoins(this->coins);
    hud.setLives(this->lives);
    hud.setWorld(this->currentLevel);
    hud.setAmmo(availableBullets, kMaxBullets);

    // Setup SoundController and Data event listeners
    auto& events = Core::EventSystem::getInstance();
    
    events.subscribe(Core::EventType::CoinCollected, [this](const Core::Event&) {
        auto& sounds = Systems::SoundController::getInstance();
        sounds.playSound(assets.getSoundBuffer("CoinSound"));
        this->coins += 1;
        this->score += 200;
        if (this->coins >= 100) {
            this->coins -= 100;
            Core::EventSystem::getInstance().broadcast({Core::EventType::OneMoreLife});
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

    // Artwork for every character the map file uses. 'H' and '1' are left out
    // on purpose: hidden blocks stay invisible until they are struck.
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

    // Scenery. One character places a whole object, which is why these go through
    // setDecorationTexture: they are several tiles big, never collide, and are
    // drawn behind the level so the cells they cover stay usable.
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
    tileMap.setDecorationTexture('F', assets.getTexture("Flagpole"));
    tileMap.setDecorationTexture('X', assets.getTexture("Castle"));
    tileMap.setDecorationTexture('W', assets.getTexture("WarpPipeForked"));

    if (!loadLevel(currentLevel)) {
        return;
    }

    playLevelMusic();

    // Placeholder avatar: slightly narrower than a tile so it slips into gaps cleanly.
    float tile = tileMap.tileSize();
    avatar.setSize({tile * 0.7f, tile * 0.95f});
    avatar.setFillColor(sf::Color::Transparent);
    avatarSprite.setTexture(assets.getTexture(selectedCharacter == CharacterType::Mario ? "MarioIdle" : "LuigiIdle"));
    respawnAvatar();
    updateCamera();

    std::cout << "[Core Engine] Controls: Left/Right (or A/D) to move, Space/Up/W to jump, left click to shoot, Esc to pause.\n";
    std::cout << "[Core Engine] Press F for free look: the camera detaches so you can scroll "
                 "through the level with A/D (hold Shift to go faster).\n";
}

void PlayState::handleInput(const sf::Event& event) {
    if (transitionPending) {
        return;
    }

    // A key held while the window loses focus never sends its release, so it would
    // stay down forever and the level would scroll on its own.
    if (event.is<sf::Event::FocusLost>()) {
        heldKeys.clear();
        inputHandler.reset();
        return;
    }

    if (const auto* keyReleased = event.getIf<sf::Event::KeyReleased>()) {
        heldKeys.erase(keyReleased->code);
        return;
    }

    if (const auto* mousePressed = event.getIf<sf::Event::MouseButtonPressed>()) {
        if (mousePressed->button == sf::Mouse::Button::Left) {
            spawnBullet();
        }
        return;
    }

    if (const auto* keyPressed = event.getIf<sf::Event::KeyPressed>()) {
        // Holding a key makes the system repeat KeyPressed; the toggles below must
        // only fire on the first one, otherwise resting on F would flicker the mode.
        bool repeat = heldKeys.count(keyPressed->code) > 0;
        heldKeys.insert(keyPressed->code);
        if (repeat) {
            return;
        }

        // Push interactive PauseState menu overlay on P or Escape
        if (keyPressed->code == sf::Keyboard::Key::P || keyPressed->code == sf::Keyboard::Key::Escape) {
            std::cout << "[Core Engine] Pause requested. Pushing PauseState...\n";
            gsm.pushState(std::make_unique<PauseState>(gsm, assets, *this));
            return;
        } else if (keyPressed->code == sf::Keyboard::Key::F) {
            freeLook = !freeLook;
            // Pick the scrolling up exactly where the camera already is, so the
            // picture does not jump when the mode changes.
            freeLookCentre = camera.getCenter();
            if (!freeLook) {
                maxCameraCenterX = std::max(maxCameraCenterX, camera.getCenter().x);
            }
            std::cout << "[Core Engine] Free look " << (freeLook ? "ON" : "OFF") << "\n";
        } else if (keyPressed->code == sf::Keyboard::Key::F5) {
            if (SaveManager::saveProgress("savegame.txt", getSaveData())) {
                std::cout << "[Core Engine] Quick Save successful (World "
                          << Config::worldNumber(currentLevel) << "-"
                          << Config::stageNumber(currentLevel) << ").\n";
            }
        } else if (keyPressed->code == sf::Keyboard::Key::F9) {
            if (quickLoad()) {
                std::cout << "[Core Engine] Quick Load successful (World "
                          << Config::worldNumber(currentLevel) << "-"
                          << Config::stageNumber(currentLevel) << ").\n";
            }
        }
    }
}

void PlayState::update(sf::Time dt) {
    if (isPaused || transitionPending) {
        return;
    }

    // Update the Input pressing from Command Pattern
    inputHandler.update(heldKeys);

    damageProtectionRemaining = std::max(
        0.f, damageProtectionRemaining - dt.asSeconds());
    starPowerRemaining = std::max(0.f, starPowerRemaining - dt.asSeconds());
    shootCooldownRemaining = std::max(0.f, shootCooldownRemaining - dt.asSeconds());
    for (float& timer : ammoRechargeTimers) {
        timer -= dt.asSeconds();
    }
    const auto expiredTimer = std::remove_if(
        ammoRechargeTimers.begin(), ammoRechargeTimers.end(),
        [this](float timer) {
            if (timer > 0.f) {
                return false;
            }
            availableBullets = std::min(kMaxBullets, availableBullets + 1);
            return true;
        });
    if (expiredTimer != ammoRechargeTimers.end()) {
        ammoRechargeTimers.erase(expiredTimer, ammoRechargeTimers.end());
        hud.setAmmo(availableBullets, kMaxBullets);
    }

    if (freeLook) {
        // The avatar is deliberately frozen: left it running it would walk off or
        // fall into a pit while the camera is somewhere else entirely.
        panCamera(dt);
        tileMap.update(dt);
        Systems::SoundController::getInstance().update();
        return;
    }

    updateMovingPlatforms(dt);
    if (!moveAvatar(dt)) {
        tileMap.update(dt);
        Systems::SoundController::getInstance().update();
        return;
    }
    updateTrampolines(dt);
    if (!updatePiranhas(dt)) {
        tileMap.update(dt);
        Systems::SoundController::getInstance().update();
        return;
    }
    if (tryEnterNextLevel()) {
        tileMap.update(dt);
        return;
    }
    updateBullets(dt);
    if (!updateWalkingEnemies(dt)) {
        tileMap.update(dt);
        Systems::SoundController::getInstance().update();
        return;
    }
    updateCamera();
    updateCoinPops(dt);
    updateMushrooms(dt);
    updateFireFlowers(dt);
    updateStars(dt);
    updateGrowingVines(dt);
    updateExplosions(dt);
    updateDeadEnemies(dt);
    updateBlocks(dt);
    if (invincibleTimer > 0.f) {
        invincibleTimer -= dt.asSeconds();
    }
    tileMap.update(dt); // keeps the question blocks blinking
    hud.update(dt);     // updates the HUD (timer, etc.)
    if (hud.getTime() <= 0.f) {
        std::cout << "[Core Engine] Time expired.\n";
        handlePlayerDeath();
    }
    Systems::SoundController::getInstance().update(); // Clean up finished sounds
}

void PlayState::pause() {
    heldKeys.clear();
    inputHandler.reset();
}

void PlayState::resume() {
    heldKeys.clear();
    inputHandler.reset();
}

sf::FloatRect PlayState::avatarBounds() const {
    const physics::AABB bounds = m_player->getPhysicsBody().getAABB();
    return sf::FloatRect(bounds.position, bounds.size);
}

void PlayState::syncAvatarPowerVisuals() {
    auto& body = m_player->getPhysicsBody();
    const sf::Vector2f position = body.getPosition();
    const sf::Vector2f colliderSize = body.getColliderSize();
    const float feetY = position.y + colliderSize.y;
    avatar.setSize(colliderSize);
    avatar.setPosition(position);

    // The available character art is shared between forms. Keep its existing
    // bottom-centred convention and scale it to the new collider immediately.
    const sf::FloatRect spriteBounds = avatarSprite.getLocalBounds();
    const float scale = spriteBounds.size.y > 0.f
        ? (colliderSize.y * 1.5f / spriteBounds.size.y)
        : 1.f;
    avatarSprite.setScale({facingRight ? scale : -scale, scale});
    avatarSprite.setPosition({position.x + colliderSize.x / 2.f, feetY});
}

void PlayState::playLevelMusic() {
    const std::string theme = Config::stageNumber(currentLevel) == 2
        ? "assets/audio/Theme2.mp3" : "assets/audio/Theme.mp3";
    Systems::SoundController::getInstance().playMusic(Systems::resourcePath(theme));
}

void PlayState::handlePlayerDeath() {
    Core::EventSystem::getInstance().broadcast({Core::EventType::PlayerDied});

    SaveData data = getSaveData();
    std::cout << "[Core Engine] Player died in World "
              << Config::worldNumber(currentLevel) << "-"
              << Config::stageNumber(currentLevel)
              << ". Transitioning to RespawnState. Lives remaining: " << lives << "\n";
    transitionPending = true;
    gsm.changeState(std::make_unique<RespawnState>(gsm, assets, data));
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
    coinPops.clear();
    mushrooms.clear();
    fireFlowers.clear();
    stars.clear();
    growingVines.clear();
    starPowerRemaining = 0.f;
    bullets.clear();
    availableBullets = kMaxBullets;
    shootCooldownRemaining = 0.f;
    ammoRechargeTimers.clear();
    hud.setAmmo(availableBullets, kMaxBullets);
    explosions.clear();
    walkingEnemies.clear();
    piranhas.clear();
    trampolines.clear();
    spawnWalkingEnemies();
    spawnPiranhas();
    spawnTrampolines();
    spawnMovingPlatforms();
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

bool PlayState::tryEnterNextLevel() {
    const sf::FloatRect player = avatarBounds();
    const bool reachedLevelExit = !Config::isLastStageOfWorld(currentLevel)
        && currentLevel < Config::kFinalLevel
        && tileMap.hasLevelExit()
        && player.findIntersection(tileMap.levelExitBounds()).has_value();
    // Most stages use a hidden W exit marker. World 2-1 follows the reference
    // map and ends directly at its flag, so a goal is also valid when no W exists.
    const bool reachedStageGoal = tileMap.hasGoal()
        && (Config::isLastStageOfWorld(currentLevel) || !tileMap.hasLevelExit())
        && player.findIntersection(tileMap.goalBounds()).has_value();

    if (!reachedLevelExit && !reachedStageGoal) {
        return false;
    }

    SaveData progress = getSaveData();

    std::cout << "[Core Engine] Level exit reached. Transitioning to LevelCompleteState...\n";
    transitionPending = true;
    gsm.changeState(std::make_unique<LevelCompleteState>(gsm, assets, progress));
    return true;
}

void PlayState::spawnCoinPop(sf::Vector2f blockPosition) {
    // The map guarantees an empty '.' cell above each question block. Start the
    // coin there instead of drawing it inside the '?' tile that released it.
    sf::Vector2f coinPosition(blockPosition.x, blockPosition.y - tileMap.tileSize());
    coinPops.push_back({coinPosition, kCoinPopSpeed, 0.f});
}

void PlayState::updateCoinPops(sf::Time dt) {
    float seconds = dt.asSeconds();
    for (CoinPop& coin : coinPops) {
        coin.elapsed += seconds;
        coin.position.y += coin.velocityY * seconds;
        coin.velocityY += kCoinPopGravity * seconds;
    }

    coinPops.erase(std::remove_if(coinPops.begin(), coinPops.end(), [](const CoinPop& coin) {
        return coin.elapsed >= kCoinPopLifetime;
    }), coinPops.end());
}

void PlayState::drawCoinPops(sf::RenderWindow& window) const {
    sf::Sprite coinSprite(assets.getTexture("Coin"));
    coinSprite.setScale({Config::kZoom, Config::kZoom});

    for (const CoinPop& coin : coinPops) {
        int frame = static_cast<int>(coin.elapsed / 0.08f) % 4;
        coinSprite.setTextureRect(sf::IntRect({frame * TileMap::kSourceTileSize, 0},
                                             {TileMap::kSourceTileSize,
                                              TileMap::kSourceTileSize}));
        coinSprite.setPosition(coin.position);
        window.draw(coinSprite);
    }
}

void PlayState::prepareItemBlockRewards() {
    std::size_t itemBlockCount = 0;
    for (const auto& row : mapParser.getGrid()) {
        itemBlockCount += static_cast<std::size_t>(std::count(row.begin(), row.end(), '?'));
        itemBlockCount += static_cast<std::size_t>(std::count(row.begin(), row.end(), 'H'));
    }

    blockRewards.clear();
    nextBlockReward = 0;

    if (currentLevel == 1) {
        // World 1-1 contains exactly two mushroom rewards. Their positions in the
        // reward sequence are randomized; every other item block gives a coin.
        const std::size_t mushroomTarget = std::min<std::size_t>(2, itemBlockCount);
        blockRewards.insert(blockRewards.end(), mushroomTarget, BlockReward::Mushroom);
        blockRewards.insert(blockRewards.end(),
                            itemBlockCount - mushroomTarget,
                            BlockReward::Coin);
        std::shuffle(blockRewards.begin(), blockRewards.end(), rewardRandom);
    } else if (itemBlockCount >= 4) {
        // Keep the original guarantee: the first four hits contain two of each.
        const BlockReward guaranteed[] = {
            BlockReward::Coin, BlockReward::Coin,
            BlockReward::Mushroom, BlockReward::Mushroom
        };
        blockRewards.insert(blockRewards.end(), std::begin(guaranteed), std::end(guaranteed));
        std::shuffle(blockRewards.begin(), blockRewards.end(), rewardRandom);

        // Across the whole level, mushrooms make up about 25% of the rewards.
        const std::size_t mushroomTarget = std::max<std::size_t>(
            2, itemBlockCount / kMushroomRewardDivisor);
        blockRewards.insert(blockRewards.end(), mushroomTarget - 2, BlockReward::Mushroom);
        blockRewards.insert(blockRewards.end(),
                            itemBlockCount - mushroomTarget - 2,
                            BlockReward::Coin);
        std::shuffle(blockRewards.begin() + 4, blockRewards.end(), rewardRandom);
    } else {
        // Small custom levels cannot guarantee two of each; still favor coins.
        const std::size_t mushroomTarget = itemBlockCount / kMushroomRewardDivisor;
        blockRewards.insert(blockRewards.end(), mushroomTarget, BlockReward::Mushroom);
        blockRewards.insert(blockRewards.end(),
                            itemBlockCount - mushroomTarget,
                            BlockReward::Coin);
        std::shuffle(blockRewards.begin(), blockRewards.end(), rewardRandom);
    }

    // Randomly convert some Mushrooms to FireFlowers (25% chance)
    for (auto& reward : blockRewards) {
        if (reward == BlockReward::Mushroom && rewardRandom() % 4 == 0) {
            reward = BlockReward::FireFlower;
        }
    }

    std::size_t mushroomCount = static_cast<std::size_t>(std::count(
        blockRewards.begin(), blockRewards.end(), BlockReward::Mushroom));
    std::size_t fireFlowerCount = static_cast<std::size_t>(std::count(
        blockRewards.begin(), blockRewards.end(), BlockReward::FireFlower));
    std::cout << "[Core Engine] Item-block rewards: "
              << (blockRewards.size() - mushroomCount - fireFlowerCount) << " coins, "
              << mushroomCount << " mushrooms, "
              << fireFlowerCount << " fire flowers";
    if (currentLevel == 1) {
        std::cout << "; World 1-1 has exactly two randomized mushrooms\n";
    } else {
        std::cout << "; mushroom rate is about 25% and the first four "
                     "guarantee two of each\n";
    }
}

PlayState::BlockReward PlayState::takeNextItemBlockReward() {
    if (nextBlockReward >= blockRewards.size()) {
        return BlockReward::Coin;
    }
    return blockRewards[nextBlockReward++];
}

void PlayState::spawnMushroom(sf::Vector2f blockPosition,
                              items::MushroomKind kind) {
    mushrooms.emplace_back(blockPosition, kind);
}

void PlayState::updateMushrooms(sf::Time dt) {
    const float seconds = dt.asSeconds();
    const sf::FloatRect player = avatarBounds();
    const float tileSize = tileMap.tileSize();

    for (auto it = mushrooms.begin(); it != mushrooms.end(); ) {
        std::vector<physics::AABB> solids;
        if (it->isCollectible()) {
            sf::FloatRect broadBounds = it->getBounds();
            broadBounds.position.x -= 16.f;
            broadBounds.position.y -= 16.f;
            broadBounds.size.x += 32.f;
            broadBounds.size.y += 32.f;
            solids = getSolidAABBsOverlapping(broadBounds);
        }

        it->update(seconds, tileSize, m_physicsSystem, solids);

        if (it->isCollectible()
            && player.findIntersection(it->getBounds()).has_value()) {
            if (it->getKind() == items::MushroomKind::OneUp) {
                Core::EventSystem::getInstance().broadcast({Core::EventType::OneMoreLife});
            } else {
                m_player->applyPower(entity::PowerType::Super);
                syncAvatarPowerVisuals();
                Core::EventSystem::getInstance().broadcast({Core::EventType::MushroomCollected});
            }
            it = mushrooms.erase(it);
        } else if (it->hasFallenOut(tileMap.pixelHeight())) {
            // fell into a pit
            it = mushrooms.erase(it);
        } else {
            ++it;
        }
    }
}

void PlayState::drawMushrooms(sf::RenderWindow& window) const {
    for (const items::Mushroom& mushroom : mushrooms) {
        const char* textureKey = mushroom.getKind() == items::MushroomKind::OneUp
            ? "OneUpMushroom"
            : "SuperMushroom";
        mushroom.render(window, assets.getTexture(textureKey), Config::kZoom);
    }
}

void PlayState::spawnFireFlower(sf::Vector2f blockPosition) {
    fireFlowers.emplace_back(blockPosition);
}

void PlayState::updateFireFlowers(sf::Time dt) {
    const float seconds = dt.asSeconds();
    const sf::FloatRect player = avatarBounds();
    const float tileSize = tileMap.tileSize();

    for (auto it = fireFlowers.begin(); it != fireFlowers.end(); ) {
        it->update(seconds, tileSize);

        if (it->isCollectible()
            && player.findIntersection(it->getBounds()).has_value()) {
            Systems::SoundController::getInstance().playSound(assets.getSoundBuffer("PowerUpSound"));
            score += 1000;
            hud.setScore(score);
            m_player->applyPower(entity::PowerType::Fire);
            it = fireFlowers.erase(it);
        } else {
            ++it;
        }
    }
}

void PlayState::drawFireFlowers(sf::RenderWindow& window) const {
    for (const items::FireFlower& flower : fireFlowers) {
        flower.render(window, assets.getTexture("FireFlower"), tileMap.tileSize());
    }
}

void PlayState::spawnStar(sf::Vector2f blockPosition) {
    stars.emplace_back(blockPosition);
}

void PlayState::updateStars(sf::Time dt) {
    const float seconds = dt.asSeconds();
    const sf::FloatRect player = avatarBounds();

    for (auto it = stars.begin(); it != stars.end();) {
        it->update(seconds, tileMap);

        if (it->isCollectible()
            && player.findIntersection(it->getBounds()).has_value()) {
            starPowerRemaining = kStarPowerDuration;
            score += 1000;
            hud.setScore(score);
            Systems::SoundController::getInstance().playSound(
                assets.getSoundBuffer("PowerUpSound"));
            it = stars.erase(it);
        } else if (it->hasFallenOut(tileMap.pixelHeight())) {
            it = stars.erase(it);
        } else {
            ++it;
        }
    }
}

void PlayState::drawStars(sf::RenderWindow& window) const {
    for (const items::Star& star : stars) {
        star.render(window, assets.getTexture("SuperStar"), Config::kZoom);
    }
}

bool PlayState::spawnGrowingVine(sf::Vector2f blockPosition) {
    const bool alreadyGrowing = std::any_of(
        growingVines.begin(), growingVines.end(),
        [blockPosition](const GrowingVineEntity& vine) {
            return vine.blockPosition == blockPosition;
        });
    if (alreadyGrowing) {
        return false;
    }

    growingVines.push_back({blockPosition, 0.f});
    Systems::SoundController::getInstance().playSound(
        assets.getSoundBuffer("PowerUpSound"));
    return true;
}

void PlayState::updateGrowingVines(sf::Time dt) {
    for (GrowingVineEntity& vine : growingVines) {
        vine.elapsed = std::min(vine.elapsed + dt.asSeconds(), kVineGrowDuration);
    }
}

void PlayState::drawGrowingVines(sf::RenderWindow& window) const {
    const sf::Texture& texture = assets.getTexture("VineTop");
    const sf::Vector2u textureSize = texture.getSize();
    if (textureSize.x == 0 || textureSize.y == 0) {
        return;
    }

    for (const GrowingVineEntity& vine : growingVines) {
        const float progress = std::clamp(vine.elapsed / kVineGrowDuration, 0.f, 1.f);
        const int visibleHeight = std::clamp(
            static_cast<int>(std::ceil(textureSize.y * progress)),
            0, static_cast<int>(textureSize.y));
        if (visibleHeight == 0) {
            continue;
        }

        sf::Sprite sprite(texture);
        sprite.setTextureRect(sf::IntRect(
            {0, 0}, {static_cast<int>(textureSize.x), visibleHeight}));
        sprite.setScale({Config::kZoom, Config::kZoom});
        const float width = textureSize.x * Config::kZoom;
        sprite.setPosition({
            vine.blockPosition.x + (tileMap.tileSize() - width) * 0.5f,
            vine.blockPosition.y - visibleHeight * Config::kZoom
        });
        window.draw(sprite);
    }
}

void PlayState::spawnBullet() {
    if (shootCooldownRemaining > 0.f || availableBullets <= 0
        || bullets.size() >= static_cast<std::size_t>(kMaxBullets)) {
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
    --availableBullets;
    ammoRechargeTimers.push_back(kBulletRechargeTime);
    shootCooldownRemaining = kShootCooldown;
    hud.setAmmo(availableBullets, kMaxBullets);
}

void PlayState::updateBullets(sf::Time dt) {
    const float cameraLeft = camera.getCenter().x - Config::kViewWidth / 2.f;
    const float cameraRight = camera.getCenter().x + Config::kViewWidth / 2.f;

    for (entity::Bullet& bullet : bullets) {
        bullet.update(dt);
        if (!bullet.isActive()) {
            continue;
        }

        const sf::FloatRect bulletBounds = bullet.bounds();
        bool collided = tileMap.intersectsSolid(bulletBounds)
                     || bullet.position().x + entity::Bullet::kSize < cameraLeft
                     || bullet.position().x > cameraRight;

        if (!collided) {
            for (WalkingEnemy& enemy : walkingEnemies) {
                if (!enemy.alive || !enemy.active) {
                    continue;
                }
                const bool isKoopa = enemy.kind != EnemyKind::Goomba;
                const float enemyHeight = enemy.state == EnemyState::Walking
                    ? (isKoopa ? tileMap.tileSize() * 1.5f : tileMap.tileSize())
                    : tileMap.tileSize() * 14.f / 16.f;
                const sf::FloatRect enemyBounds(
                    enemy.position, {tileMap.tileSize(), enemyHeight});
                if (!bulletBounds.findIntersection(enemyBounds).has_value()) {
                    continue;
                }
                enemy.alive = false;
                score += isKoopa ? 200 : 100;
                hud.setScore(score);
                Systems::SoundController::getInstance().playSound(
                    assets.getSoundBuffer("StompSound"));
                collided = true;
                break;
            }
        }

        if (!collided) {
            const float scale = tileMap.tileSize() / TileMap::kSourceTileSize;
            for (PiranhaEntity& plant : piranhas) {
                if (!plant.alive || plant.exposure < 0.65f) {
                    continue;
                }
                const sf::FloatRect plantBounds(
                    {plant.position.x + scale, plant.position.y + scale},
                    {16.f * scale, 23.f * scale});
                if (!bulletBounds.findIntersection(plantBounds).has_value()) {
                    continue;
                }
                plant.alive = false;
                score += 200;
                hud.setScore(score);
                Systems::SoundController::getInstance().playSound(
                    assets.getSoundBuffer("StompSound"));
                collided = true;
                break;
            }
        }

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
        if (it->elapsed >= 0.05f) { // 50ms per frame
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
    // Explosion is 567x440 -> 3 cols, 2 rows -> 189x220 per frame
    // We scale it down to match a tile size or so (e.g. 48x48)
    expSprite.setScale({32.f / 189.f, 32.f / 220.f}); // fit roughly 32x32

    for (const ExplosionEntity& exp : explosions) {
        int col = exp.currentFrame % 3;
        int row = exp.currentFrame / 3;
        expSprite.setTextureRect(sf::IntRect({col * 189, row * 220}, {189, 220}));
        
        expSprite.setPosition({exp.position.x - 8.f, exp.position.y - 8.f}); // center slightly
        window.draw(expSprite);
    }
}

void PlayState::spawnWalkingEnemies() {
    walkingEnemies.clear();
    walkingEnemies.reserve(tileMap.enemySpawns().size()
                           + tileMap.blueKoopaSpawns().size()
                           + tileMap.greenKoopaSpawns().size()
                           + tileMap.greenParatroopaSpawns().size());

    for (sf::Vector2f spawn : tileMap.enemySpawns()) {
        auto data = entity::EntityFactory::createEnemyData(entity::EnemyType::Goomba, spawn, kGoombaSpeed);
        walkingEnemies.push_back({EnemyKind::Goomba, data.position, data.velocity});
    }

    // A Koopa sprite is 24px tall while one map cell is 16px. Markers remain
    // bottom-aligned with Goomba markers, so lift the Koopa by half a tile.
    const float koopaHeightOffset = tileMap.tileSize() * 0.5f;
    for (sf::Vector2f spawn : tileMap.blueKoopaSpawns()) {
        spawn.y -= koopaHeightOffset;
        auto data = entity::EntityFactory::createEnemyData(entity::EnemyType::BlueKoopa, spawn, kBlueKoopaSpeed);
        walkingEnemies.push_back({EnemyKind::BlueKoopa, data.position, data.velocity});
    }
    for (sf::Vector2f spawn : tileMap.greenKoopaSpawns()) {
        spawn.y -= koopaHeightOffset;
        walkingEnemies.push_back(
            {EnemyKind::GreenKoopa, spawn, {-kGreenKoopaSpeed, 0.f}});
    }
    for (sf::Vector2f spawn : tileMap.greenParatroopaSpawns()) {
        spawn.y -= koopaHeightOffset;
        walkingEnemies.push_back(
            {EnemyKind::GreenParatroopa, spawn, {-kGreenKoopaSpeed, 0.f}});
    }
}

void PlayState::spawnPiranhas() {
    piranhas.clear();
    piranhas.reserve(tileMap.piranhaSpawns().size());
    const float scale = tileMap.tileSize() / TileMap::kSourceTileSize;

    for (const sf::Vector2f marker : tileMap.piranhaSpawns()) {
        const float pipeTopY = marker.y + tileMap.tileSize() * 2.f;
        const sf::Vector2f shownPosition(
            marker.x + 7.f * scale,
            pipeTopY - 23.f * scale);
        piranhas.push_back({shownPosition, pipeTopY});
    }
}

bool PlayState::updatePiranhas(sf::Time dt) {
    const float seconds = dt.asSeconds();
    const float scale = tileMap.tileSize() / TileMap::kSourceTileSize;
    const float cycleDuration = kPiranhaVisibleDuration
        + kPiranhaMoveDuration + kPiranhaHiddenDuration
        + kPiranhaMoveDuration;

    for (PiranhaEntity& plant : piranhas) {
        if (!plant.alive) {
            continue;
        }

        plant.elapsed = std::fmod(plant.elapsed + seconds, cycleDuration);
        if (plant.elapsed < kPiranhaVisibleDuration) {
            plant.exposure = 1.f;
        } else if (plant.elapsed < kPiranhaVisibleDuration + kPiranhaMoveDuration) {
            const float progress = (plant.elapsed - kPiranhaVisibleDuration)
                / kPiranhaMoveDuration;
            plant.exposure = 1.f - progress;
        } else if (plant.elapsed < kPiranhaVisibleDuration + kPiranhaMoveDuration
                                  + kPiranhaHiddenDuration) {
            plant.exposure = 0.f;
        } else {
            const float progress = (plant.elapsed - kPiranhaVisibleDuration
                - kPiranhaMoveDuration - kPiranhaHiddenDuration)
                / kPiranhaMoveDuration;
            plant.exposure = progress;
        }
        plant.position.y = plant.pipeTopY - plant.exposure * 23.f * scale;

        if (plant.exposure < 0.65f) {
            continue;
        }
        const sf::FloatRect bounds(
            {plant.position.x + scale, plant.position.y + scale},
            {16.f * scale, 23.f * scale});
        if (!avatarBounds().findIntersection(bounds).has_value()) {
            continue;
        }

        if (starPowerRemaining > 0.f) {
            plant.alive = false;
            score += 200;
            hud.setScore(score);
            Systems::SoundController::getInstance().playSound(
                assets.getSoundBuffer("StompSound"));
            continue;
        }
        if (damageProtectionRemaining > 0.f || invincibleTimer > 0.f) {
            continue;
        }
        const bool hadFirePower = m_player->hasFirePower();
        if (m_player->removeLatestPower()) {
            syncAvatarPowerVisuals();
            damageProtectionRemaining = kDamageProtectionDuration;
            if (hadFirePower && !m_player->hasFirePower()) {
                invincibleTimer = 1.5f;
                sf::Vector2f velocity = m_player->getPhysicsBody().getVelocity();
                velocity.y = -300.f;
                m_player->getPhysicsBody().setVelocity(velocity);
                Systems::SoundController::getInstance().playSound(
                    assets.getSoundBuffer("DowngradeSound"));
            }
            return true;
        }

        handlePlayerDeath();
        return false;
    }
    return true;
}

void PlayState::drawPiranhas(sf::RenderWindow& window) const {
    sf::Sprite sprite(assets.getTexture("PiranhaPlant"));
    sprite.setScale({Config::kZoom, Config::kZoom});
    for (const PiranhaEntity& plant : piranhas) {
        if (!plant.alive) {
            continue;
        }
        sprite.setPosition(plant.position);
        window.draw(sprite);
    }
}

void PlayState::spawnTrampolines() {
    trampolines.clear();
    trampolines.reserve(tileMap.trampolineSpawns().size());
    for (const sf::Vector2f marker : tileMap.trampolineSpawns()) {
        trampolines.push_back(
            {marker.x, marker.y + tileMap.tileSize() * 2.f});
    }
}

void PlayState::updateTrampolines(sf::Time dt) {
    const float seconds = dt.asSeconds();
    const float scale = tileMap.tileSize() / TileMap::kSourceTileSize;
    const float normalHeight = 31.f * scale;
    const float compressedHeight = 9.f * scale;
    auto& body = m_player->getPhysicsBody();

    for (TrampolineEntity& trampoline : trampolines) {
        if (trampoline.state == TrampolineState::Compressed) {
            trampoline.elapsed += seconds;
            if (trampoline.carryingPlayer) {
                sf::Vector2f position = body.getPosition();
                position.y = trampoline.bottomY - compressedHeight
                           - body.getColliderSize().y;
                body.setPosition(position);
                sf::Vector2f velocity = body.getVelocity();
                velocity.y = 0.f;
                body.setVelocity(velocity);
                avatar.setPosition(position);
            }
            if (trampoline.elapsed >= kTrampolineCompressDuration) {
                trampoline.state = TrampolineState::Launch;
                trampoline.elapsed = 0.f;
                trampoline.carryingPlayer = false;
                sf::Vector2f position = body.getPosition();
                position.y = trampoline.bottomY - normalHeight
                           - body.getColliderSize().y;
                body.setPosition(position);
                sf::Vector2f velocity = body.getVelocity();
                velocity.y = -kTrampolineLaunchSpeed;
                body.setVelocity(velocity);
                body.setGrounded(false);
                avatar.setPosition(position);
                Systems::SoundController::getInstance().playSound(
                    assets.getSoundBuffer("JumpSound"));
            }
            continue;
        }

        if (trampoline.state == TrampolineState::Launch) {
            trampoline.elapsed += seconds;
            if (trampoline.elapsed >= kTrampolineLaunchDuration) {
                trampoline.state = TrampolineState::Normal;
                trampoline.elapsed = 0.f;
            }
            continue;
        }

        const sf::FloatRect player = avatarBounds();
        const float trampolineTop = trampoline.bottomY - normalHeight;
        const bool horizontallyOverlapping =
            player.position.x + player.size.x > trampoline.x + 2.f
            && player.position.x < trampoline.x + tileMap.tileSize() - 2.f;
        const bool crossingTop = player.position.y < trampolineTop
            && player.position.y + player.size.y >= trampolineTop;
        if (body.getVelocity().y >= 0.f && horizontallyOverlapping && crossingTop) {
            trampoline.state = TrampolineState::Compressed;
            trampoline.elapsed = 0.f;
            trampoline.carryingPlayer = true;
            sf::Vector2f position = body.getPosition();
            position.y = trampoline.bottomY - compressedHeight
                       - body.getColliderSize().y;
            body.setPosition(position);
            sf::Vector2f velocity = body.getVelocity();
            velocity.y = 0.f;
            body.setVelocity(velocity);
            body.setGrounded(true);
            avatar.setPosition(position);
        }
    }
}

void PlayState::drawTrampolines(sf::RenderWindow& window) const {
    sf::Sprite normalSprite(assets.getTexture("TrampolineNormal"));
    sf::Sprite compressedSprite(assets.getTexture("TrampolineCompressed"));
    sf::Sprite launchSprite(assets.getTexture("TrampolineLaunch"));
    const float scale = tileMap.tileSize() / TileMap::kSourceTileSize;

    for (const TrampolineEntity& trampoline : trampolines) {
        sf::Sprite* sprite = &normalSprite;
        float sourceHeight = 31.f;
        if (trampoline.state == TrampolineState::Compressed) {
            sprite = &compressedSprite;
            sourceHeight = 9.f;
        } else if (trampoline.state == TrampolineState::Launch) {
            sprite = &launchSprite;
        }
        sprite->setScale({scale, scale});
        sprite->setPosition(
            {trampoline.x, trampoline.bottomY - sourceHeight * scale});
        window.draw(*sprite);
    }
}

void PlayState::spawnMovingPlatforms() {
    movingPlatforms.clear();
    movingPlatforms.reserve(tileMap.movingPlatformSpawns().size());

    bool moveRight = true;
    for (const sf::Vector2f spawn : tileMap.movingPlatformSpawns()) {
        const float speed = moveRight ? kMovingPlatformSpeed : -kMovingPlatformSpeed;
        movingPlatforms.push_back({spawn, spawn, spawn.x, speed});
        moveRight = !moveRight;
    }
}

void PlayState::updateMovingPlatforms(sf::Time dt) {
    const float seconds = dt.asSeconds();
    const float tileSize = tileMap.tileSize();
    const sf::Vector2f platformSize(
        tileSize * kMovingPlatformWidthTiles, tileSize * 0.5f);
    const float travel = tileSize * kMovingPlatformRangeTiles;
    auto& body = m_player->getPhysicsBody();

    for (MovingPlatform& platform : movingPlatforms) {
        platform.previousPosition = platform.position;

        const sf::FloatRect oldBounds(platform.previousPosition, platformSize);
        const sf::FloatRect player = avatarBounds();
        const float playerBottom = player.position.y + player.size.y;
        const float oldTop = oldBounds.position.y;
        const bool horizontallyOverlapping =
            player.position.x + player.size.x > oldBounds.position.x + 2.f
            && player.position.x < oldBounds.position.x + oldBounds.size.x - 2.f;
        const bool standingOnPlatform = horizontallyOverlapping
            && std::abs(playerBottom - oldTop) <= 4.f
            && body.getVelocity().y >= 0.f;

        platform.position.x += platform.velocityX * seconds;
        const float leftEnd = platform.originX - travel;
        const float rightEnd = platform.originX + travel;
        if (platform.position.x < leftEnd) {
            platform.position.x = leftEnd;
            platform.velocityX = std::abs(platform.velocityX);
        } else if (platform.position.x > rightEnd) {
            platform.position.x = rightEnd;
            platform.velocityX = -std::abs(platform.velocityX);
        }

        if (standingOnPlatform) {
            sf::Vector2f position = body.getPosition();
            position.x += platform.position.x - platform.previousPosition.x;
            body.setPosition(position);
            avatar.setPosition(position);
        }
    }
}

void PlayState::drawMovingPlatforms(sf::RenderWindow& window) const {
    const bool coinHeaven = Config::worldNumber(currentLevel) == 2
                         && Config::stageNumber(currentLevel) == 1;
    sf::Sprite platformSprite(assets.getTexture(
        coinHeaven ? "CoinHeavenLift" : "MovingPlatform"));
    platformSprite.setScale({Config::kZoom, Config::kZoom});
    for (const MovingPlatform& platform : movingPlatforms) {
        platformSprite.setPosition(platform.position);
        window.draw(platformSprite);
    }
}

bool PlayState::updateWalkingEnemies(sf::Time dt) {
    const float seconds = dt.asSeconds();
    const float tileSize = tileMap.tileSize();
    const float cameraLeft = camera.getCenter().x - Config::kViewWidth / 2.f - tileSize;
    const float cameraRight = camera.getCenter().x + Config::kViewWidth / 2.f + tileSize;
    bool playerHit = false;

    for (WalkingEnemy& enemy : walkingEnemies) {
        if (!enemy.alive) {
            continue;
        }

        const bool isGreenKoopa = enemy.kind == EnemyKind::GreenKoopa;
        const bool isParatroopa = enemy.kind == EnemyKind::GreenParatroopa;
        const bool isKoopa = enemy.kind != EnemyKind::Goomba;
        const bool isShell = enemy.state != EnemyState::Walking;
        const float shellHeight = tileSize * 14.f / 16.f;
        const float enemyHeight = isShell
            ? shellHeight : (isKoopa ? tileSize * 1.5f : tileSize);
        const sf::Vector2f size(tileSize, enemyHeight);
        const float baseWalkSpeed = enemy.kind == EnemyKind::BlueKoopa
            ? kBlueKoopaSpeed : (isKoopa ? kGreenKoopaSpeed : kGoombaSpeed);
        const float moveSpeed = enemy.state == EnemyState::ShellMoving
            ? kShellSpeed : baseWalkSpeed;
        const float frameDuration = isKoopa
            ? kBlueKoopaFrameDuration : kGoombaFrameDuration;

        if (!enemy.active) {
            const float centreX = enemy.position.x + size.x / 2.f;
            if (centreX < cameraLeft || centreX > cameraRight) {
                continue;
            }
            enemy.active = true;
        }

        enemy.animationElapsed += seconds;
        if (enemy.state == EnemyState::Walking
            && enemy.animationElapsed >= frameDuration) {
            enemy.animationElapsed -= frameDuration;
            enemy.animationFrame = (enemy.animationFrame + 1) % 2;
        }
        if (enemy.state == EnemyState::ShellIdle) {
            enemy.velocity.x = 0.f;
        }

        // Delegate enemy kinematics and collision to PhysicsSystem (Phase 3)
        sf::Vector2f initialVel = enemy.velocity;
        if (initialVel.x == 0.f && enemy.state == EnemyState::Walking) {
            initialVel.x = -moveSpeed;
        }

        physics::PhysicsBody enemyBody(enemy.position, size);
        enemyBody.setVelocity(initialVel);

        sf::FloatRect broadBounds(
            {enemy.position.x - tileSize, enemy.position.y - tileSize},
            {size.x + tileSize * 2.f, size.y + tileSize * 2.f});
        const std::vector<physics::AABB> solids =
            getSolidAABBsOverlapping(broadBounds);

        m_physicsSystem.update(enemyBody, solids, seconds);

        enemy.position = enemyBody.getPosition();
        enemy.velocity = enemyBody.getVelocity();

        if (enemyBody.hitWallLeft()) {
            enemy.velocity.x = moveSpeed;
        } else if (enemyBody.hitWallRight()) {
            enemy.velocity.x = -moveSpeed;
        }

        if (isParatroopa && enemy.state == EnemyState::Walking
            && enemyBody.isGrounded()) {
            enemy.velocity.y = -kParatroopaBounceSpeed;
        }

        if (enemy.position.x < 0.f) {
            enemy.position.x = 0.f;
            enemy.velocity.x = moveSpeed;
        } else if (enemy.position.x + size.x > tileMap.pixelWidth()) {
            enemy.position.x = tileMap.pixelWidth() - size.x;
            enemy.velocity.x = -moveSpeed;
        }

        if (enemy.position.y > tileMap.pixelHeight()) {
            enemy.alive = false;
            continue;
        }

        sf::FloatRect enemyBounds(enemy.position, size);
        if (enemy.state == EnemyState::ShellMoving) {
            for (WalkingEnemy& other : walkingEnemies) {
                if (&other == &enemy || !other.alive || !other.active) {
                    continue;
                }
                const bool otherKoopa = other.kind != EnemyKind::Goomba;
                const bool otherShell = other.state != EnemyState::Walking;
                const float otherHeight = otherShell
                    ? shellHeight : (otherKoopa ? tileSize * 1.5f : tileSize);
                const sf::FloatRect otherBounds(
                    other.position, {tileSize, otherHeight});
                if (!enemyBounds.findIntersection(otherBounds).has_value()) {
                    continue;
                }
                other.alive = false;
                deadEnemies.push_back(
                    {other.kind, other.position, {0.f, -250.f}, 0.f});
                score += otherKoopa ? 400 : 200;
                hud.setScore(score);
                Systems::SoundController::getInstance().playSound(
                    assets.getSoundBuffer("StompSound"));
            }
        }

        if (!avatarBounds().findIntersection(enemyBounds).has_value()) {
            continue;
        }

        if (starPowerRemaining > 0.f) {
            enemy.alive = false;
            deadEnemies.push_back(
                {enemy.kind, enemy.position, {0.f, -250.f}, 0.f});
            score += isKoopa ? 400 : 200;
            hud.setScore(score);
            Systems::SoundController::getInstance().playSound(
                assets.getSoundBuffer("StompSound"));
            continue;
        }

        auto& body = m_player->getPhysicsBody();
        const float avatarBottom = body.getAABB().bottom();
        const bool stomped = body.getVelocity().y > 0.f
                          && avatarBottom <= enemy.position.y + size.y * 0.55f;
        if (stomped) {
            sf::Vector2f velocity = body.getVelocity();
            velocity.y = -kGoombaStompBounce;
            body.setVelocity(velocity);
            body.setGrounded(false);
            if (isParatroopa && enemy.state == EnemyState::Walking) {
                enemy.kind = EnemyKind::GreenKoopa;
                enemy.velocity.x = std::copysign(kGreenKoopaSpeed,
                                                  enemy.velocity.x);
                score += 200;
            } else if (isGreenKoopa && enemy.state == EnemyState::Walking) {
                enemy.position.y += size.y - shellHeight;
                enemy.state = EnemyState::ShellIdle;
                enemy.velocity = {0.f, 0.f};
                enemy.animationFrame = 0;
                score += 200;
            } else if (enemy.state == EnemyState::ShellMoving) {
                enemy.state = EnemyState::ShellIdle;
                enemy.velocity.x = 0.f;
                score += 100;
            } else if (enemy.state == EnemyState::ShellIdle) {
                enemy.state = EnemyState::ShellMoving;
                enemy.velocity.x = facingRight ? kShellSpeed : -kShellSpeed;
                score += 400;
            } else {
                enemy.alive = false;
                score += isKoopa ? 200 : 100;
            }
            hud.setScore(score);
            Systems::SoundController::getInstance().playSound(assets.getSoundBuffer("StompSound"));
        } else if (enemy.state == EnemyState::ShellIdle) {
            const physics::AABB playerBounds = body.getAABB();
            const float playerCentre = playerBounds.left() + playerBounds.size.x / 2.f;
            const float shellCentre = enemy.position.x + size.x / 2.f;
            enemy.state = EnemyState::ShellMoving;
            enemy.velocity.x = playerCentre < shellCentre ? kShellSpeed : -kShellSpeed;
            damageProtectionRemaining = 0.25f;
        } else if (damageProtectionRemaining > 0.f) {
            continue;
        } else {
            if (invincibleTimer > 0.f) {
                // Still invincible from recent downgrade, ignore hit
                continue;
            }
            const bool hadFirePower = m_player->hasFirePower();
            if (m_player->removeLatestPower()) {
                syncAvatarPowerVisuals();
                damageProtectionRemaining = kDamageProtectionDuration;
                if (hadFirePower && !m_player->hasFirePower()) {
                    invincibleTimer = 1.5f; // 1.5 seconds of invincibility
                    sf::Vector2f velocity = body.getVelocity();
                    velocity.y = -300.f; // Knockback upward
                    body.setVelocity(velocity);
                    Systems::SoundController::getInstance().playSound(assets.getSoundBuffer("DowngradeSound"));
                }
                break;
            } else {
                playerHit = true;
                break;
            }
        }
    }

    if (playerHit) {
        handlePlayerDeath();
        return false;
    }

    walkingEnemies.erase(std::remove_if(
        walkingEnemies.begin(), walkingEnemies.end(), [](const WalkingEnemy& enemy) {
            return !enemy.alive;
        }), walkingEnemies.end());
    return true;
}

void PlayState::drawWalkingEnemies(sf::RenderWindow& window) const {
    const std::string goombaTextureKey = Config::stageNumber(currentLevel) == 2
        ? "GoombaUnderground" : "Goomba";
    sf::Sprite goombaSprite(assets.getTexture(goombaTextureKey));
    sf::Sprite blueKoopaSprite(assets.getTexture("BlueKoopaUnderground"));
    sf::Sprite greenKoopaSprite(assets.getTexture("GreenKoopa"));
    sf::Sprite paratroopaSprite(assets.getTexture("GreenParatroopa"));
    sf::Sprite shellSprite(assets.getTexture("GreenShell"));

    for (const WalkingEnemy& enemy : walkingEnemies) {
        const bool isKoopa = enemy.kind != EnemyKind::Goomba;
        const bool isShell = enemy.state != EnemyState::Walking;
        sf::Sprite* sprite = &goombaSprite;
        int sourceHeight = TileMap::kSourceTileSize;
        int frame = enemy.animationFrame;
        float verticalOffset = 0.f;

        if (isShell) {
            sprite = &shellSprite;
            sourceHeight = 14;
            frame = 0;
        } else if (enemy.kind == EnemyKind::BlueKoopa) {
            sprite = &blueKoopaSprite;
            sourceHeight = 24;
        } else if (enemy.kind == EnemyKind::GreenKoopa) {
            sprite = &greenKoopaSprite;
            sourceHeight = 23;
            frame = 0;
            verticalOffset = Config::kZoom; // bottom-align 23px art in a 24px body
        } else if (enemy.kind == EnemyKind::GreenParatroopa) {
            sprite = &paratroopaSprite;
            sourceHeight = 24;
        }

        sprite->setTextureRect(sf::IntRect(
            {frame * TileMap::kSourceTileSize, 0},
            {TileMap::kSourceTileSize, sourceHeight}));

        // The supplied Koopa faces left. Mirror it only after a wall sends it right.
        if (isKoopa && !isShell && enemy.velocity.x > 0.f) {
            sprite->setScale({-Config::kZoom, Config::kZoom});
            sprite->setPosition({enemy.position.x + tileMap.tileSize(),
                                 enemy.position.y + verticalOffset});
        } else {
            sprite->setScale({Config::kZoom, Config::kZoom});
            sprite->setPosition({enemy.position.x,
                                 enemy.position.y + verticalOffset});
        }
        window.draw(*sprite);
    }
}

std::vector<physics::AABB> PlayState::getSolidAABBsOverlapping(const sf::FloatRect& bounds) const {
    std::vector<physics::AABB> result;
    const auto tiles = tileMap.solidTilesOverlapping(bounds);
    result.reserve(tiles.size());
    for (const auto& tile : tiles) {
        result.emplace_back(sf::Vector2f(tile.position.x, tile.position.y), sf::Vector2f(tile.size.x, tile.size.y));
    }
    return result;
}

void PlayState::respawnAvatar() {
    // A newly created Player always starts with an empty power stack and the
    // original Small collider dimensions.
    avatar.setSize({tileMap.tileSize() * 0.7f, tileMap.tileSize() * 0.95f});
    sf::Vector2f spawn = tileMap.playerSpawn();
    // Sit the avatar on the bottom of its spawn tile rather than its top-left corner.
    const sf::Vector2f position{
        spawn.x, spawn.y + tileMap.tileSize() - avatar.getSize().y};
    maxCameraCenterX = position.x + avatar.getSize().x / 2.f;

    if (selectedCharacter == CharacterType::Luigi) {
        m_player = std::make_unique<entity::Luigi>(position, avatar.getSize());
    } else {
        m_player = std::make_unique<entity::Mario>(position, avatar.getSize());
    }
    syncAvatarPowerVisuals();
}

bool PlayState::moveAvatar(sf::Time dt) {
    float seconds = dt.asSeconds();

    const auto& playerInput = inputHandler.getPlayerInput();
    auto& body = m_player->getPhysicsBody();
    const bool wasGrounded = body.isGrounded();
    m_player->setInput(playerInput);
    m_player->update(seconds);

    const sf::Vector2f size = body.getColliderSize();
    const float previousBottom = body.getAABB().bottom();
    if (wasGrounded && body.getVelocity().y < 0.f) {
        Core::EventSystem::getInstance().broadcast({Core::EventType::PlayerJumped});
    }

    // Collect solid tile colliders in broadphase area
    sf::FloatRect broadBounds = avatarBounds();
    broadBounds.position.x -= 16.0f;
    broadBounds.position.y -= 16.0f;
    broadBounds.size.x += 32.0f;
    broadBounds.size.y += 32.0f;
    std::vector<physics::AABB> solids = getSolidAABBsOverlapping(broadBounds);

    // Delegate kinematics & collision resolution to PhysicsSystem
    m_physicsSystem.update(body, solids, seconds);

    // SMB 1985 Camera Lock: Player cannot walk back past the left edge of the screen
    float cameraLeftEdge = camera.getCenter().x - Config::kViewWidth / 2.f;
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
                            // Spawn debris
                            float tx = tile.position.x;
                            float ty = tile.position.y;
                            brickDebris.push_back({{tx, ty}, {-60.f, -200.f}, 0.f, 0});
                            brickDebris.push_back({{tx + 8.f, ty}, {60.f, -200.f}, 0.f, 0});
                            brickDebris.push_back({{tx, ty + 8.f}, {-40.f, -100.f}, 0.f, 0});
                            brickDebris.push_back({{tx + 8.f, ty + 8.f}, {40.f, -100.f}, 0.f, 0});
                            
                            Systems::SoundController::getInstance().playSound(assets.getSoundBuffer("BrickBreak"));
                            
                            // Kill enemies above
                            for (auto& enemy : walkingEnemies) {
                                if (enemy.alive) {
                                    float ew = tileMap.tileSize();
                                    float eh = tileMap.tileSize();
                                    float ex = enemy.position.x;
                                    float ey = enemy.position.y;
                                    
                                    // Check if enemy is standing on the block
                                    if (ey + eh >= ty - 2.0f && ey + eh <= ty + 2.0f && ex + ew > tx && ex < tx + tileMap.tileSize()) {
                                        enemy.alive = false;
                                        deadEnemies.push_back({enemy.kind, enemy.position, {0.f, -250.f}, 0.f});
                                    }
                                }
                            }
                        }
                    } else {
                        // Bounce brick
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
                            
                            // Kill enemies above
                            float tx = tile.position.x;
                            float ty = tile.position.y;
                            for (auto& enemy : walkingEnemies) {
                                if (enemy.alive) {
                                    float ew = tileMap.tileSize();
                                    float eh = tileMap.tileSize();
                                    float ex = enemy.position.x;
                                    float ey = enemy.position.y;
                                    
                                    // Check if enemy is standing on the block
                                    if (ey + eh >= ty - 2.0f && ey + eh <= ty + 2.0f && ex + ew > tx && ex < tx + tileMap.tileSize()) {
                                        enemy.alive = false;
                                        deadEnemies.push_back({enemy.kind, enemy.position, {0.f, -250.f}, 0.f});
                                    }
                                }
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

    // World 1-3 lifts are one-way platforms: Mario may jump through them from
    // below, then lands when his feet cross their top surface while falling.
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
                position = body.getPosition();
                position.y = platformTop - size.y;
                body.setPosition(position);
                sf::Vector2f velocity = body.getVelocity();
                velocity.y = 0.f;
                body.setVelocity(velocity);
                body.setGrounded(true);
                break;
            }
        }
    }

    const int collectedCoins = tileMap.collectCoinsOverlapping(avatarBounds());
    for (int i = 0; i < collectedCoins; ++i) {
        Core::EventSystem::getInstance().broadcast({Core::EventType::CoinCollected});
    }

    // Fell down one of the level's pits: lose a life or game over.
    if (body.getPosition().y > tileMap.pixelHeight()) {
        std::cout << "[Core Engine] Avatar fell into a pit.\n";
        handlePlayerDeath();
        return false;
    }

    avatar.setPosition(body.getPosition());

    // --- Update Avatar Movement Animation & Facing Direction ---
    if (body.getVelocity().x > 10.f) {
        facingRight = true;
    } else if (body.getVelocity().x < -10.f) {
        facingRight = false;
    }

    std::string prefix = (m_player->hasFirePower() ? "Fire" : "") + std::string(selectedCharacter == CharacterType::Mario ? "Mario" : "Luigi");

    if (!body.isGrounded()) {
        // Jumping state
        avatarSprite.setTexture(assets.getTexture(prefix + "Jump"));
    }
    else if (std::abs(body.getVelocity().x) > 10.f) {
        // Running state (cycle Idle -> Run1 -> Run2 -> Run1)
        runAnimTimer += dt.asSeconds();
        if (runAnimTimer >= 0.08f) {
            runAnimTimer = 0.0f;
            currentRunStep = (currentRunStep + 1) % 4;
            
            // Play walking sound on specific steps when on the ground
            if (currentRunStep == 1 || currentRunStep == 3) {
                Systems::SoundController::getInstance().playSound(assets.getSoundBuffer("WalkingSound"));
            }
        }
        const std::string runTexKeys[] = {prefix + "Idle", prefix + "Run1", prefix + "Run2", prefix + "Run1"};
        avatarSprite.setTexture(assets.getTexture(runTexKeys[currentRunStep]));
    }
    else {
        // Idle state
        runAnimTimer = 0.0f;
        currentRunStep = 0;
        avatarSprite.setTexture(assets.getTexture(prefix + "Idle"));
    }

    // Reset texture rect to full size of currently active texture
    sf::Vector2u texSize = avatarSprite.getTexture().getSize();
    avatarSprite.setTextureRect(sf::IntRect({0, 0}, {(int)texSize.x, (int)texSize.y}));

    sf::FloatRect sb = avatarSprite.getLocalBounds();
    // Align sprite origin at bottom-center so feet rest perfectly on top of ground tiles
    avatarSprite.setOrigin({sb.position.x + sb.size.x / 2.f, sb.position.y + sb.size.y});

    float targetHeight = avatar.getSize().y * 1.5f;
    float scaleY = (sb.size.y > 0.f) ? (targetHeight / sb.size.y) : 1.f;
    float scaleX = facingRight ? scaleY : -scaleY;

    avatarSprite.setScale({scaleX, scaleY});
    position = body.getPosition();
    avatarSprite.setPosition({position.x + size.x / 2.f, position.y + size.y});
    return true;
}

void PlayState::centreCamera(sf::Vector2f target) {
    // Never show anything past the edges of the level.
    float halfWidth = Config::kViewWidth / 2.f;
    float halfHeight = Config::kViewHeight / 2.f;
    float maxCenterX = std::max(halfWidth, tileMap.pixelWidth() - halfWidth);
    float maxCenterY = std::max(halfHeight, tileMap.pixelHeight() - halfHeight);

    camera.setCenter({std::clamp(target.x, halfWidth, maxCenterX),
                      std::clamp(target.y, halfHeight, maxCenterY)});
}

void PlayState::updateCamera() {
    const physics::AABB player = m_player->getPhysicsBody().getAABB();
    float playerCenterX = player.left() + player.size.x / 2.f;
    maxCameraCenterX = std::max(maxCameraCenterX, playerCenterX);
    centreCamera({maxCameraCenterX, player.top() + player.size.y / 2.f});
}

void PlayState::panCamera(sf::Time dt) {
    const auto& playerInput = inputHandler.getPlayerInput();
    float direction = playerInput.moveAxis;
    bool boost = heldKeys.count(sf::Keyboard::Key::LShift) > 0 || heldKeys.count(sf::Keyboard::Key::RShift) > 0;
    float speed = kFreeLookSpeed * (boost ? kFreeLookBoost : 1.f);

    freeLookCentre.x += direction * speed * dt.asSeconds();
    centreCamera(freeLookCentre);

    // Read the clamp back, otherwise holding a key at either end of the level
    // would build up an offset that has to be undone before scrolling resumes.
    freeLookCentre = camera.getCenter();
}

void PlayState::drawFreeLookHint(sf::RenderWindow& window) const {
    std::string label = "F = MAP VIEW";
    sf::Vector2f position(16.f, Config::kViewHeight - 34.f);
    unsigned size = 16;

    if (freeLook) {
        // Show which columns of the current level are on screen, so what is drawn
        // can be matched against its map file straight away.
        float leftEdge = camera.getCenter().x - Config::kViewWidth / 2.f;
        int firstColumn = static_cast<int>(leftEdge / Config::kTileSize);
        label = "MAP VIEW " + std::to_string(Config::worldNumber(currentLevel))
              + "-" + std::to_string(Config::stageNumber(currentLevel))
              + "   COL " + std::to_string(firstColumn) + "-"
              + std::to_string(firstColumn + Config::kViewTilesX - 1)
              + "   A/D SCROLL   SHIFT FASTER   F EXIT";
        position = {16.f, 12.f};
        size = 20;
    }

    sf::Text hint(assets.getFont("MarioFont"), label, size);
    hint.setPosition(position);
    hint.setFillColor(sf::Color::White);
    hint.setOutlineColor(sf::Color::Black);
    hint.setOutlineThickness(3.f);
    window.draw(hint);
}

void PlayState::render(sf::RenderWindow& window) {
    // Game::render hands us a view whose viewport letterboxes the picture; the
    // camera has to reuse that viewport, otherwise the level would be drawn over
    // the black bars.
    const sf::View screenView = window.getView();
    camera.setViewport(screenView.getViewport());

    // Each middle stage stays dark underground, then returns to the outdoor sky
    // for its adapted pipe, staircase, flag and castle goal area.
    sf::RectangleShape sky({Config::kViewWidth, Config::kViewHeight});
    const int outdoorStartColumn = std::max(
        0, static_cast<int>(mapParser.getWidth()) - kOutdoorGoalColumns);
    bool underground = Config::stageNumber(currentLevel) == 2
                    && m_player->getPhysicsBody().getPosition().x
                        < outdoorStartColumn * Config::kTileSize;
    sky.setFillColor(underground ? sf::Color(0, 0, 0) : sf::Color(92, 148, 252));
    window.draw(sky);

    window.setView(camera);
    drawPiranhas(window);
    window.draw(tileMap);
    drawGrowingVines(window);
    drawTrampolines(window);
    drawMovingPlatforms(window);
    drawCoinPops(window);
    drawMushrooms(window);
    drawFireFlowers(window);
    drawStars(window);
    drawBullets(window);
    drawWalkingEnemies(window);
    drawDeadEnemies(window);
    drawBlocks(window);
    window.draw(avatarSprite);
    drawExplosions(window);

    window.setView(screenView);
    drawFreeLookHint(window); // always on: it doubles as proof the build is current
    hud.render(window);
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

SaveData PlayState::getSaveData() const {
    SaveData data;
    data.currentLevel = this->currentLevel;
    data.score = this->score;
    data.coins = this->coins;
    data.lives = this->lives;
    data.selectedCharacter = this->selectedCharacter;
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
            avatarSprite.setTexture(assets.getTexture(
                selectedCharacter == CharacterType::Mario ? "MarioIdle" : "LuigiIdle"), true);
            freeLook = false;
            isPaused = false;
            heldKeys.clear();
            respawnAvatar();
            updateCamera();
            playLevelMusic();
            return true;
        }
    }
    return false;
}

void PlayState::updateDeadEnemies(sf::Time dt) {
    float seconds = dt.asSeconds();
    for (auto it = deadEnemies.begin(); it != deadEnemies.end(); ) {
        it->elapsed += seconds;
        it->velocity.y += kGravity * seconds;
        it->position += it->velocity * seconds;
        
        if (it->position.y > tileMap.pixelHeight() + 100.f) {
            it = deadEnemies.erase(it);
        } else {
            ++it;
        }
    }
}

void PlayState::drawDeadEnemies(sf::RenderWindow& window) const {
    const std::string goombaTextureKey = Config::stageNumber(currentLevel) == 2
        ? "GoombaUnderground" : "Goomba";
    sf::Sprite goombaSprite(assets.getTexture(goombaTextureKey));
    sf::Sprite blueKoopaSprite(assets.getTexture("BlueKoopaUnderground"));
    sf::Sprite greenKoopaSprite(assets.getTexture("GreenKoopa"));
    sf::Sprite paratroopaSprite(assets.getTexture("GreenParatroopa"));

    for (const DeadEnemy& enemy : deadEnemies) {
        sf::Sprite* sprite = &goombaSprite;
        int sourceHeight = TileMap::kSourceTileSize;
        if (enemy.kind == EnemyKind::BlueKoopa) {
            sprite = &blueKoopaSprite;
            sourceHeight = 24;
        } else if (enemy.kind == EnemyKind::GreenKoopa) {
            sprite = &greenKoopaSprite;
            sourceHeight = 23;
        } else if (enemy.kind == EnemyKind::GreenParatroopa) {
            sprite = &paratroopaSprite;
            sourceHeight = 24;
        }

        sprite->setTextureRect(sf::IntRect(
            {0, 0},
            {TileMap::kSourceTileSize, sourceHeight}));
        
        // Upside down
        sprite->setScale({Config::kZoom, -Config::kZoom});
        float scaledHeight = sourceHeight * Config::kZoom;
        sprite->setPosition({enemy.position.x, enemy.position.y + scaledHeight});
        
        window.draw(*sprite);
    }
}
