#include "Physics/PlatformContact.hpp"

#include <cmath>

namespace physics {
namespace {

constexpr float kVelocityEpsilon = 0.001f;

} // namespace

bool isSupportedByPlatform(
    const AABB& bodyBounds,
    bool grounded,
    float verticalVelocity,
    const AABB& platformBounds,
    float horizontalInset,
    float verticalTolerance
) noexcept {
    if (!grounded || verticalVelocity < -kVelocityEpsilon) {
        return false;
    }

    const bool horizontalOverlap =
        bodyBounds.right() > platformBounds.left() + horizontalInset
        && bodyBounds.left() < platformBounds.right() - horizontalInset;
    if (!horizontalOverlap) {
        return false;
    }

    return std::abs(bodyBounds.bottom() - platformBounds.top())
        <= verticalTolerance;
}

} // namespace physics
