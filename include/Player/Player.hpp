#pragma once

#include <SFML/System/Vector2.hpp>

#include "Entities/Character.hpp"
#include "Input/PlayerInput.hpp"

namespace entity {

struct PlayerMovementConfig {
    float moveSpeed{200.0f};
    float jumpSpeed{450.0f};
};

class Player : public Character {
public:
    virtual ~Player() override = default;

    /**
     * @brief Set input parameters for the current frame.
     */
    void setInput(const PlayerInput& input) noexcept;

    /**
     * @brief Update player state and convert input to velocity.
     */
    void update(float deltaTime) override;

    [[nodiscard]]
    const PlayerInput& getInput() const noexcept;

    [[nodiscard]]
    const PlayerMovementConfig& getMovementConfig() const noexcept;

protected:
    Player(
        const sf::Vector2f& position,
        const sf::Vector2f& colliderSize,
        const sf::Vector2f& colliderOffset,
        const PlayerMovementConfig& movementConfig
    );

    void processHorizontalMovement();
    void processJump();

private:
    PlayerInput m_input{};
    PlayerMovementConfig m_movementConfig{};
};

} // namespace entity