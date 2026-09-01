#include "Physics/AABB.hpp"
#include "Physics/Broadphase.hpp"
#include "Physics/PhysicsBody.hpp"
#include "Physics/PhysicsSystem.hpp"
#include "Physics/PlatformContact.hpp"

#include <cmath>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

namespace {

constexpr float kTestEpsilon = 0.001f;

struct TestContext {
    int failures = 0;
    std::vector<std::string> diagnostics;

    void expectTrue(bool condition, const std::string& message) {
        if (condition) {
            return;
        }

        ++failures;
        diagnostics.push_back(message);
    }

    void expectNear(
        float actual,
        float expected,
        const std::string& label,
        float epsilon = kTestEpsilon
    ) {
        if (std::abs(actual - expected) <= epsilon) {
            return;
        }

        ++failures;
        std::ostringstream message;
        message << label << ": expected " << expected << ", actual " << actual;
        diagnostics.push_back(message.str());
    }

    void expectGreaterOrEqual(
        float actual,
        float expected,
        const std::string& label
    ) {
        if (actual + kTestEpsilon >= expected) {
            return;
        }

        ++failures;
        std::ostringstream message;
        message << label << ": expected >= " << expected << ", actual " << actual;
        diagnostics.push_back(message.str());
    }
};

using TestFunction = void (*)(TestContext&);

void runTest(
    const char* name,
    TestFunction test,
    int& passedTests,
    int& failedTests
) {
    TestContext context;
    test(context);

    if (context.failures == 0) {
        ++passedTests;
        std::cout << "[PASS] " << name << '\n';
        return;
    }

    ++failedTests;
    std::cout << "[FAIL] " << name << '\n';
    for (const std::string& diagnostic : context.diagnostics) {
        std::cout << "       " << diagnostic << '\n';
    }
}

void testPhysicsBodyAabb(TestContext& context) {
    const physics::PhysicsBody body(
        {10.0f, 20.0f},
        {16.0f, 24.0f},
        {2.0f, 3.0f}
    );
    const physics::AABB bounds = body.getAABB();

    context.expectNear(bounds.left(), 12.0f, "collider left");
    context.expectNear(bounds.top(), 23.0f, "collider top");
    context.expectNear(bounds.right(), 28.0f, "collider right");
    context.expectNear(bounds.bottom(), 47.0f, "collider bottom");

    const physics::AABB overlapping({27.0f, 46.0f}, {4.0f, 4.0f});
    const physics::AABB edgeTouching({28.0f, 23.0f}, {4.0f, 4.0f});
    context.expectTrue(bounds.intersects(overlapping), "overlapping AABBs should intersect");
    context.expectTrue(
        !bounds.intersects(edgeTouching),
        "edge-touching AABBs should not intersect"
    );
}

void testExactPlatformSupport(TestContext& context) {
    const physics::AABB player({120.0f, 160.0f}, {24.0f, 40.0f});
    const physics::AABB platform({100.0f, 200.0f}, {144.0f, 48.0f});

    context.expectTrue(
        physics::isSupportedByPlatform(player, true, 0.0f, platform, 2.0f, 2.0f),
        "grounded player at platform top should be supported"
    );
}

void testAirbornePlayerIsNotSupported(TestContext& context) {
    const physics::AABB player({120.0f, 160.0f}, {24.0f, 40.0f});
    const physics::AABB platform({100.0f, 200.0f}, {144.0f, 48.0f});

    context.expectTrue(
        !physics::isSupportedByPlatform(player, false, 0.0f, platform, 2.0f, 2.0f),
        "airborne player at platform top should not be supported"
    );
}

void testUpwardMovingPlayerIsNotSupported(TestContext& context) {
    const physics::AABB player({120.0f, 160.0f}, {24.0f, 40.0f});
    const physics::AABB platform({100.0f, 200.0f}, {144.0f, 48.0f});

    context.expectTrue(
        !physics::isSupportedByPlatform(player, true, -10.0f, platform, 2.0f, 2.0f),
        "upward-moving player should not be supported despite stale grounded state"
    );
}

void testDeepPlatformProximityIsNotSupport(TestContext& context) {
    const physics::AABB player({120.0f, 168.0f}, {24.0f, 40.0f});
    const physics::AABB platform({100.0f, 200.0f}, {144.0f, 48.0f});

    context.expectTrue(
        !physics::isSupportedByPlatform(player, true, 0.0f, platform, 2.0f, 2.0f),
        "player eight pixels below platform top should not be supported"
    );
}

void testSmallPlatformContactErrorIsSupported(TestContext& context) {
    const physics::AABB player({120.0f, 161.5f}, {24.0f, 40.0f});
    const physics::AABB platform({100.0f, 200.0f}, {144.0f, 48.0f});

    context.expectTrue(
        physics::isSupportedByPlatform(player, true, 0.0f, platform, 2.0f, 2.0f),
        "player feet within contact tolerance should be supported"
    );
}

void testMissingHorizontalPlatformOverlapIsNotSupport(TestContext& context) {
    const physics::AABB player({78.0f, 160.0f}, {24.0f, 40.0f});
    const physics::AABB platform({100.0f, 200.0f}, {144.0f, 48.0f});

    context.expectTrue(
        !physics::isSupportedByPlatform(player, true, 0.0f, platform, 2.0f, 2.0f),
        "player outside the inset platform surface should not be supported"
    );
}

void testTallPlayerPlatformSupport(TestContext& context) {
    const physics::AABB player({120.0f, 120.0f}, {24.0f, 80.0f});
    const physics::AABB platform({100.0f, 200.0f}, {144.0f, 48.0f});

    context.expectTrue(
        physics::isSupportedByPlatform(player, true, 0.0f, platform, 2.0f, 2.0f),
        "support should depend on player feet rather than collider height"
    );
}

void testSweptBroadphaseMaxSpeedFall(TestContext& context) {
    constexpr float gravity = 2400.0f;
    constexpr float maxFallSpeed = 1400.0f;
    constexpr float deltaTime = 1.0f / 60.0f;
    constexpr float safetyMargin = 1.0f;

    physics::PhysicsBody body({100.0f, 200.0f}, {20.0f, 30.0f});
    body.setVelocity({120.0f, 1400.0f});
    body.setAcceleration({60.0f, 100.0f});

    const physics::AABB current = body.getAABB();
    const physics::AABB swept = physics::sweptBroadphaseBounds(
        body,
        deltaTime,
        gravity,
        maxFallSpeed,
        safetyMargin
    );
    const float predictedHorizontalVelocity = 120.0f + 60.0f * deltaTime;
    const float predictedHorizontalDisplacement =
        predictedHorizontalVelocity * deltaTime;
    const float predictedVerticalDisplacement = maxFallSpeed * deltaTime;

    context.expectNear(
        swept.left(),
        current.left() - safetyMargin,
        "swept left"
    );
    context.expectNear(
        swept.right(),
        current.right() + predictedHorizontalDisplacement + safetyMargin,
        "swept right"
    );
    context.expectNear(
        swept.top(),
        current.top() - safetyMargin,
        "swept top"
    );
    context.expectNear(
        swept.bottom(),
        current.bottom() + predictedVerticalDisplacement + safetyMargin,
        "swept bottom"
    );
    context.expectGreaterOrEqual(
        swept.bottom() - safetyMargin,
        current.bottom() + predictedVerticalDisplacement,
        "fall coverage"
    );
    context.expectTrue(
        predictedVerticalDisplacement > 16.0f,
        "test setup must exceed the old fixed 16 px broadphase"
    );
}

void testSweptBroadphaseNegativeMovement(TestContext& context) {
    constexpr float deltaTime = 0.5f;
    constexpr float safetyMargin = 2.0f;

    physics::PhysicsBody body({50.0f, 70.0f}, {10.0f, 20.0f});
    body.setVelocity({-40.0f, -30.0f});
    body.setAcceleration({-20.0f, -10.0f});

    const physics::AABB current = body.getAABB();
    const physics::AABB swept = physics::sweptBroadphaseBounds(
        body,
        deltaTime,
        0.0f,
        1000.0f,
        safetyMargin
    );

    context.expectNear(swept.left(), 23.0f, "leftward swept left");
    context.expectNear(swept.right(), current.right() + safetyMargin, "leftward swept right");
    context.expectNear(swept.top(), 50.5f, "upward swept top");
    context.expectNear(swept.bottom(), current.bottom() + safetyMargin, "upward swept bottom");
    context.expectTrue(
        swept.left() <= current.left() && swept.right() >= current.right()
            && swept.top() <= current.top() && swept.bottom() >= current.bottom(),
        "swept bounds should contain the current AABB"
    );

    const physics::AABB negativeDelta = physics::sweptBroadphaseBounds(
        body,
        -0.25f,
        0.0f,
        1000.0f,
        safetyMargin
    );
    context.expectNear(negativeDelta.left(), current.left() - safetyMargin, "negative-dt left");
    context.expectNear(negativeDelta.right(), current.right() + safetyMargin, "negative-dt right");
    context.expectNear(negativeDelta.top(), current.top() - safetyMargin, "negative-dt top");
    context.expectNear(negativeDelta.bottom(), current.bottom() + safetyMargin, "negative-dt bottom");
}

void testRightWallCollision(TestContext& context) {
    physics::PhysicsBody body({70.0f, 20.0f}, {20.0f, 20.0f});
    body.setVelocity({100.0f, 0.0f});
    const std::vector<physics::AABB> solids{
        {{100.0f, 0.0f}, {20.0f, 200.0f}}
    };

    const physics::PhysicsSystem system(0.0f, 1400.0f);
    system.update(body, solids, 0.2f);

    context.expectNear(body.getAABB().right(), solids.front().left(), "player right edge");
    context.expectTrue(body.hitWallRight(), "right wall flag should be set");
    context.expectTrue(!body.hitWallLeft(), "left wall flag should remain clear");
    context.expectNear(body.getVelocity().x, 0.0f, "horizontal velocity");
}

void testLeftWallCollision(TestContext& context) {
    physics::PhysicsBody body({110.0f, 20.0f}, {20.0f, 20.0f});
    body.setVelocity({-100.0f, 0.0f});
    const std::vector<physics::AABB> solids{
        {{80.0f, 0.0f}, {20.0f, 200.0f}}
    };

    const physics::PhysicsSystem system(0.0f, 1400.0f);
    system.update(body, solids, 0.2f);

    context.expectNear(body.getAABB().left(), solids.front().right(), "player left edge");
    context.expectTrue(body.hitWallLeft(), "left wall flag should be set");
    context.expectTrue(!body.hitWallRight(), "right wall flag should remain clear");
    context.expectNear(body.getVelocity().x, 0.0f, "horizontal velocity");
}

void testFloorLanding(TestContext& context) {
    physics::PhysicsBody body({20.0f, 70.0f}, {20.0f, 20.0f});
    body.setVelocity({0.0f, 100.0f});
    const std::vector<physics::AABB> solids{
        {{0.0f, 100.0f}, {200.0f, 20.0f}}
    };

    const physics::PhysicsSystem system(0.0f, 1400.0f);
    system.update(body, solids, 0.2f);

    context.expectNear(body.getAABB().bottom(), solids.front().top(), "player bottom");
    context.expectTrue(body.isGrounded(), "grounded flag should be set");
    context.expectNear(body.getVelocity().y, 0.0f, "vertical velocity");
    context.expectTrue(!body.hitCeiling(), "ceiling flag should remain clear");
}

void testCeilingCollision(TestContext& context) {
    physics::PhysicsBody body({20.0f, 110.0f}, {20.0f, 20.0f});
    body.setVelocity({0.0f, -100.0f});
    const std::vector<physics::AABB> solids{
        {{0.0f, 80.0f}, {200.0f, 20.0f}}
    };

    const physics::PhysicsSystem system(0.0f, 1400.0f);
    system.update(body, solids, 0.2f);

    context.expectNear(body.getAABB().top(), solids.front().bottom(), "player top");
    context.expectTrue(body.hitCeiling(), "ceiling flag should be set");
    context.expectTrue(!body.isGrounded(), "grounded flag should remain clear");
    context.expectNear(body.getVelocity().y, 0.0f, "vertical velocity");
}

void testDiagonalLandingPreservesHorizontalMovement(TestContext& context) {
    physics::PhysicsBody body({40.0f, 70.0f}, {20.0f, 20.0f});
    body.setVelocity({50.0f, 100.0f});
    const std::vector<physics::AABB> solids{
        {{0.0f, 100.0f}, {200.0f, 20.0f}}
    };
    const float beforeX = body.getPosition().x;

    const physics::PhysicsSystem system(0.0f, 1400.0f);
    system.update(body, solids, 0.2f);

    context.expectGreaterOrEqual(body.getPosition().x, beforeX, "horizontal position");
    context.expectNear(body.getPosition().x, 50.0f, "horizontal movement");
    context.expectTrue(body.isGrounded(), "grounded flag should be set");
    context.expectTrue(!body.hitWallRight(), "right wall flag should remain clear");
    context.expectNear(body.getVelocity().x, 50.0f, "horizontal velocity");
    context.expectNear(body.getVelocity().y, 0.0f, "vertical velocity");
}

void testFloorOverlapIsNotSideWallCollision(TestContext& context) {
    physics::PhysicsBody body({20.0f, 90.0f}, {20.0f, 20.0f});
    body.setVelocity({50.0f, 0.0f});
    const std::vector<physics::AABB> solids{
        {{0.0f, 108.0f}, {200.0f, 20.0f}}
    };

    const physics::PhysicsSystem system(0.0f, 1400.0f);
    system.update(body, solids, 0.1f);

    context.expectNear(body.getPosition().x, 25.0f, "horizontal movement");
    context.expectTrue(!body.hitWallRight(), "floor overlap should not set right wall");
    context.expectTrue(!body.hitWallLeft(), "floor overlap should not set left wall");
    context.expectNear(body.getVelocity().x, 50.0f, "horizontal velocity");
}

void testWallOverlapIsNotFloorCollision(TestContext& context) {
    physics::PhysicsBody body({95.0f, 100.0f}, {20.0f, 20.0f});
    body.setVelocity({0.0f, 50.0f});
    const std::vector<physics::AABB> solids{
        {{100.0f, 50.0f}, {20.0f, 200.0f}}
    };

    const physics::PhysicsSystem system(0.0f, 1400.0f);
    system.update(body, solids, 0.1f);

    context.expectNear(body.getPosition().y, 105.0f, "vertical movement");
    context.expectTrue(!body.isGrounded(), "wall overlap should not set grounded");
    context.expectNear(body.getVelocity().y, 50.0f, "vertical velocity");
}

void testLandingAcrossAdjacentFloorTiles(TestContext& context) {
    physics::PhysicsBody body({90.0f, 80.0f}, {20.0f, 20.0f});
    body.setVelocity({10.0f, 20.0f});
    const std::vector<physics::AABB> solids{
        {{80.0f, 100.0f}, {20.0f, 20.0f}},
        {{100.0f, 100.0f}, {20.0f, 20.0f}}
    };

    const physics::PhysicsSystem system(0.0f, 1400.0f);
    system.update(body, solids, 0.1f);

    context.expectTrue(body.isGrounded(), "grounded flag should be set at the seam");
    context.expectTrue(!body.hitWallLeft(), "tile seam should not set left wall");
    context.expectTrue(!body.hitWallRight(), "tile seam should not set right wall");
    context.expectNear(body.getPosition().x, 91.0f, "horizontal position");
    context.expectNear(body.getVelocity().x, 10.0f, "horizontal velocity");
    context.expectNear(body.getAABB().bottom(), 100.0f, "player bottom");
}

void testMaxFallSpeedClamp(TestContext& context) {
    constexpr float maxFallSpeed = 1400.0f;
    constexpr float deltaTime = 1.0f / 60.0f;

    physics::PhysicsBody body({0.0f, 0.0f}, {20.0f, 20.0f});
    body.setVelocity({0.0f, 2000.0f});

    const physics::PhysicsSystem system(0.0f, maxFallSpeed);
    system.update(body, {}, deltaTime);

    context.expectNear(body.getVelocity().y, maxFallSpeed, "vertical velocity");
    context.expectNear(
        body.getPosition().y,
        maxFallSpeed * deltaTime,
        "vertical position"
    );
}

} // namespace

int main() {
    int passedTests = 0;
    int failedTests = 0;

    runTest("PhysicsBody AABB semantics", testPhysicsBodyAabb, passedTests, failedTests);
    runTest("exact moving-platform support", testExactPlatformSupport, passedTests, failedTests);
    runTest("airborne player is not platform-supported", testAirbornePlayerIsNotSupported, passedTests, failedTests);
    runTest("upward player is not platform-supported", testUpwardMovingPlayerIsNotSupported, passedTests, failedTests);
    runTest("deep platform proximity is not support", testDeepPlatformProximityIsNotSupport, passedTests, failedTests);
    runTest("small platform contact error is supported", testSmallPlatformContactErrorIsSupported, passedTests, failedTests);
    runTest("missing horizontal platform overlap is not support", testMissingHorizontalPlatformOverlapIsNotSupport, passedTests, failedTests);
    runTest("tall player remains platform-supported", testTallPlayerPlatformSupport, passedTests, failedTests);
    runTest("swept broadphase covers max-speed fall", testSweptBroadphaseMaxSpeedFall, passedTests, failedTests);
    runTest("swept broadphase handles negative movement", testSweptBroadphaseNegativeMovement, passedTests, failedTests);
    runTest("right wall collision", testRightWallCollision, passedTests, failedTests);
    runTest("left wall collision", testLeftWallCollision, passedTests, failedTests);
    runTest("floor landing", testFloorLanding, passedTests, failedTests);
    runTest("ceiling collision", testCeilingCollision, passedTests, failedTests);
    runTest("diagonal landing preserves horizontal movement", testDiagonalLandingPreservesHorizontalMovement, passedTests, failedTests);
    runTest("pre-existing floor overlap is not a side-wall collision", testFloorOverlapIsNotSideWallCollision, passedTests, failedTests);
    runTest("pre-existing wall overlap is not a floor collision", testWallOverlapIsNotFloorCollision, passedTests, failedTests);
    runTest("landing across adjacent floor tiles", testLandingAcrossAdjacentFloorTiles, passedTests, failedTests);
    runTest("max fall speed clamp", testMaxFallSpeedClamp, passedTests, failedTests);

    std::cout << "\nPhysics regression tests: " << passedTests << " passed, "
              << failedTests << " failed\n";
    return failedTests == 0 ? 0 : 1;
}
