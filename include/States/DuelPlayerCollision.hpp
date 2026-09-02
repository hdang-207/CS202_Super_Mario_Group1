#pragma once

#include "Physics/AABB.hpp"
#include "Physics/PhysicsBody.hpp"

namespace duel {

/**
 * Resolves a side-on collision between the two Duel players.
 *
 * The bounds from the start of the frame establish the players' ordering and
 * distinguish horizontal contact from vertical contact. Only X position and
 * inward X velocity can be changed.
 */
[[nodiscard]] bool resolveHorizontalPlayerCollision(
    physics::PhysicsBody& playerOne,
    physics::PhysicsBody& playerTwo,
    const physics::AABB& previousPlayerOneBounds,
    const physics::AABB& previousPlayerTwoBounds
);

} // namespace duel
