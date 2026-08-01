#pragma once

#include <SFML/System/Vector2.hpp>

#include "Physics/PhysicsBody.hpp"

namespace sf {
class RenderTarget;
}

namespace entity {

class Character {
public:
    virtual ~Character() = default;

    // Mỗi lớp con tự định nghĩa gameplay logic.
    virtual void update(float deltaTime) = 0;

    // Mỗi lớp con tự quản lý cách render sprite/animation.
    virtual void render(sf::RenderTarget& target) const = 0;

    [[nodiscard]]
    physics::PhysicsBody& getPhysicsBody() noexcept;

    [[nodiscard]]
    const physics::PhysicsBody& getPhysicsBody() const noexcept;

    void setPosition(const sf::Vector2f& position);

    [[nodiscard]]
    const sf::Vector2f& getPosition() const noexcept;

protected:
    Character(
        const sf::Vector2f& position,
        const sf::Vector2f& colliderSize,
        const sf::Vector2f& colliderOffset = {0.0f, 0.0f}
    );

    physics::PhysicsBody m_physicsBody;
};

} // namespace entity