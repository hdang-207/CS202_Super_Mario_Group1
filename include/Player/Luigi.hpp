#pragma once

#include "Player/Player.hpp"

namespace entity {

class Luigi : public Player {
public:
    explicit Luigi(
        const sf::Vector2f& position,
        const sf::Vector2f& colliderSize = {16.0f, 16.0f},
        const sf::Vector2f& colliderOffset = {0.0f, 0.0f}
    );
};

} // namespace entity
