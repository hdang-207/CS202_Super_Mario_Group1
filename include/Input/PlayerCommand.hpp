#pragma once

#include "Input/PlayerInput.hpp"

// DESIGN PATTERN: COMMAND
// Encapsulates a player input action.
class PlayerCommand {
public:
    virtual ~PlayerCommand() = default;

    virtual void execute(PlayerInput& input) const = 0;
};

class MoveCommand final : public PlayerCommand {
public:
    explicit MoveCommand(float direction) noexcept
        : m_direction(direction) {
    }

    void execute(PlayerInput& input) const override {
        input.moveAxis += m_direction;
    }

private:
    float m_direction;
};

class JumpCommand final : public PlayerCommand {
public:
    void execute(PlayerInput& input) const override {
        input.jumpHeld = true;
    }
};