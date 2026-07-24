#pragma once
#include <SFML/System/Vector2.hpp>


namespace physics {


struct AABB {
    sf::Vector2f position{0.0f, 0.0f}; //top left corner of the box
    sf::Vector2f size{0.0f, 0.0f};

    AABB() = default;

    AABB(const sf::Vector2f& position, const sf::Vector2f& size)
        : position(position), size(size) {}

    float left() const {
        return position.x;
    }

    float right() const {
        return position.x + size.x;
    }

    float top() const {
        return position.y;
    }

    float bottom() const {
        return position.y + size.y;
    }
    
    bool intersects(const AABB& other) const {
        return left() < other.right() && right() > other.left() &&
               top() < other.bottom() && bottom() > other.top();
    }


};
}
