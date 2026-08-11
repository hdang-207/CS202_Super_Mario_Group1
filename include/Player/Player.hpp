#pragma once

#include <memory>
#include <SFML/System/Vector2.hpp>

#include "Entities/Character.hpp"
#include "Input/PlayerInput.hpp"
#include "Player/PlayerState.hpp"

namespace entity {

struct PlayerMovementConfig {
    float moveSpeed{200.0f};
    float jumpSpeed{450.0f};
};

class Player : public Character {
public:
    Player(
        const sf::Vector2f& position,
        const sf::Vector2f& colliderSize = {16.0f, 16.0f},
        const sf::Vector2f& colliderOffset = {0.0f, 0.0f},
        const PlayerMovementConfig& movementConfig = {}
    );

    virtual ~Player() override = default;

    /**
     * @brief Set input parameters for the current frame.
     */
    void setInput(const PlayerInput& input) noexcept;

    /**
     * @brief Update player state and convert input to velocity.
     */
    void update(float deltaTime) override;

    /**
     * @brief Render player sprite.
     */
    void render(sf::RenderTarget& target) const override;

    /**
     * @brief Change current player state (State Pattern).
     */
    void changeState(std::unique_ptr<PlayerState> newState);

    [[nodiscard]]
    const PlayerState* getCurrentState() const noexcept;

    [[nodiscard]]
    const PlayerInput& getInput() const noexcept;

    [[nodiscard]]
    const PlayerMovementConfig& getMovementConfig() const noexcept;

protected:
    void processHorizontalMovement();
    void processJump();

private:
    PlayerInput m_input{};
    PlayerMovementConfig m_movementConfig{};
    std::unique_ptr<PlayerState> m_currentState;
};

} // namespace entity