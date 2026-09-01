#pragma once

#include "Input/InputHandler.hpp"
#include "Physics/PhysicsSystem.hpp"
#include "Player/Player.hpp"
#include "Player/PlayerAnimator.hpp"
#include "States/State.hpp"
#include "Systems/MapParser.hpp"
#include "Systems/TileMap.hpp"
#include <memory>
#include <set>
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

    sf::RectangleShape skyBackground;
    sf::Text titleText;
    sf::Text errorText;
    sf::Text hintText;

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
        sf::Time dt
    );
    static sf::Vector2f feetCentre(const entity::Player& player);

public:
    DuelState(GameStateManager& gsm, Systems::AssetManager& assets);
    ~DuelState() override = default;

    void init() override;
    void handleInput(const sf::Event& event) override;
    void update(sf::Time dt) override;
    void render(sf::RenderWindow& window) override;
};
