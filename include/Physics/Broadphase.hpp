#pragma once

#include "Physics/AABB.hpp"

namespace physics {

class PhysicsBody;

[[nodiscard]]
AABB sweptBroadphaseBounds(
    const PhysicsBody& body,
    float deltaTime,
    float gravity,
    float maxFallSpeed,
    float safetyMargin
) noexcept;

} // namespace physics
