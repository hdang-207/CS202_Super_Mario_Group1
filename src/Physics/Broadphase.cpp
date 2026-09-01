#include "Physics/Broadphase.hpp"

#include "Physics/PhysicsBody.hpp"

#include <algorithm>

namespace physics {

AABB sweptBroadphaseBounds(
    const PhysicsBody& body,
    float deltaTime,
    float gravity,
    float maxFallSpeed,
    float safetyMargin
) noexcept {
    const float physicsDelta = std::max(0.0f, deltaTime);
    const AABB currentBounds = body.getAABB();
    sf::Vector2f predictedVelocity = body.getVelocity();
    const sf::Vector2f& acceleration = body.getAcceleration();

    // Match PhysicsSystem's upcoming integration before sizing the query.
    predictedVelocity.x += acceleration.x * physicsDelta;
    predictedVelocity.y += (acceleration.y + gravity) * physicsDelta;
    predictedVelocity.y = std::min(predictedVelocity.y, maxFallSpeed);

    const sf::Vector2f displacement = predictedVelocity * physicsDelta;
    const float left = std::min(
        currentBounds.left(),
        currentBounds.left() + displacement.x
    );
    const float right = std::max(
        currentBounds.right(),
        currentBounds.right() + displacement.x
    );
    const float top = std::min(
        currentBounds.top(),
        currentBounds.top() + displacement.y
    );
    const float bottom = std::max(
        currentBounds.bottom(),
        currentBounds.bottom() + displacement.y
    );

    return AABB(
        {left - safetyMargin, top - safetyMargin},
        {
            right - left + 2.0f * safetyMargin,
            bottom - top + 2.0f * safetyMargin
        }
    );
}

} // namespace physics
