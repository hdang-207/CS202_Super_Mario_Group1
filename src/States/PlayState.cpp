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
#include <iostream>

namespace {
    // Kinematics, in world pixels per second. Tuned against a 48px tile so the jump
    // clears roughly three and a half tiles, like Super Mario Bros.
    constexpr float kGravity = 2400.f;
    constexpr float kWalkAcceleration = 1800.f;
    constexpr float kMaxWalkSpeed = 420.f;
    constexpr float kGroundFriction = 2000.f;
    constexpr float kJumpSpeed = 1000.f;
    constexpr float kJumpCutoff = 0.45f; ///< Releasing jump early shortens the hop.
    constexpr float kMaxFallSpeed = 1400.f;

    /// Coin released by a question block: rises, falls back, then vanishes.
    constexpr float kCoinPopSpeed = -480.f;
    constexpr float kCoinPopGravity = 1400.f;
    constexpr float kCoinPopLifetime = 0.7f;
    constexpr float kMushroomRiseDuration = 0.45f;
    constexpr std::size_t kMushroomRewardDivisor = 4; ///< About 25% mushroom rewards.

    /// Goombas wake up when their spawn enters the camera, then walk until defeated.
    constexpr float kGoombaSpeed = 72.f;
    constexpr float kGoombaFrameDuration = 0.3f;
    constexpr float kBlueKoopaSpeed = 60.f;
    constexpr float kBlueKoopaFrameDuration = 0.2f;
    constexpr float kGoombaStompBounce = 550.f;

    /// How fast free-look scrolls the level, and the multiplier while Shift is held.
    constexpr float kFreeLookSpeed = 900.f;
    constexpr float kFreeLookBoost = 3.f;

    /// World 1-2's original underground grid is 158 columns wide. The adapted
    /// outdoor goal area begins immediately after it.
    constexpr int kLevel2UndergroundColumns = 158;

}

bool PlayState::holding(sf::Keyboard::Key key) const {
    return heldKeys.count(key) > 0;
}

bool PlayState::wantsLeft() const {
    return holding(sf::Keyboard::Key::Left) || holding(sf::Keyboard::Key::A);
}

bool PlayState::wantsRight() const {
    return holding(sf::Keyboard::Key::Right) || holding(sf::Keyboard::Key::D);
}

bool PlayState::wantsJump() const {
    return holding(sf::Keyboard::Key::Space) || holding(sf::Keyboard::Key::Up)
        || holding(sf::Keyboard::Key::W);
}

bool PlayState::wantsBoost() const {
    return holding(sf::Keyboard::Key::LShift) || holding(sf::Keyboard::Key::RShift);
}

PlayState::PlayState(GameStateManager& gsm, Systems::AssetManager& assets, CharacterType character)
    : State(gsm, assets), selectedCharacter(character),
      camera(sf::FloatRect({0.f, 0.f}, {Config::kViewWidth, Config::kViewHeight})),
      avatarSprite(assets.getTexture(character == CharacterType::Mario ? "MarioIdle" : "LuigiIdle")) {}

PlayState::PlayState(GameStateManager& gsm, Systems::AssetManager& assets, const SaveData& data)
    : State(gsm, assets), selectedCharacter(data.selectedCharacter),
      camera(sf::FloatRect({0.f, 0.f}, {Config::kViewWidth, Config::kViewHeight})),
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

    // Artwork for every character the map file uses. 'H' is left out on purpose:
    // the hidden block has to stay invisible until it is struck.
    tileMap.setTileTexture('#', assets.getTexture("GroundTile"));
    tileMap.setTileTexture('B', assets.getTexture("BrickTile"));
    tileMap.setTileTexture('S', assets.getTexture("HardBlockTile"));
    tileMap.setTileTexture('[', assets.getTexture("PipeTopLeft"));
    tileMap.setTileTexture(']', assets.getTexture("PipeTopRight"));
    tileMap.setTileTexture('{', assets.getTexture("PipeBodyLeft"));
    tileMap.setTileTexture('}', assets.getTexture("PipeBodyRight"));
    tileMap.setTileTexture('?', assets.getTexture("QuestionBlock"), 4, sf::seconds(0.15f));
    tileMap.setTileTexture('U', assets.getTexture("EmptyBlock"));
    tileMap.setTileTexture('o', assets.getTexture("Coin"), 4, sf::seconds(0.12f));

    // Scenery. One character places a whole object, which is why these go through
    // setDecorationTexture: they are several tiles big, never collide, and are
    // drawn behind the level so the cells they cover stay usable.
    tileMap.setDecorationTexture('M', assets.getTexture("HillBig"));
    tileMap.setDecorationTexture('m', assets.getTexture("HillSmall"));
    tileMap.setDecorationTexture('V', assets.getTexture("BushBig"));
    tileMap.setDecorationTexture('v', assets.getTexture("BushSmall"));
    tileMap.setDecorationTexture('l', assets.getTexture("CloudBig"));
    tileMap.setDecorationTexture('c', assets.getTexture("CloudSmall"));
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

    std::cout << "[Core Engine] Controls: Left/Right (or A/D) to move, Space/Up/W to jump, Esc to quit.\n";
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
        return;
    }

    if (const auto* keyReleased = event.getIf<sf::Event::KeyReleased>()) {
        heldKeys.erase(keyReleased->code);
        return;
    }

    if (const auto* keyPressed = event.getIf<sf::Event::KeyPressed>()) {
        // Holding a key makes the system repeat KeyPressed; the toggles below must
        // only fire on the first one, otherwise resting on F would flicker the mode.
        bool repeat = holding(keyPressed->code);
        heldKeys.insert(keyPressed->code);
        if (repeat) {
            return;
        }

        // Push interactive PauseState menu overlay on P or Escape
        if (keyPressed->code == sf::Keyboard::Key::P || keyPressed->code == sf::Keyboard::Key::Escape) {
            std::cout << "[Core Engine] Pause requested. Pushing PauseState...\n";
            gsm.pushState(std::make_unique<PauseState>(gsm, assets, *this));
            return;
        } else if (keyPressed->code == sf::Keyboard::Key::LShift || keyPressed->code == sf::Keyboard::Key::RShift) {
            if (currentForm == AvatarForm::Fire) {
                spawnBullet();
            }
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
                std::cout << "[Core Engine] Quick Save successful (Level " << currentLevel << ").\n";
            }
        } else if (keyPressed->code == sf::Keyboard::Key::F9) {
            if (quickLoad()) {
                std::cout << "[Core Engine] Quick Load successful.\n";
            }
        }
    }
}

void PlayState::update(sf::Time dt) {
    if (isPaused || transitionPending) {
        return;
    }

    if (freeLook) {
        // The avatar is deliberately frozen: left it running it would walk off or
        // fall into a pit while the camera is somewhere else entirely.
        panCamera(dt);
        tileMap.update(dt);
        Systems::SoundController::getInstance().update();
        return;
    }

    if (!moveAvatar(dt)) {
        tileMap.update(dt);
        Systems::SoundController::getInstance().update();
        return;
    }
    if (tryEnterNextLevel()) {
        tileMap.update(dt);
        return;
    }
    if (!updateWalkingEnemies(dt)) {
        tileMap.update(dt);
        Systems::SoundController::getInstance().update();
        return;
    }
    updateCamera();
    updateCoinPops(dt);
    updateMushrooms(dt);
    updateFireFlowers(dt);
    updateBullets(dt);
    updateExplosions(dt);
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

sf::FloatRect PlayState::avatarBounds() const {
    return sf::FloatRect(avatarPos, avatar.getSize());
}

void PlayState::playLevelMusic() {
    const std::string theme = currentLevel == 2
        ? "assets/audio/Theme2.mp3" : "assets/audio/Theme.mp3";
    Systems::SoundController::getInstance().playMusic(Systems::resourcePath(theme));
}

void PlayState::handlePlayerDeath() {
    Core::EventSystem::getInstance().broadcast({Core::EventType::PlayerDied});

    lives--;
    SaveData data = getSaveData();
    std::cout << "[Core Engine] Player died. Transitioning to RespawnState. Lives remaining: " << lives << "\n";
    transitionPending = true;
    gsm.changeState(std::make_unique<RespawnState>(gsm, assets, data));
}

bool PlayState::loadLevel(int level) {
    if (level < 1 || level > Config::kFinalLevel) {
        std::cerr << "[Core Engine] Invalid level number: " << level << "\n";
        return false;
    }

    const std::string mapName = level == 2
        ? "assets/maps/level1-2.txt"
        : "assets/maps/level" + std::to_string(level) + ".txt";
    if (!mapParser.loadFromFile(Systems::resourcePath(mapName))) {
        std::cerr << "[Core Engine] Warning: Failed to load " << mapName << " map!\n";
        return false;
    }

    // Level 1-2 uses its own cyan underground artwork. Switching the registered
    // batches here keeps every outdoor tile in level 1 unchanged.
    tileMap.setTileTexture('#', assets.getTexture(
        level == 2 ? "GroundUndergroundTile" : "GroundTile"));
    tileMap.setTileTexture('B', assets.getTexture(
        level == 2 ? "BrickUndergroundTile" : "BrickTile"));
    tileMap.setTileTexture('S', assets.getTexture(
        level == 2 ? "HardBlockUndergroundTile" : "HardBlockTile"));
    tileMap.setTileTexture('?', assets.getTexture(
        level == 2 ? "QuestionBlockUnderground" : "QuestionBlock"),
        level == 2 ? 6 : 4, sf::seconds(0.15f));

    if (!tileMap.build(mapParser, Config::kZoom)) {
        std::cerr << "[Core Engine] Warning: Level " << level
                  << " map is empty, nothing to play!\n";
        return false;
    }

    currentLevel = level;
    hud.setWorld(currentLevel);
    hud.setTime(400.f);
    coinPops.clear();
    mushrooms.clear();
    fireFlowers.clear();
    bullets.clear();
    explosions.clear();
    walkingEnemies.clear();
    spawnWalkingEnemies();
    prepareQuestionBlockRewards();
    std::cout << "[Core Engine] Level " << currentLevel << " loaded: "
              << mapParser.getWidth() << "x" << mapParser.getHeight() << " tiles, "
              << tileMap.enemySpawns().size() << " Goombas, "
              << tileMap.blueKoopaSpawns().size() << " Blue Koopas\n";
    return true;
}

bool PlayState::tryEnterNextLevel() {
    const sf::FloatRect player = avatarBounds();
    const bool reachedLevelExit = currentLevel < Config::kFinalLevel
        && tileMap.hasLevelExit()
        && player.findIntersection(tileMap.levelExitBounds()).has_value();
    const bool reachedFinalGoal = currentLevel == Config::kFinalLevel
        && tileMap.hasGoal()
        && player.findIntersection(tileMap.goalBounds()).has_value();

    if (!reachedLevelExit && !reachedFinalGoal) {
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

void PlayState::prepareQuestionBlockRewards() {
    std::size_t questionBlockCount = 0;
    for (const auto& row : mapParser.getGrid()) {
        questionBlockCount += static_cast<std::size_t>(std::count(row.begin(), row.end(), '?'));
    }

    blockRewards.clear();
    nextBlockReward = 0;

    if (currentLevel == 1) {
        // Level 1 contains exactly two mushroom rewards. Their positions in the
        // reward sequence are randomized; every other question block gives a coin.
        const std::size_t mushroomTarget = std::min<std::size_t>(2, questionBlockCount);
        blockRewards.insert(blockRewards.end(), mushroomTarget, BlockReward::Mushroom);
        blockRewards.insert(blockRewards.end(),
                            questionBlockCount - mushroomTarget,
                            BlockReward::Coin);
        std::shuffle(blockRewards.begin(), blockRewards.end(), rewardRandom);
    } else if (questionBlockCount >= 4) {
        // Keep the original guarantee: the first four hits contain two of each.
        const BlockReward guaranteed[] = {
            BlockReward::Coin, BlockReward::Coin,
            BlockReward::Mushroom, BlockReward::Mushroom
        };
        blockRewards.insert(blockRewards.end(), std::begin(guaranteed), std::end(guaranteed));
        std::shuffle(blockRewards.begin(), blockRewards.end(), rewardRandom);

        // Across the whole level, mushrooms make up about 25% of the rewards.
        const std::size_t mushroomTarget = std::max<std::size_t>(
            2, questionBlockCount / kMushroomRewardDivisor);
        blockRewards.insert(blockRewards.end(), mushroomTarget - 2, BlockReward::Mushroom);
        blockRewards.insert(blockRewards.end(),
                            questionBlockCount - mushroomTarget - 2,
                            BlockReward::Coin);
        std::shuffle(blockRewards.begin() + 4, blockRewards.end(), rewardRandom);
    } else {
        // Small custom levels cannot guarantee two of each; still favor coins.
        const std::size_t mushroomTarget = questionBlockCount / kMushroomRewardDivisor;
        blockRewards.insert(blockRewards.end(), mushroomTarget, BlockReward::Mushroom);
        blockRewards.insert(blockRewards.end(),
                            questionBlockCount - mushroomTarget,
                            BlockReward::Coin);
        std::shuffle(blockRewards.begin(), blockRewards.end(), rewardRandom);
    }

    std::size_t mushroomCount = static_cast<std::size_t>(std::count(
        blockRewards.begin(), blockRewards.end(), BlockReward::Mushroom));
        
    // Randomly convert some Mushrooms to FireFlowers (25% chance)
    for (auto& reward : blockRewards) {
        if (reward == BlockReward::Mushroom && rewardRandom() % 4 == 0) {
            reward = BlockReward::FireFlower;
        }
    }
    std::cout << "[Core Engine] Question rewards: "
              << (blockRewards.size() - mushroomCount) << " coins, "
              << mushroomCount << " mushrooms";
    if (currentLevel == 1) {
        std::cout << "; Level 1 has exactly two randomized mushrooms\n";
    } else {
        std::cout << "; mushroom rate is about 25% and the first four "
                     "guarantee two of each\n";
    }
}

PlayState::BlockReward PlayState::takeNextQuestionBlockReward() {
    if (nextBlockReward >= blockRewards.size()) {
        return BlockReward::Coin;
    }
    return blockRewards[nextBlockReward++];
}

void PlayState::spawnMushroom(sf::Vector2f blockPosition) {
    mushrooms.push_back({blockPosition, blockPosition, {0.f, 0.f}, MushroomState::Emerging, 0.f});
}

void PlayState::updateMushrooms(sf::Time dt) {
    constexpr float kMushroomSpeed = 120.f;
    constexpr float kGravity = 2400.f;
    constexpr float kMaxFallSpeed = 600.f;
    float seconds = dt.asSeconds();
    
    sf::FloatRect ab = avatarBounds();

    for (auto it = mushrooms.begin(); it != mushrooms.end(); ) {
        if (it->state == MushroomState::Emerging) {
            it->elapsed = std::min(it->elapsed + seconds, kMushroomRiseDuration);
            float progress = it->elapsed / kMushroomRiseDuration;
            it->position = {it->blockPosition.x,
                            it->blockPosition.y - tileMap.tileSize() * progress};
            
            if (it->elapsed >= kMushroomRiseDuration) {
                it->state = MushroomState::Moving;
                it->velocity.x = kMushroomSpeed;
            }
        } else if (it->state == MushroomState::Moving) {
            it->velocity.y = std::min(it->velocity.y + kGravity * seconds, kMaxFallSpeed);
            
            sf::Vector2f size(tileMap.tileSize(), tileMap.tileSize());
            
            // X movement & collision
            it->position.x += it->velocity.x * seconds;
            sf::FloatRect xBounds(it->position, size);
            xBounds.position.y += 1.0f;
            xBounds.size.y -= 2.0f;
            
            for (const auto& tile : tileMap.solidTilesOverlapping(xBounds)) {
                if (it->velocity.x > 0.f) {
                    it->position.x = tile.position.x - size.x;
                } else if (it->velocity.x < 0.f) {
                    it->position.x = tile.position.x + tile.size.x;
                }
                it->velocity.x = -it->velocity.x; // bounce horizontally
                break;
            }
            
            // Y movement & collision
            it->position.y += it->velocity.y * seconds;
            sf::FloatRect yBounds(it->position, size);
            yBounds.position.x += 1.0f;
            yBounds.size.x -= 2.0f;
            
            for (const auto& tile : tileMap.solidTilesOverlapping(yBounds)) {
                if (it->velocity.y > 0.f) {
                    it->position.y = tile.position.y - size.y;
                } else if (it->velocity.y < 0.f) {
                    it->position.y = tile.position.y + tile.size.y;
                }
                it->velocity.y = 0.f;
            }
        }
        
        sf::FloatRect mushroomBounds(it->position, sf::Vector2f(tileMap.tileSize(), tileMap.tileSize()));
        if (it->state == MushroomState::Moving && ab.findIntersection(mushroomBounds)) {
            Core::EventSystem::getInstance().broadcast({Core::EventType::MushroomCollected});
            it = mushrooms.erase(it);
        } else if (it->position.y > tileMap.pixelHeight()) {
            // fell into a pit
            it = mushrooms.erase(it);
        } else {
            ++it;
        }
    }
}

void PlayState::drawMushrooms(sf::RenderWindow& window) const {
    sf::Sprite mushroomSprite(assets.getTexture("SuperMushroom"));
    mushroomSprite.setScale({Config::kZoom, Config::kZoom});

    for (const MushroomEntity& mushroom : mushrooms) {
        mushroomSprite.setPosition(mushroom.position);
        window.draw(mushroomSprite);
    }
}

void PlayState::spawnFireFlower(sf::Vector2f blockPosition) {
    fireFlowers.push_back({blockPosition, blockPosition, MushroomState::Emerging, 0.f});
}

void PlayState::updateFireFlowers(sf::Time dt) {
    float seconds = dt.asSeconds();
    sf::FloatRect ab = avatarBounds();
    float ts = tileMap.tileSize();
    // Flower drawn size (fit within a tile)
    float flowerSize = ts * 0.75f; // 75% of a tile

    for (auto it = fireFlowers.begin(); it != fireFlowers.end(); ) {
        if (it->state == MushroomState::Emerging) {
            it->elapsed = std::min(it->elapsed + seconds, 0.6f); // 0.6s rise
            float progress = it->elapsed / 0.6f;
            // Start at bottom of the block above, rise up one tile
            float centerX = it->blockPosition.x + (ts - flowerSize) / 2.f;
            it->position = {centerX,
                            it->blockPosition.y - flowerSize * progress};
            
            if (it->elapsed >= 0.6f) {
                it->state = MushroomState::Moving; // stays still
            }
        }
        
        sf::FloatRect bounds(it->position, sf::Vector2f(flowerSize, flowerSize));
        if (it->state == MushroomState::Moving && ab.findIntersection(bounds)) {
            Systems::SoundController::getInstance().playSound(assets.getSoundBuffer("PowerUpSound"));
            score += 1000;
            hud.setScore(score);
            currentForm = AvatarForm::Fire;
            it = fireFlowers.erase(it);
        } else {
            ++it;
        }
    }
}

void PlayState::drawFireFlowers(sf::RenderWindow& window) const {
    sf::Sprite flowerSprite(assets.getTexture("FireFlower"));
    // Scale flower to 75% of a tile size
    float ts = tileMap.tileSize();
    float flowerSize = ts * 0.75f;
    sf::Vector2u texSize = flowerSprite.getTexture().getSize();
    if (texSize.x > 0 && texSize.y > 0) {
        flowerSprite.setScale({flowerSize / static_cast<float>(texSize.x),
                               flowerSize / static_cast<float>(texSize.y)});
    }

    for (const FireFlowerEntity& flower : fireFlowers) {
        flowerSprite.setPosition(flower.position);
        window.draw(flowerSprite);
    }
}

void PlayState::spawnBullet() {
    if (bullets.size() >= 2) return; // Max 2 bullets
    
    Systems::SoundController::getInstance().playSound(assets.getSoundBuffer("FireSound"));

    BulletEntity b;
    // Position bullet at character's upper body / gun height (about 1/3 from top)
    float bulletX = facingRight ? avatarPos.x + avatar.getSize().x : avatarPos.x - 8.f;
    float bulletY = avatarPos.y + avatar.getSize().y * 0.3f;
    b.position = {bulletX, bulletY};
    b.facingRight = facingRight;
    b.velocity = {facingRight ? 500.f : -500.f, 0.f};
    bullets.push_back(b);
}

void PlayState::updateBullets(sf::Time dt) {
    float seconds = dt.asSeconds();
    constexpr float kBulletSize = 8.f;
    sf::Vector2f size(kBulletSize, kBulletSize);

    for (auto it = bullets.begin(); it != bullets.end(); ) {
        // Move straight horizontally (no gravity)
        it->position.x += it->velocity.x * seconds;

        sf::FloatRect bounds(it->position, size);

        // Check wall collision
        bool hitWall = false;
        for (const auto& tile : tileMap.solidTilesOverlapping(bounds)) {
            hitWall = true;
            break;
        }

        // Destroy if hit wall or went off-screen
        if (hitWall || it->position.x < camera.getCenter().x - Config::kViewWidth / 2.f ||
            it->position.x > camera.getCenter().x + Config::kViewWidth / 2.f) {
            spawnExplosion(it->position);
            it = bullets.erase(it);
            continue;
        }

        ++it;
    }
}

void PlayState::drawBullets(sf::RenderWindow& window) const {
    sf::Sprite bulletSprite(assets.getTexture("Bullet"));
    // Scale bullet to be small (~8px on screen)
    sf::Vector2u texSize = bulletSprite.getTexture().getSize();
    if (texSize.x > 0 && texSize.y > 0) {
        bulletSprite.setScale({8.f / static_cast<float>(texSize.x), 8.f / static_cast<float>(texSize.y)});
    }
    
    for (const BulletEntity& bullet : bullets) {
        bulletSprite.setPosition(bullet.position);
        window.draw(bulletSprite);
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
                           + tileMap.blueKoopaSpawns().size());

    for (sf::Vector2f spawn : tileMap.enemySpawns()) {
        walkingEnemies.push_back(
            {EnemyKind::Goomba, spawn, {-kGoombaSpeed, 0.f}});
    }

    // A Koopa sprite is 24px tall while one map cell is 16px. Markers remain
    // bottom-aligned with Goomba markers, so lift the Koopa by half a tile.
    const float koopaHeightOffset = tileMap.tileSize() * 0.5f;
    for (sf::Vector2f spawn : tileMap.blueKoopaSpawns()) {
        spawn.y -= koopaHeightOffset;
        walkingEnemies.push_back(
            {EnemyKind::BlueKoopa, spawn, {-kBlueKoopaSpeed, 0.f}});
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

        const bool isKoopa = enemy.kind == EnemyKind::BlueKoopa;
        const float walkSpeed = isKoopa ? kBlueKoopaSpeed : kGoombaSpeed;
        const float frameDuration = isKoopa
            ? kBlueKoopaFrameDuration : kGoombaFrameDuration;
        const sf::Vector2f size(tileSize, isKoopa ? tileSize * 1.5f : tileSize);

        if (!enemy.active) {
            const float centreX = enemy.position.x + size.x / 2.f;
            if (centreX < cameraLeft || centreX > cameraRight) {
                continue;
            }
            enemy.active = true;
        }

        enemy.animationElapsed += seconds;
        if (enemy.animationElapsed >= frameDuration) {
            enemy.animationElapsed -= frameDuration;
            enemy.animationFrame = (enemy.animationFrame + 1) % 2;
        }

        enemy.velocity.y = std::min(enemy.velocity.y + kGravity * seconds,
                                    kMaxFallSpeed);

        // Resolve horizontal movement first. Hitting a solid tile turns the
        // enemy around instead of stopping it permanently.
        enemy.position.x += enemy.velocity.x * seconds;
        sf::FloatRect xBounds(enemy.position, size);
        xBounds.position.y += 1.f;
        xBounds.size.y -= 2.f;

        for (const sf::FloatRect& tile : tileMap.solidTilesOverlapping(xBounds)) {
            if (enemy.velocity.x > 0.f) {
                enemy.position.x = tile.position.x - size.x;
                enemy.velocity.x = -walkSpeed;
            } else {
                enemy.position.x = tile.position.x + tile.size.x;
                enemy.velocity.x = walkSpeed;
            }
            break;
        }

        if (enemy.position.x < 0.f) {
            enemy.position.x = 0.f;
            enemy.velocity.x = walkSpeed;
        } else if (enemy.position.x + size.x > tileMap.pixelWidth()) {
            enemy.position.x = tileMap.pixelWidth() - size.x;
            enemy.velocity.x = -walkSpeed;
        }

        // Gravity and floor/platform collision use the same tile geometry as Mario.
        enemy.position.y += enemy.velocity.y * seconds;
        sf::FloatRect yBounds(enemy.position, size);
        yBounds.position.x += 1.f;
        yBounds.size.x -= 2.f;

        for (const sf::FloatRect& tile : tileMap.solidTilesOverlapping(yBounds)) {
            if (enemy.velocity.y > 0.f) {
                enemy.position.y = tile.position.y - size.y;
                enemy.velocity.y = 0.f;
            } else if (enemy.velocity.y < 0.f) {
                enemy.position.y = tile.position.y + tile.size.y;
                enemy.velocity.y = 0.f;
            }
            break;
        }

        if (enemy.position.y > tileMap.pixelHeight()) {
            enemy.alive = false;
            continue;
        }

        sf::FloatRect enemyBounds(enemy.position, size);
        if (!avatarBounds().findIntersection(enemyBounds).has_value()) {
            continue;
        }

        const float avatarBottom = avatarPos.y + avatar.getSize().y;
        const bool stomped = avatarVelocity.y > 0.f
                          && avatarBottom <= enemy.position.y + size.y * 0.55f;
        if (stomped) {
            enemy.alive = false;
            avatarVelocity.y = -kGoombaStompBounce;
            onGround = false;
            score += isKoopa ? 200 : 100;
            hud.setScore(score);
            Systems::SoundController::getInstance().playSound(assets.getSoundBuffer("StompSound"));
        } else {
            if (invincibleTimer > 0.f) {
                // Still invincible from recent downgrade, ignore hit
                continue;
            }
            if (currentForm == AvatarForm::Fire) {
                currentForm = AvatarForm::Normal;
                invincibleTimer = 1.5f; // 1.5 seconds of invincibility
                avatarVelocity.y = -300.f; // Knockback upward
                Systems::SoundController::getInstance().playSound(assets.getSoundBuffer("DowngradeSound"));
            } else {
                playerHit = true;
                break;
            }
        }
    }

    // Check Bullet collisions with enemies
    constexpr float kBulletSize = 8.f;
    float ts = tileMap.tileSize();
    for (auto& bullet : bullets) {
        sf::FloatRect bulletBounds(bullet.position, sf::Vector2f(kBulletSize, kBulletSize));
        for (auto& enemy : walkingEnemies) {
            if (!enemy.alive || !enemy.active) continue;
            float enemyH = (enemy.kind == EnemyKind::BlueKoopa) ? 24.f * Config::kZoom : ts;
            sf::FloatRect enemyBounds(enemy.position, sf::Vector2f(ts, enemyH));
            if (bulletBounds.findIntersection(enemyBounds)) {
                enemy.alive = false;
                score += 100;
                hud.setScore(score);
                Systems::SoundController::getInstance().playSound(assets.getSoundBuffer("StompSound"));
                spawnExplosion(bullet.position);
                bullet.position.y = 9999.f; // Mark for deletion
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
    const std::string goombaTextureKey = currentLevel == 2
        ? "GoombaUnderground" : "Goomba";
    sf::Sprite goombaSprite(assets.getTexture(goombaTextureKey));
    sf::Sprite koopaSprite(assets.getTexture("BlueKoopaUnderground"));

    for (const WalkingEnemy& enemy : walkingEnemies) {
        const bool isKoopa = enemy.kind == EnemyKind::BlueKoopa;
        sf::Sprite& sprite = isKoopa ? koopaSprite : goombaSprite;
        const int sourceHeight = isKoopa ? 24 : TileMap::kSourceTileSize;

        sprite.setTextureRect(sf::IntRect(
            {enemy.animationFrame * TileMap::kSourceTileSize, 0},
            {TileMap::kSourceTileSize, sourceHeight}));

        // The supplied Koopa faces left. Mirror it only after a wall sends it right.
        if (isKoopa && enemy.velocity.x > 0.f) {
            sprite.setScale({-Config::kZoom, Config::kZoom});
            sprite.setPosition({enemy.position.x + tileMap.tileSize(), enemy.position.y});
        } else {
            sprite.setScale({Config::kZoom, Config::kZoom});
            sprite.setPosition(enemy.position);
        }
        window.draw(sprite);
    }
}

void PlayState::respawnAvatar() {
    sf::Vector2f spawn = tileMap.playerSpawn();
    // Sit the avatar on the bottom of its spawn tile rather than its top-left corner.
    avatarPos = {spawn.x, spawn.y + tileMap.tileSize() - avatar.getSize().y};
    avatarVelocity = {0.f, 0.f};
    onGround = false;
    maxCameraCenterX = avatarPos.x + avatar.getSize().x / 2.f;
}

bool PlayState::moveAvatar(sf::Time dt) {
    float seconds = dt.asSeconds();

    // --- horizontal intent -------------------------------------------------
    float direction = (wantsRight() ? 1.f : 0.f) - (wantsLeft() ? 1.f : 0.f);
    if (direction != 0.f) {
        avatarVelocity.x += direction * kWalkAcceleration * seconds;
        avatarVelocity.x = std::clamp(avatarVelocity.x, -kMaxWalkSpeed, kMaxWalkSpeed);
    } else {

        // Friction logic: check if character is grounded
        // If grounded -> apply full ground friction
        // If airborne -> apply reduced air resistance to retain forward momentum
        float friction = onGround ? kGroundFriction : (kGroundFriction * 0.02f);
        // Coast to a stop instead of snapping, so movement keeps some inertia.
        float drop = friction * seconds;
        if (std::abs(avatarVelocity.x) <= drop) {
            avatarVelocity.x = 0.f;
        } else {
            avatarVelocity.x -= std::copysign(drop, avatarVelocity.x);
        }
    }

    // --- jumping -----------------------------------------------------------
    bool jumpPressed = wantsJump();
    if (jumpPressed && !jumpHeld && onGround) {
        avatarVelocity.y = -kJumpSpeed;
        onGround = false;
        Core::EventSystem::getInstance().broadcast({Core::EventType::PlayerJumped});
    }
    // Let go early and the jump is cut short - the classic variable jump height.
    if (!jumpPressed && avatarVelocity.y < -kJumpSpeed * kJumpCutoff) {
        avatarVelocity.y = -kJumpSpeed * kJumpCutoff;
    }
    jumpHeld = jumpPressed;

    avatarVelocity.y = std::min(avatarVelocity.y + kGravity * seconds, kMaxFallSpeed);

    // --- move and resolve, one axis at a time ------------------------------
    sf::Vector2f size = avatar.getSize();

    avatarPos.x += avatarVelocity.x * seconds;

    // Virtual X-axis hitbox: slightly narrow height to prevent snagging on floor/ceiling tiles
    sf::FloatRect xBounds = avatarBounds();
    xBounds.position.y += 1.0f;    // Shift top edge down by 1 pixel
    xBounds.size.y -= 2.0f;        // Shift bottom edge up by 1 pixel


    for (const sf::FloatRect& tile : tileMap.solidTilesOverlapping(xBounds)) {
        if (avatarVelocity.x > 0.f) {
            avatarPos.x = tile.position.x - size.x;
            avatarVelocity.x = 0.f;
        } else if (avatarVelocity.x < 0.f) {
            avatarPos.x = tile.position.x + tile.size.x;
            avatarVelocity.x = 0.f;
        }
    }

    // SMB 1985 Camera Lock: Player cannot walk back past the left edge of the screen
    float cameraLeftEdge = camera.getCenter().x - Config::kViewWidth / 2.f;
    avatarPos.x = std::clamp(avatarPos.x, cameraLeftEdge, std::max(cameraLeftEdge, tileMap.pixelWidth() - size.x));

    onGround = false;
    avatarPos.y += avatarVelocity.y * seconds;
    
    // Virtual Y-axis hitbox: slightly narrow width to prevent snagging on wall tiles
    sf::FloatRect yBounds = avatarBounds();
    yBounds.position.x += 1.0f;    // Shift left edge inward by 1 pixel
    yBounds.size.x -= 2.0f;        // Shift right edge inward by 1 pixel

    for (const sf::FloatRect& tile : tileMap.solidTilesOverlapping(yBounds)) {
        if (avatarVelocity.y > 0.f) {
            avatarPos.y = tile.position.y - size.y;
            avatarVelocity.y = 0.f;
            onGround = true;
        } else if (avatarVelocity.y < 0.f) {
            avatarPos.y = tile.position.y + tile.size.y;
            int col = static_cast<int>(tile.position.x / tileMap.tileSize());
            int row = static_cast<int>(tile.position.y / tileMap.tileSize());
            if (tileMap.activateQuestionBlock(col, row)) {
                BlockReward reward = takeNextQuestionBlockReward();
                if (reward == BlockReward::Mushroom) {
                    spawnMushroom(tile.position);
                } else if (reward == BlockReward::FireFlower) {
                    spawnFireFlower(tile.position);
                } else {
                    spawnCoinPop(tile.position);
                    Core::EventSystem::getInstance().broadcast({Core::EventType::CoinCollected});
                }
            }
            avatarVelocity.y = 0.f;
        }
    }

    const int collectedCoins = tileMap.collectCoinsOverlapping(avatarBounds());
    for (int i = 0; i < collectedCoins; ++i) {
        Core::EventSystem::getInstance().broadcast({Core::EventType::CoinCollected});
    }

    // Fell down one of the level's pits: lose a life or game over.
    if (avatarPos.y > tileMap.pixelHeight()) {
        std::cout << "[Core Engine] Avatar fell into a pit.\n";
        handlePlayerDeath();
        return false;
    }

    avatar.setPosition(avatarPos);

    // --- Update Avatar Movement Animation & Facing Direction ---
    if (avatarVelocity.x > 10.f) {
        facingRight = true;
    } else if (avatarVelocity.x < -10.f) {
        facingRight = false;
    }

    std::string prefix = (currentForm == AvatarForm::Fire ? "Fire" : "") + std::string(selectedCharacter == CharacterType::Mario ? "Mario" : "Luigi");

    if (!onGround) {
        // Jumping state
        avatarSprite.setTexture(assets.getTexture(prefix + "Jump"));
    }
    else if (std::abs(avatarVelocity.x) > 10.f) {
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
    avatarSprite.setPosition({avatarPos.x + avatar.getSize().x / 2.f, avatarPos.y + avatar.getSize().y});
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
    float playerCenterX = avatarPos.x + avatar.getSize().x / 2.f;
    maxCameraCenterX = std::max(maxCameraCenterX, playerCenterX);
    centreCamera({maxCameraCenterX, avatarPos.y + avatar.getSize().y / 2.f});
}

void PlayState::panCamera(sf::Time dt) {
    float direction = (wantsRight() ? 1.f : 0.f) - (wantsLeft() ? 1.f : 0.f);
    float speed = kFreeLookSpeed * (wantsBoost() ? kFreeLookBoost : 1.f);

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
        label = "MAP VIEW L" + std::to_string(currentLevel)
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

    // World 1-2 stays dark while underground, then returns to the outdoor sky
    // for the adapted pipe, staircase, flag and castle goal area.
    sf::RectangleShape sky({Config::kViewWidth, Config::kViewHeight});
    bool underground = currentLevel == 2
                    && avatarPos.x < kLevel2UndergroundColumns * Config::kTileSize;
    sky.setFillColor(underground ? sf::Color(0, 0, 0) : sf::Color(92, 148, 252));
    window.draw(sky);

    window.setView(camera);
    window.draw(tileMap);
    drawCoinPops(window);
    drawMushrooms(window);
    drawFireFlowers(window);
    drawBullets(window);
    drawWalkingEnemies(window);
    window.draw(avatarSprite);
    drawExplosions(window);

    window.setView(screenView);
    drawFreeLookHint(window); // always on: it doubles as proof the build is current
    hud.render(window);
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
            jumpHeld = false;
            respawnAvatar();
            updateCamera();
            playLevelMusic();
            return true;
        }
    }
    return false;
}
