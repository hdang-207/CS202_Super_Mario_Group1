#pragma once

#include "Entities/Koopa.hpp"

namespace entity {

class Paratroopa : public Koopa {
public:
    Paratroopa(const sf::Vector2f& position, float tileSize, const sf::Texture* walkingTexture = nullptr, const sf::Texture* shellTexture = nullptr, float initialSpeed = 60.f, float bounceSpeed = 600.f, KoopaKind kind = KoopaKind::Green);
    virtual ~Paratroopa() override = default;

    void update(float deltaTime) override;
    void update(float deltaTime, physics::PhysicsSystem& physicsSystem, const std::vector<physics::AABB>& solids, float mapWidth, float mapHeight);

    void stomp();
    [[nodiscard]] bool hasWings() const noexcept { return m_hasWings; }

private:
    bool m_hasWings{true};
    float m_bounceSpeed{600.f};
    float m_bounceElapsed{0.f};
};

} // namespace entity
