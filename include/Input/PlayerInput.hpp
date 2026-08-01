#pragma once


struct PlayerInput {
    // Hướng di chuyển ngang:
    // -1.0f: sang trái
    //  0.0f: đứng yên
    //  1.0f: sang phải
    float moveAxis{0.0f};

    // True tại toàn bộ frame người chơi giữ nút nhảy.
    bool jumpHeld{false};
};

