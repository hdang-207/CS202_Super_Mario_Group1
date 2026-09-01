#pragma once

#include "AABB.hpp"

namespace physics {

[[nodiscard]]
bool isSupportedByPlatform(
    const AABB& bodyBounds,
    bool grounded,
    float verticalVelocity,
    const AABB& platformBounds,
    float horizontalInset,
    float verticalTolerance
) noexcept;

} // namespace physics
