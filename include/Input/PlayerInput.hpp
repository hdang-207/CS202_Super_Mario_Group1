#pragma once


struct PlayerInput {
    // Hướng di chuyển ngang:
    // -1.0f: sang trái
    //  0.0f: đứng yên
    //  1.0f: sang phải
    float moveAxis{0.0f};

    // True đúng tại thời điểm người chơi vừa nhấn nút nhảy.
    bool jumpHeld{false};
};

