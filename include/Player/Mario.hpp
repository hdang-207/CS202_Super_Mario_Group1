#pragma once

#include "Player/Player.hpp"

namespace entity {

class Mario : public Player {
public:
    explicit Mario(
        const sf::Vector2f& position,
        const sf::Vector2f& colliderSize = {16.0f, 16.0f},
        const sf::Vector2f& colliderOffset = {0.0f, 0.0f}
    );
};

} // namespace entity
