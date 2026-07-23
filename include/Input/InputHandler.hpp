#include "PlayerInput.hpp"


class InputHandler {
public:
    void update();

    void reset();

    //getter
    const PlayerInput& getPlayerInput() const;
private:
    PlayerInput playerInput{};
};