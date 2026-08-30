#pragma once

#include <memory>
#include <vector>
#include <SFML/System/Vector2.hpp>

#include "Entities/Character.hpp"
#include "Input/PlayerInput.hpp"
#include "Player/PlayerState.hpp"
#include "Player/PowerType.hpp"

namespace entity {

struct PlayerMovementConfig {
    float moveSpeed{420.0f};
    float jumpSpeed{1000.0f};
    float groundFriction{2000.0f};
    float walkAcceleration{1800.0f};
    float airFrictionMultiplier{0.02f};
    float jumpCutoff{0.45f};
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

    void applyPower(PowerType power);
    bool removeLatestPower();

    /**
     * @brief Ducks or stands the player back up.
     *
     * Only the Super and Fire forms can duck, and only with both feet on the
     * ground - crouching mid-jump would freeze Mario in the air.
     */
    void setCrouching(bool crouching) noexcept;

    [[nodiscard]] bool isCrouching() const noexcept;

    [[nodiscard]] bool hasPower(PowerType power) const noexcept;
    [[nodiscard]] bool isSuper() const noexcept;
    [[nodiscard]] bool hasFirePower() const noexcept;

protected:
    void processHorizontalMovement(float deltaTime);
    void processJump();

private:
    void setSuperCollider(bool super);

    PlayerInput m_input{};
    PlayerMovementConfig m_movementConfig{};
    std::unique_ptr<PlayerState> m_currentState;
    bool m_crouching{false};
    sf::Vector2f m_smallColliderSize{};
    std::vector<PowerType> m_powerStack;
};

} // namespace entity
