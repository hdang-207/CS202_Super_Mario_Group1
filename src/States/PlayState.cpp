#include "States/PlayState.hpp"
#include "Core/Config.hpp"
#include "States/GameStateManager.hpp"
#include "States/IntroMenuState.hpp"
#include "Systems/ResourcePath.hpp"
#include <algorithm>
#include <iostream>

namespace {
    // Kinematics, in world pixels per second. Tuned against a 48px tile so the jump
    // clears roughly three and a half tiles, like Super Mario Bros.
    constexpr float kGravity = 2400.f;
    constexpr float kWalkAcceleration = 1800.f;
    constexpr float kMaxWalkSpeed = 420.f;
    constexpr float kGroundFriction = 2000.f;
    constexpr float kJumpSpeed = 900.f;
    constexpr float kJumpCutoff = 0.45f; ///< Releasing jump early shortens the hop.
    constexpr float kMaxFallSpeed = 1400.f;

    /// How fast free-look scrolls the level, and the multiplier while Shift is held.
    constexpr float kFreeLookSpeed = 900.f;
    constexpr float kFreeLookBoost = 3.f;

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
      camera(sf::FloatRect({0.f, 0.f}, {Config::kViewWidth, Config::kViewHeight})) {}

void PlayState::init() {
    std::string charName = (selectedCharacter == CharacterType::Mario) ? "Mario" : "Luigi";
    std::cout << "[Core Engine] PlayState Initialized with character: " << charName << "\n";

    // Load level 1 map file
    if (mapParser.loadFromFile(Systems::resourcePath("assets/maps/level1.txt"))) {
        std::cout << "[Core Engine] Level size: " << mapParser.getWidth() << "x"
                  << mapParser.getHeight() << " tiles\n";
    } else {
        std::cerr << "[Core Engine] Warning: Failed to load level1.txt map!\n";
    }

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

    if (!tileMap.build(mapParser, Config::kZoom)) {
        std::cerr << "[Core Engine] Warning: Level map is empty, nothing to play!\n";
        return;
    }
    std::cout << "[Core Engine] Enemy spawns found: " << tileMap.enemySpawns().size() << "\n";

    // Placeholder avatar: slightly narrower than a tile so it slips into gaps cleanly.
    float tile = tileMap.tileSize();
    avatar.setSize({tile * 0.7f, tile * 0.95f});
    avatar.setFillColor(selectedCharacter == CharacterType::Mario
                            ? sf::Color(216, 40, 0)     // Mario red
                            : sf::Color(0, 168, 0));    // Luigi green
    avatar.setOutlineThickness(-2.f);
    avatar.setOutlineColor(sf::Color(20, 20, 20));
    respawnAvatar();
    updateCamera();

    std::cout << "[Core Engine] Controls: Left/Right (or A/D) to move, Space/Up/W to jump, Esc to quit.\n";
    std::cout << "[Core Engine] Press F for free look: the camera detaches so you can scroll "
                 "through the level with A/D (hold Shift to go faster).\n";
}

void PlayState::handleInput(const sf::Event& event) {
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

        // Press Escape to return to Main Menu
        if (keyPressed->code == sf::Keyboard::Key::Escape) {
            std::cout << "[Core Engine] Escape pressed in PlayState. Returning to IntroMenuState...\n";
            gsm.changeState(std::make_unique<IntroMenuState>(gsm, assets));
        } else if (keyPressed->code == sf::Keyboard::Key::F) {
            freeLook = !freeLook;
            // Pick the scrolling up exactly where the camera already is, so the
            // picture does not jump when the mode changes.
            freeLookCentre = camera.getCenter();
            std::cout << "[Core Engine] Free look " << (freeLook ? "ON" : "OFF") << "\n";
        }
    }
}

void PlayState::update(sf::Time dt) {
    if (freeLook) {
        // The avatar is deliberately frozen: left it running it would walk off or
        // fall into a pit while the camera is somewhere else entirely.
        panCamera(dt);
    } else {
        moveAvatar(dt);
        updateCamera();
    }
    tileMap.update(dt); // keeps the question blocks blinking
}

sf::FloatRect PlayState::avatarBounds() const {
    return sf::FloatRect(avatarPos, avatar.getSize());
}

void PlayState::respawnAvatar() {
    sf::Vector2f spawn = tileMap.playerSpawn();
    // Sit the avatar on the bottom of its spawn tile rather than its top-left corner.
    avatarPos = {spawn.x, spawn.y + tileMap.tileSize() - avatar.getSize().y};
    avatarVelocity = {0.f, 0.f};
    onGround = false;
}

void PlayState::moveAvatar(sf::Time dt) {
    float seconds = dt.asSeconds();

    // --- horizontal intent -------------------------------------------------
    float direction = (wantsRight() ? 1.f : 0.f) - (wantsLeft() ? 1.f : 0.f);
    if (direction != 0.f) {
        avatarVelocity.x += direction * kWalkAcceleration * seconds;
        avatarVelocity.x = std::clamp(avatarVelocity.x, -kMaxWalkSpeed, kMaxWalkSpeed);
    } else {
        // Coast to a stop instead of snapping, so movement keeps some inertia.
        float drop = kGroundFriction * seconds;
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
    for (const sf::FloatRect& tile : tileMap.solidTilesOverlapping(avatarBounds())) {
        if (avatarVelocity.x > 0.f) {
            avatarPos.x = tile.position.x - size.x;
            avatarVelocity.x = 0.f;
        } else if (avatarVelocity.x < 0.f) {
            avatarPos.x = tile.position.x + tile.size.x;
            avatarVelocity.x = 0.f;
        }
    }
    avatarPos.x = std::clamp(avatarPos.x, 0.f, std::max(0.f, tileMap.pixelWidth() - size.x));

    onGround = false;
    avatarPos.y += avatarVelocity.y * seconds;
    for (const sf::FloatRect& tile : tileMap.solidTilesOverlapping(avatarBounds())) {
        if (avatarVelocity.y > 0.f) {
            avatarPos.y = tile.position.y - size.y;
            avatarVelocity.y = 0.f;
            onGround = true;
        } else if (avatarVelocity.y < 0.f) {
            avatarPos.y = tile.position.y + tile.size.y;
            avatarVelocity.y = 0.f;
        }
    }

    // Fell down one of the level's pits: start over from the spawn point.
    if (avatarPos.y > tileMap.pixelHeight()) {
        respawnAvatar();
    }

    avatar.setPosition(avatarPos);
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
    centreCamera(avatarPos + avatar.getSize() / 2.f);
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
        // Show which columns of level1.txt are on screen, so what is drawn can be
        // matched against the map file straight away.
        float leftEdge = camera.getCenter().x - Config::kViewWidth / 2.f;
        int firstColumn = static_cast<int>(leftEdge / Config::kTileSize);
        label = "MAP VIEW   COL " + std::to_string(firstColumn) + "-"
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

    // Sky: the same blue as the level artwork, so tiles blend into the background.
    // It is painted as a rectangle rather than with clear() so it stays inside the
    // game area and leaves the letterbox bars black.
    sf::RectangleShape sky({Config::kViewWidth, Config::kViewHeight});
    sky.setFillColor(sf::Color(92, 148, 252));
    window.draw(sky);

    window.setView(camera);
    window.draw(tileMap);
    window.draw(avatar);

    window.setView(screenView);
    drawFreeLookHint(window); // always on: it doubles as proof the build is current
}
