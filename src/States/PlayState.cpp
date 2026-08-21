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

    constexpr float kVineGrowDuration = 2.25f;
    constexpr std::size_t kMushroomRewardDivisor = 4;
    constexpr float kStarPowerDuration = 10.f;

    constexpr float kTrampolineCompressDuration = 0.12f;
    constexpr float kTrampolineLaunchDuration = 0.18f;
    constexpr float kTrampolineLaunchSpeed = 1400.f;
    constexpr float kGoombaStompBounce = 550.f;
    constexpr float kDamageProtectionDuration = 0.75f;

    constexpr float kBombExplosionRadius = 72.f;

    constexpr float kMovingPlatformSpeed = 90.f;
    constexpr float kMovingPlatformRangeTiles = 3.f;
    constexpr float kMovingPlatformWidthTiles = 3.f;

    constexpr int kOutdoorGoalColumns = 41;
    constexpr int kWorld21BonusStartColumn = 293;
    constexpr int kWorld22EntrancePipeColumn = 15;
    constexpr int kWorld22WaterStartColumn = 37;
    constexpr int kWorld22WaterEndColumn = 198;
    constexpr int kWorld22WaterSurfaceRow = 2;
    constexpr int kWorld22WaterSpawnColumn = 39;
    constexpr int kWorld22WaterSpawnRow = 9;
}

PlayState::PlayState(GameStateManager& gsm, Systems::AssetManager& assets, CharacterType character)
    : State(gsm, assets), selectedCharacter(character),
      m_physicsSystem(kGravity, kMaxFallSpeed),
      avatarSprite(assets.getTexture(character == CharacterType::Mario ? "MarioIdle" : "LuigiIdle")) {}

PlayState::PlayState(GameStateManager& gsm, Systems::AssetManager& assets, const SaveData& data)
    : State(gsm, assets), selectedCharacter(data.selectedCharacter),
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

    // Setup SoundController and Data event listeners
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

    // Artwork setup
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
    avatarSprite.setTexture(assets.getTexture(selectedCharacter == CharacterType::Mario ? "MarioIdle" : "LuigiIdle"));
    respawnAvatar();
    m_cameraSystem.followTarget(m_player ? m_player->getPosition() : avatar.getPosition(), tileMap.pixelWidth(), tileMap.pixelHeight());

    std::cout << "[Core Engine] Controls: Left/Right (or A/D) to move, Space/Up/W to jump, X to shoot, C to throw a Fire bomb, Esc to pause.\n";
    std::cout << "[Core Engine] Press F for free look: the camera detaches so you can scroll through the level with A/D (hold Shift to go faster).\n";
}

void PlayState::handleInput(const sf::Event& event) {
    if (transitionPending) {
        return;
    }

    if (event.is<sf::Event::FocusLost>()) {
        heldKeys.clear();
        inputHandler.reset();
        return;
    }

    if (const auto* keyReleased = event.getIf<sf::Event::KeyReleased>()) {
        heldKeys.erase(keyReleased->code);
        return;
    }

    if (const auto* keyPressed = event.getIf<sf::Event::KeyPressed>()) {
        bool repeat = heldKeys.count(keyPressed->code) > 0;
        heldKeys.insert(keyPressed->code);
        if (repeat) {
            return;
        }

        if (keyPressed->code == sf::Keyboard::Key::X) {
            spawnBullet();
        } else if (keyPressed->code == sf::Keyboard::Key::C) {
            spawnBomb();
        } else if (keyPressed->code == sf::Keyboard::Key::P || keyPressed->code == sf::Keyboard::Key::Escape) {
            std::cout << "[Core Engine] Pause requested. Pushing PauseState...\n";
            gsm.pushState(std::make_unique<PauseState>(gsm, assets, *this));
            return;
        } else if (keyPressed->code == sf::Keyboard::Key::F) {
            m_cameraSystem.toggleFreeLook();
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

    inputHandler.update(heldKeys);

    damageProtectionRemaining = std::max(0.f, damageProtectionRemaining - dt.asSeconds());
    starPowerRemaining = std::max(0.f, starPowerRemaining - dt.asSeconds());

    if (m_cameraSystem.isFreeLook()) {
        const auto& playerInput = inputHandler.getPlayerInput();
        bool boost = heldKeys.count(sf::Keyboard::Key::LShift) > 0 || heldKeys.count(sf::Keyboard::Key::RShift) > 0;
        m_cameraSystem.pan(playerInput.moveAxis, boost, dt, tileMap.pixelWidth(), tileMap.pixelHeight());
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

    // Update all managed entities (Enemies, Items, CoinPops)
    m_entityManager.update(dt);

    m_cameraSystem.followTarget(m_player->getPhysicsBody().getPosition(), tileMap.pixelWidth(), tileMap.pixelHeight());

    updateGrowingVines(dt);
    updateBomb(dt);
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
                    && m_player->getPhysicsBody().getPosition().x < outdoorStartColumn * Config::kTileSize;
    const sf::Color outdoorSky = (world21 || world22) ? sf::Color(146, 144, 255) : sf::Color(92, 148, 252);
    sky.setFillColor(underground ? sf::Color(0, 0, 0) : outdoorSky);
    window.draw(sky);

    window.setView(m_cameraSystem.getView());
    if (world22) {
        const float waterLeft = kWorld22WaterStartColumn * tileMap.tileSize();
        const float waterRight = kWorld22WaterEndColumn * tileMap.tileSize();
        const float waterTop = kWorld22WaterSurfaceRow * tileMap.tileSize();
        const float waterHeight = tileMap.pixelHeight() - waterTop;
        const float bandHeight = waterHeight / 4.f;
        const sf::Color depthColours[] = {
            sf::Color(66, 64, 255),
            sf::Color(59, 68, 244),
            sf::Color(51, 73, 230),
            sf::Color(43, 79, 214)
        };

        for (int band = 0; band < 4; ++band) {
            const float top = waterTop + band * bandHeight;
            const float bottom = band == 3 ? tileMap.pixelHeight() : waterTop + (band + 1) * bandHeight;
            sf::RectangleShape waterBand({waterRight - waterLeft, bottom - top});
            waterBand.setPosition({waterLeft, top});
            waterBand.setFillColor(depthColours[band]);
            window.draw(waterBand);
        }

        sf::Sprite waterSurface(assets.getTexture("UnderwaterTiles"));
        waterSurface.setScale({Config::kZoom, Config::kZoom});
        for (int column = kWorld22WaterStartColumn; column < kWorld22WaterEndColumn; column += 4) {
            const int tileCount = std::min(4, kWorld22WaterEndColumn - column);
            waterSurface.setTextureRect(sf::IntRect({0, 0}, {tileCount * TileMap::kSourceTileSize, 32}));
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

    window.draw(tileMap);
    drawGrowingVines(window);
    drawTrampolines(window);
    drawMovingPlatforms(window);

    // Polymorphic rendering of all managed entities
    m_entityManager.render(window);

    drawBomb(window);
    drawBlocks(window);
    window.draw(avatarSprite);

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
            avatarSprite.setTexture(assets.getTexture(
                selectedCharacter == CharacterType::Mario ? "MarioIdle" : "LuigiIdle"), true);
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
    if (m_player->hasFirePower()) {
        avatarSprite.setTexture(assets.getTexture(selectedCharacter == CharacterType::Mario ? "MarioFire" : "LuigiFire"));
    } else if (m_player->isSuper()) {
        avatarSprite.setTexture(assets.getTexture(selectedCharacter == CharacterType::Mario ? "MarioSuper" : "LuigiSuper"));
    } else {
        avatarSprite.setTexture(assets.getTexture(selectedCharacter == CharacterType::Mario ? "MarioIdle" : "LuigiIdle"));
    }
}

void PlayState::handlePlayerDeath() {
    if (transitionPending) return;
    transitionPending = true;
    Core::EventSystem::getInstance().broadcast(Core::Event(Core::EventType::PlayerDied));
    if (lives > 0) {
        gsm.changeState(std::make_unique<RespawnState>(gsm, assets, getSaveData()));
    } else {
        gsm.changeState(std::make_unique<GameOverState>(gsm, assets));
    }
}

void PlayState::playLevelMusic() {
    auto& sounds = Systems::SoundController::getInstance();
    const bool world22 = Config::worldNumber(currentLevel) == 2 && Config::stageNumber(currentLevel) == 2;
    if (world22) {
        sounds.playMusic(Systems::resourcePath("assets/audio/UnderwaterTheme.mp3"));
    } else if (Config::stageNumber(currentLevel) == 2) {
        sounds.playMusic(Systems::resourcePath("assets/audio/UndergroundTheme.mp3"));
    } else {
        sounds.playMusic(Systems::resourcePath("assets/audio/Theme.mp3"));
    }
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
    syncAvatarPowerVisuals();
}

bool PlayState::loadLevel(int level) {
    std::string path = "assets/levels/level" + std::to_string(Config::worldNumber(level)) + "-" + std::to_string(Config::stageNumber(level)) + ".txt";
    if (!mapParser.loadFromFile(path)) {
        std::cerr << "[Core Engine] Failed to load level map: " << path << "\n";
        return false;
    }
    tileMap.build(mapParser, Config::kZoom);
    this->currentLevel = level;
    hud.setWorld(level);

    m_entityManager.clear();
    m_cameraSystem.reset();
    spawnWalkingEnemies();
    spawnPiranhas();
    spawnMovingPlatforms();
    spawnTrampolines();
    prepareItemBlockRewards();
    return true;
}

bool PlayState::tryEnterWorld22WaterPipe() {
    if (Config::worldNumber(currentLevel) != 2 || Config::stageNumber(currentLevel) != 2) {
        return false;
    }
    const float tile = tileMap.tileSize();
    const float pipeX = kWorld22EntrancePipeColumn * tile;
    auto& body = m_player->getPhysicsBody();
    if (body.getPosition().x < pipeX - tile || body.getPosition().x > pipeX + tile * 2.f) {
        return false;
    }
    if (!heldKeys.count(sf::Keyboard::Key::Down) && !heldKeys.count(sf::Keyboard::Key::S)) {
        return false;
    }

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
    auto& body = m_player->getPhysicsBody();
    if (body.getPosition().x >= tileMap.pixelWidth() - tileMap.tileSize() * 2.f) {
        if (transitionPending) return false;
        transitionPending = true;
        std::cout << "[Core Engine] Level Complete!\n";
        gsm.changeState(std::make_unique<LevelCompleteState>(gsm, assets, getSaveData()));
        return true;
    }
    return false;
}

void PlayState::spawnCoinPop(sf::Vector2f blockPosition) {
    m_entityManager.addEntity(entity::EntityFactory::createCoinPop(blockPosition, tileMap.tileSize()));
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
        const std::size_t mushroomTarget = std::min<std::size_t>(2, itemBlockCount);
        blockRewards.insert(blockRewards.end(), mushroomTarget, BlockReward::Mushroom);
        blockRewards.insert(blockRewards.end(), itemBlockCount - mushroomTarget, BlockReward::Coin);
        std::shuffle(blockRewards.begin(), blockRewards.end(), rewardRandom);
    } else {
        const std::size_t mushroomTarget = std::max<std::size_t>(2, itemBlockCount / kMushroomRewardDivisor);
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
    m_entityManager.addEntity(entity::EntityFactory::createMushroom(blockPosition, kind));
}

void PlayState::spawnFireFlower(sf::Vector2f blockPosition) {
    m_entityManager.addEntity(entity::EntityFactory::createFireFlower(blockPosition));
}

void PlayState::spawnStar(sf::Vector2f blockPosition) {
    m_entityManager.addEntity(entity::EntityFactory::createStar(blockPosition));
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
    // Projectile logic
    Systems::SoundController::getInstance().playSound(assets.getSoundBuffer("FireballSound"));
}

void PlayState::spawnBomb() {
    if (!activeBomb) {
        activeBomb.emplace(m_player->getPhysicsBody().getPosition(), facingRight);
        Systems::SoundController::getInstance().playSound(assets.getSoundBuffer("FireballSound"));
    }
}

void PlayState::updateBomb(sf::Time dt) {
    if (activeBomb) {
        activeBomb->update(dt.asSeconds());
        if (activeBomb->fuseExpired()) {
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
    for (auto& block : bouncingBlocks) {
        if (!block.active) continue;
        block.velocityY += kGravity * seconds;
        block.position.y += block.velocityY * seconds;
        if (block.position.y >= block.startY) {
            block.position.y = block.startY;
            block.active = false;
        }
    }
}

void PlayState::drawBlocks(sf::RenderWindow& window) const {
    for (const auto& block : bouncingBlocks) {
        if (block.active) {
            sf::Sprite sprite(assets.getTexture("QuestionBlock"));
            sprite.setPosition(block.position);
            window.draw(sprite);
        }
    }
}

void PlayState::spawnWalkingEnemies() {
    for (sf::Vector2f spawn : tileMap.enemySpawns()) {
        m_entityManager.addEntity(entity::EntityFactory::createGoomba(spawn, tileMap.tileSize()));
    }
    const float koopaHeightOffset = tileMap.tileSize() * 0.5f;
    for (sf::Vector2f spawn : tileMap.blueKoopaSpawns()) {
        spawn.y -= koopaHeightOffset;
        m_entityManager.addEntity(entity::EntityFactory::createKoopa(spawn, tileMap.tileSize(), entity::KoopaKind::BlueUnderground));
    }
    for (sf::Vector2f spawn : tileMap.greenKoopaSpawns()) {
        spawn.y -= koopaHeightOffset;
        m_entityManager.addEntity(entity::EntityFactory::createKoopa(spawn, tileMap.tileSize(), entity::KoopaKind::Green));
    }
    for (sf::Vector2f spawn : tileMap.greenParatroopaSpawns()) {
        spawn.y -= koopaHeightOffset;
        m_entityManager.addEntity(entity::EntityFactory::createParatroopa(spawn, tileMap.tileSize()));
    }
}

void PlayState::spawnPiranhas() {
    const float scale = tileMap.tileSize() / TileMap::kSourceTileSize;
    for (const sf::Vector2f marker : tileMap.piranhaSpawns()) {
        const float pipeTopY = marker.y + tileMap.tileSize() * 2.f;
        const sf::Vector2f shownPosition(marker.x + 7.f * scale, pipeTopY - 23.f * scale);
        m_entityManager.addEntity(entity::EntityFactory::createPiranhaPlant(shownPosition, pipeTopY, scale));
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
    sf::Sprite sprite(assets.getTexture("HardBlockTile"));
    for (const auto& plat : movingPlatforms) {
        sprite.setPosition(plat.position);
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
    sf::Sprite sprite(assets.getTexture("HardBlockTile"));
    for (const auto& tramp : trampolines) {
        sprite.setPosition({tramp.x, tramp.bottomY - tileMap.tileSize()});
        window.draw(sprite);
    }
}

bool PlayState::moveAvatar(sf::Time dt) {
    float seconds = dt.asSeconds();
    auto& body = m_player->getPhysicsBody();
    auto& input = inputHandler.getPlayerInput();

    m_player->setInput(input);
    m_player->update(seconds);

    sf::FloatRect broadBounds = avatarBounds();
    broadBounds.position.x -= 32.f;
    broadBounds.position.y -= 32.f;
    broadBounds.size.x += 64.f;
    broadBounds.size.y += 64.f;

    std::vector<physics::AABB> solids = getSolidAABBsOverlapping(broadBounds);
    m_physicsSystem.update(body, solids, seconds);

    if (body.getPosition().y > tileMap.pixelHeight() + 100.f) {
        handlePlayerDeath();
        return false;
    }

    if (input.moveAxis > 0.f) facingRight = true;
    else if (input.moveAxis < 0.f) facingRight = false;

    float scaleY = Config::kZoom;
    float scaleX = facingRight ? scaleY : -scaleY;
    avatarSprite.setScale({scaleX, scaleY});
    avatarSprite.setPosition({body.getPosition().x + avatar.getSize().x / 2.f, body.getPosition().y + avatar.getSize().y});

    // Check entity interactions (quái vật & items)
    auto overlappingEntities = m_entityManager.queryOverlapping(avatarBounds());
    for (auto* ent : overlappingEntities) {
        if (!ent || !ent->isAlive()) continue;

        if (starPowerRemaining > 0.f) {
            ent->setAlive(false);
            score += 200;
            hud.setScore(score);
            Systems::SoundController::getInstance().playSound(assets.getSoundBuffer("StompSound"));
            continue;
        }

        // Stomp check
        if (body.getVelocity().y > 0.f && body.getAABB().bottom() <= ent->getPosition().y + 24.f) {
            ent->setAlive(false);
            sf::Vector2f vel = body.getVelocity();
            vel.y = -kGoombaStompBounce;
            body.setVelocity(vel);
            body.setGrounded(false);
            score += 100;
            hud.setScore(score);
            Systems::SoundController::getInstance().playSound(assets.getSoundBuffer("StompSound"));
        } else if (damageProtectionRemaining <= 0.f && invincibleTimer <= 0.f) {
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

    return true;
}
