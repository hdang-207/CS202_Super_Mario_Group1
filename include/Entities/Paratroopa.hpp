#pragma once

#include "Entities/Koopa.hpp"

namespace entity {

class Paratroopa : public Koopa {
public:
    Paratroopa(const sf::Vector2f& position, float tileSize, float initialSpeed = 60.f, float bounceSpeed = 600.f);
    virtual ~Paratroopa() override = default;

    void update(float deltaTime, physics::PhysicsSystem& physicsSystem, const std::vector<physics::AABB>& solids, float mapWidth, float mapHeight) override;

    [[nodiscard]] bool hasWings() const noexcept { return m_hasWings; }
    void removeWings();

private:
    float m_bounceSpeed{600.f};
    bool m_hasWings{true};
};

} // namespace entity
