#pragma once


struct PlayerInput {
    // Horizontal movement direction: -1.0f (left), 0.0f (idle), 1.0f (right)
    float moveAxis{0.0f};

    // True while the player holds the jump key
    bool jumpHeld{false};
};

