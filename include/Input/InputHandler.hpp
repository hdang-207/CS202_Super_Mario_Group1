#pragma once

#include "PlayerInput.hpp"
#include "Input/PlayerCommand.hpp"



class InputHandler {
public:
    void update();

    void reset();

    //getter
    const PlayerInput& getPlayerInput() const noexcept;
private:
    PlayerInput m_playerInput{};

    MoveCommand m_moveLeftCommand{-1.0f};
    MoveCommand m_moveRightCommand{1.0f};
    JumpCommand m_jumpCommand{};
};
