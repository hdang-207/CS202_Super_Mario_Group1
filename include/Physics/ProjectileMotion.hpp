#pragma once

#include <SFML/System/Vector2.hpp>

namespace physics {

struct ProjectileMotion {
    sf::Vector2f position{0.f, 0.f};
    sf::Vector2f velocity{0.f, 0.f};
    sf::Vector2f acceleration{0.f, 0.f};

    void integrate(float deltaTime) noexcept {
        if (deltaTime <= 0.f) {
            return;
        }

        velocity += acceleration * deltaTime;
        position += velocity * deltaTime;
    }
};

} // namespace physics
