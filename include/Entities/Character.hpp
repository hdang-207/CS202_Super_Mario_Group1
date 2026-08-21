#pragma once

#include <SFML/System/Vector2.hpp>
#include <SFML/Graphics/Rect.hpp>
#include "Entities/Entity.hpp"
#include "Physics/PhysicsBody.hpp"

namespace sf {
class RenderTarget;
class RenderWindow;
}

namespace entity {

class Character : public Entity {
public:
    virtual ~Character() override = default;

    void update(sf::Time dt) override {
        update(dt.asSeconds());
    }

    void render(sf::RenderWindow& window) const override;

    virtual void update(float deltaTime) = 0;
    virtual void render(sf::RenderTarget& target) const = 0;

    [[nodiscard]]
    physics::PhysicsBody& getPhysicsBody() noexcept;

    [[nodiscard]]
    const physics::PhysicsBody& getPhysicsBody() const noexcept;

    void setPosition(const sf::Vector2f& position);

    [[nodiscard]]
    const sf::Vector2f& getPosition() const noexcept;

    [[nodiscard]]
    sf::FloatRect getBounds() const override {
        const auto& aabb = m_physicsBody.getAABB();
        return sf::FloatRect(aabb.position, aabb.size);
    }

protected:
    Character(
        const sf::Vector2f& position,
        const sf::Vector2f& colliderSize,
        const sf::Vector2f& colliderOffset = {0.0f, 0.0f}
    );

    physics::PhysicsBody m_physicsBody;
};

} // namespace entity