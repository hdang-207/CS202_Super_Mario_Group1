#include "States/DuelPlayerCollision.hpp"

#include <algorithm>
#include <cmath>
#include <limits>

namespace {

constexpr float kCollisionEpsilon = 0.0001f;

struct AxisSweep {
    float entryTime{0.f};
    float exitTime{0.f};
    bool canOverlap{false};
};

AxisSweep sweepAxis(
    float movingMinimum,
    float movingMaximum,
    float otherMinimum,
    float otherMaximum,
    float relativeDisplacement
) {
    if (relativeDisplacement > kCollisionEpsilon) {
        return {
            (otherMinimum - movingMaximum) / relativeDisplacement,
            (otherMaximum - movingMinimum) / relativeDisplacement,
            true
        };
    }

    if (relativeDisplacement < -kCollisionEpsilon) {
        return {
            (otherMaximum - movingMinimum) / relativeDisplacement,
            (otherMinimum - movingMaximum) / relativeDisplacement,
            true
        };
    }

    const bool overlaps = movingMinimum < otherMaximum - kCollisionEpsilon
        && movingMaximum > otherMinimum + kCollisionEpsilon;
    if (!overlaps) {
        return {};
    }

    return {
        -std::numeric_limits<float>::infinity(),
        std::numeric_limits<float>::infinity(),
        true
    };
}

} // namespace

namespace duel {

bool resolveHorizontalPlayerCollision(
    physics::PhysicsBody& playerOne,
    physics::PhysicsBody& playerTwo,
    const physics::AABB& previousPlayerOneBounds,
    const physics::AABB& previousPlayerTwoBounds
) {
    physics::PhysicsBody* leftBody = &playerOne;
    physics::PhysicsBody* rightBody = &playerTwo;
    const physics::AABB* previousLeft = &previousPlayerOneBounds;
    const physics::AABB* previousRight = &previousPlayerTwoBounds;

    if (previousPlayerTwoBounds.right()
        <= previousPlayerOneBounds.left() + kCollisionEpsilon) {
        std::swap(leftBody, rightBody);
        std::swap(previousLeft, previousRight);
    } else if (previousPlayerOneBounds.right()
               > previousPlayerTwoBounds.left() + kCollisionEpsilon) {
        // The previous frame did not establish a side-on ordering. Combat
        // handlers own vertical and already-overlapping contacts.
        return false;
    }

    const physics::AABB currentLeft = leftBody->getAABB();
    const physics::AABB currentRight = rightBody->getAABB();
    const float leftDisplacementX = currentLeft.left() - previousLeft->left();
    const float rightDisplacementX = currentRight.left() - previousRight->left();
    const float relativeDisplacementX =
        leftDisplacementX - rightDisplacementX;
    if (relativeDisplacementX <= kCollisionEpsilon) {
        return false;
    }

    const float previousGapX = std::max(
        0.f,
        previousRight->left() - previousLeft->right()
    );
    const float correction = relativeDisplacementX - previousGapX;
    // Reaching the exact edge is not penetration and needs no correction.
    if (correction <= kCollisionEpsilon) {
        return false;
    }

    const float leftDisplacementY = currentLeft.top() - previousLeft->top();
    const float rightDisplacementY = currentRight.top() - previousRight->top();
    const AxisSweep verticalSweep = sweepAxis(
        previousLeft->top(),
        previousLeft->bottom(),
        previousRight->top(),
        previousRight->bottom(),
        leftDisplacementY - rightDisplacementY
    );
    if (!verticalSweep.canOverlap) {
        return false;
    }

    const float horizontalEntryTime =
        previousGapX / relativeDisplacementX;
    const float horizontalExitTime =
        (previousRight->right() - previousLeft->left())
        / relativeDisplacementX;
    const float entryTime = std::max(
        horizontalEntryTime,
        verticalSweep.entryTime
    );
    const float exitTime = std::min(
        horizontalExitTime,
        verticalSweep.exitTime
    );

    const bool collidesDuringFrame = entryTime <= 1.f + kCollisionEpsilon
        && exitTime >= -kCollisionEpsilon
        && entryTime + kCollisionEpsilon < exitTime;
    // The later entry axis determines the contact normal. A tie belongs to Y,
    // preserving stomp and corner-landing behavior.
    const bool horizontalContact = horizontalEntryTime
        > verticalSweep.entryTime + kCollisionEpsilon;
    if (!collidesDuringFrame || !horizontalContact) {
        return false;
    }

    const float leftClosingDisplacement = std::max(0.f, leftDisplacementX);
    const float rightClosingDisplacement = std::max(0.f, -rightDisplacementX);
    const float totalClosingDisplacement =
        leftClosingDisplacement + rightClosingDisplacement;
    if (totalClosingDisplacement <= kCollisionEpsilon) {
        return false;
    }

    sf::Vector2f leftPosition = leftBody->getPosition();
    sf::Vector2f rightPosition = rightBody->getPosition();
    leftPosition.x -= correction
        * (leftClosingDisplacement / totalClosingDisplacement);
    rightPosition.x += correction
        * (rightClosingDisplacement / totalClosingDisplacement);
    leftBody->setPosition(leftPosition);
    rightBody->setPosition(rightPosition);

    sf::Vector2f leftVelocity = leftBody->getVelocity();
    if (leftVelocity.x > 0.f) {
        leftVelocity.x = 0.f;
        leftBody->setVelocity(leftVelocity);
    }

    sf::Vector2f rightVelocity = rightBody->getVelocity();
    if (rightVelocity.x < 0.f) {
        rightVelocity.x = 0.f;
        rightBody->setVelocity(rightVelocity);
    }

    return true;
}

} // namespace duel
