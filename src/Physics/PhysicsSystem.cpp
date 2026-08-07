#include "Physics/PhysicsSystem.hpp"

#include <algorithm>

namespace physics {

PhysicsSystem::PhysicsSystem(float gravity, float maxFallSpeed) noexcept : m_gravity(gravity), m_maxFallSpeed(maxFallSpeed) {}

void PhysicsSystem::update(
    PhysicsBody& body,
    const std::vector<AABB>& solidColliders,
    float deltaTime
) const {
    if (deltaTime <= 0.0f) {
        return;
    }

    // Clear collision results from previous physics step.
    body.beginPhysicsStep();

    // Gravity acts as acceleration along positive Y axis.
    sf::Vector2f acceleration =
        body.getAcceleration();

    acceleration.y += m_gravity;

    body.setAcceleration(acceleration);

    // acceleration → velocity
    integrateVelocity(body, deltaTime);

    // velocity.x → position.x → resolve X
    moveAndResolveHorizontal(
        body,
        solidColliders,
        deltaTime
    );

    // velocity.y → position.y → resolve Y
    moveAndResolveVertical(
        body,
        solidColliders,
        deltaTime
    );

    // Acceleration is only effective during this physics step.
    body.setAcceleration({0.0f, 0.0f});
}

void PhysicsSystem::integrateVelocity(
    PhysicsBody& body,
    float deltaTime
) const {
    sf::Vector2f velocity =
        body.getVelocity();

    const sf::Vector2f& acceleration =
        body.getAcceleration();

    velocity.x += acceleration.x * deltaTime;
    velocity.y += acceleration.y * deltaTime;

    // Clamp fall velocity to maximum allowed speed.
    velocity.y = std::min(
        velocity.y,
        m_maxFallSpeed
    );

    body.setVelocity(velocity);
}

void PhysicsSystem::moveAndResolveHorizontal(PhysicsBody& body, const std::vector<AABB>& solidColliders, float deltaTime) const {
    sf::Vector2f velocity =
        body.getVelocity();

    if (velocity.x == 0.0f) {
        return;
    }

    const float movementDirection = velocity.x;

    // Move along X axis only.
    sf::Vector2f position =
        body.getPosition();

    position.x += velocity.x * deltaTime;

    body.setPosition(position);

    bool collided = false;

    for (const AABB& solid : solidColliders) {
        AABB bodyBox = body.getAABB();

        if (!bodyBox.intersects(solid)) {
            continue;
        }

        position = body.getPosition();

        if (movementDirection > 0.0f) {
            // Moving right: push right edge of collider to left edge of tile.
            const float correction =
                solid.left() - bodyBox.right();

            position.x += correction;

            body.setHitWallRight(true);
        } else {
            // Moving left: push left edge of collider to right edge of tile.
            const float correction =
                solid.right() - bodyBox.left();

            position.x += correction;

            body.setHitWallLeft(true);
        }

        body.setPosition(position);
        collided = true;
    }

    if (collided) {
        velocity.x = 0.0f;
        body.setVelocity(velocity);
    }
}

void PhysicsSystem::moveAndResolveVertical(
    PhysicsBody& body,
    const std::vector<AABB>& solidColliders,
    float deltaTime
) const {
    sf::Vector2f velocity =
        body.getVelocity();

    if (velocity.y == 0.0f) {
        return;
    }

    const float movementDirection = velocity.y;

    // Move along Y axis only.
    sf::Vector2f position =
        body.getPosition();

    position.y += velocity.y * deltaTime;

    body.setPosition(position);

    bool collided = false;

    for (const AABB& solid : solidColliders) {
        AABB bodyBox = body.getAABB();

        if (!bodyBox.intersects(solid)) {
            continue;
        }

        position = body.getPosition();

        if (movementDirection > 0.0f) {
            // Falling down: snap bottom of collider to top of tile.
            const float correction =
                solid.top() - bodyBox.bottom();

            position.y += correction;

            body.setGrounded(true);
        } else {
            // Moving upward: snap top of collider to bottom of tile.
            const float correction =
                solid.bottom() - bodyBox.top();

            position.y += correction;

            body.setHitCeiling(true);
        }

        body.setPosition(position);
        collided = true;
    }

    if (collided) {
        velocity.y = 0.0f;
        body.setVelocity(velocity);
    }
}

} // namespace physics