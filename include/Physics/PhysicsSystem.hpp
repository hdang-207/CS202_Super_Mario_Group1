#pragma once

#include <vector>

#include "Physics/AABB.hpp"
#include "Physics/PhysicsBody.hpp"

namespace physics {

class PhysicsSystem {
public:
    explicit PhysicsSystem(
        float gravity = 1600.0f,
        float maxFallSpeed = 900.0f
    ) noexcept;

    void update(
        PhysicsBody& body,
        const std::vector<AABB>& solidColliders,
        float deltaTime
    ) const;

private:
    void integrateVelocity(PhysicsBody& body, float deltaTime) const;

    void moveAndResolveHorizontal(PhysicsBody& body, const std::vector<AABB>& solidColliders,float deltaTime) const;

    void moveAndResolveVertical(PhysicsBody& body, const std::vector<AABB>& solidColliders,float deltaTime) const;

private:
    float m_gravity;
    float m_maxFallSpeed;
};

} // namespace physics