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

    // Xóa các kết quả collision của physics step trước.
    body.beginPhysicsStep();

    // Gravity là acceleration theo hướng Y dương.
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

    // Acceleration chỉ có hiệu lực trong physics step này.
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

    // Không cho vận tốc rơi vượt quá giới hạn.
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

    // Chỉ di chuyển theo X.
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
            // Player đang đi sang phải.
            // Đẩy cạnh phải của Player về cạnh trái tile.
            const float correction =
                solid.left() - bodyBox.right();

            position.x += correction;

            body.setHitWallRight(true);
        } else {
            // Player đang đi sang trái.
            // Đẩy cạnh trái của Player về cạnh phải tile.
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

    // Chỉ di chuyển theo Y.
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
            // Player đang rơi xuống.
            // Đưa đáy collider lên đúng mặt trên tile.
            const float correction =
                solid.top() - bodyBox.bottom();

            position.y += correction;

            body.setGrounded(true);
        } else {
            // Player đang bay lên.
            // Đưa đỉnh collider xuống dưới tile.
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