#pragma once

#include "PlayerInput.hpp"
#include "Input/PlayerCommand.hpp"
#include <SFML/Window/Keyboard.hpp>
#include <set>

class InputHandler {
public:
    /**
     * @brief Converts event-tracked keys into the current gameplay command state.
     *
     * Using window events avoids macOS Input Monitoring permission requirements
     * that can make global sf::Keyboard polling report every key as released.
     */
    void update(const std::set<sf::Keyboard::Key>& heldKeys);

    void reset();

    //getter
    const PlayerInput& getPlayerInput() const noexcept;
private:
    PlayerInput m_playerInput{};

    MoveCommand m_moveLeftCommand{-1.0f};
    MoveCommand m_moveRightCommand{1.0f};
    JumpCommand m_jumpCommand{};
};
