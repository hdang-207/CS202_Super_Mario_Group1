#include "States/DuelPlayerCollision.hpp"

#include <cmath>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

namespace {

constexpr float kTestEpsilon = 0.001f;

struct TestContext {
    int failures{0};
    std::vector<std::string> diagnostics;

    void expectTrue(bool condition, const std::string& message) {
        if (!condition) {
            ++failures;
            diagnostics.push_back(message);
        }
    }

    void expectNear(float actual, float expected, const std::string& label) {
        if (std::abs(actual - expected) <= kTestEpsilon) {
            return;
        }

        ++failures;
        std::ostringstream message;
        message << label << ": expected " << expected << ", actual " << actual;
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

physics::PhysicsBody makeBody(sf::Vector2f position) {
    return {position, {10.f, 20.f}};
}

void expectTouching(TestContext& context,
                    const physics::PhysicsBody& left,
                    const physics::PhysicsBody& right) {
    context.expectNear(
        left.getAABB().right(),
        right.getAABB().left(),
        "resolved collider edges"
    );
}

void testLeftPlayerRunsIntoStationaryPlayer(TestContext& context) {
    physics::PhysicsBody left = makeBody({0.f, 0.f});
    physics::PhysicsBody right = makeBody({15.f, 0.f});
    const physics::AABB previousLeft = left.getAABB();
    const physics::AABB previousRight = right.getAABB();
    left.setPosition({8.f, 0.f});
    left.setVelocity({120.f, 30.f});

    context.expectTrue(
        duel::resolveHorizontalPlayerCollision(
            left, right, previousLeft, previousRight),
        "side overlap should resolve"
    );
    context.expectNear(left.getPosition().x, 5.f, "moving player rollback");
    context.expectNear(right.getPosition().x, 15.f, "stationary player position");
    context.expectNear(left.getVelocity().x, 0.f, "inward left velocity");
    expectTouching(context, left, right);
}

void testRightPlayerRunsIntoStationaryPlayer(TestContext& context) {
    physics::PhysicsBody left = makeBody({0.f, 0.f});
    physics::PhysicsBody right = makeBody({15.f, 0.f});
    const physics::AABB previousLeft = left.getAABB();
    const physics::AABB previousRight = right.getAABB();
    right.setPosition({7.f, 0.f});
    right.setVelocity({-120.f, -15.f});

    context.expectTrue(
        duel::resolveHorizontalPlayerCollision(
            right, left, previousRight, previousLeft),
        "right-to-left side overlap should resolve"
    );
    context.expectNear(left.getPosition().x, 0.f, "stationary left position");
    context.expectNear(right.getPosition().x, 10.f, "moving right rollback");
    context.expectNear(right.getVelocity().x, 0.f, "inward right velocity");
    expectTouching(context, left, right);
}

void testBothPlayersShareCorrection(TestContext& context) {
    physics::PhysicsBody left = makeBody({0.f, 0.f});
    physics::PhysicsBody right = makeBody({20.f, 0.f});
    const physics::AABB previousLeft = left.getAABB();
    const physics::AABB previousRight = right.getAABB();
    left.setPosition({8.f, 0.f});
    right.setPosition({14.f, 0.f});
    left.setVelocity({100.f, 0.f});
    right.setVelocity({-75.f, 0.f});

    context.expectTrue(
        duel::resolveHorizontalPlayerCollision(
            left, right, previousLeft, previousRight),
        "two closing players should resolve"
    );
    context.expectNear(left.getPosition().x, 5.714286f, "weighted left correction");
    context.expectNear(right.getPosition().x, 15.714286f, "weighted right correction");
    context.expectNear(left.getVelocity().x, 0.f, "left inward velocity");
    context.expectNear(right.getVelocity().x, 0.f, "right inward velocity");
    expectTouching(context, left, right);
}

void testOutwardVelocitiesArePreserved(TestContext& context) {
    physics::PhysicsBody left = makeBody({0.f, 0.f});
    physics::PhysicsBody right = makeBody({15.f, 0.f});
    const physics::AABB previousLeft = left.getAABB();
    const physics::AABB previousRight = right.getAABB();
    left.setPosition({8.f, 0.f});
    left.setVelocity({-40.f, 0.f});
    right.setVelocity({25.f, 0.f});

    context.expectTrue(
        duel::resolveHorizontalPlayerCollision(
            left, right, previousLeft, previousRight),
        "test setup should produce a side collision"
    );
    context.expectNear(left.getVelocity().x, -40.f, "outward left velocity");
    context.expectNear(right.getVelocity().x, 25.f, "outward right velocity");
}

void testExactEdgeTouchIsNoOp(TestContext& context) {
    physics::PhysicsBody left = makeBody({0.f, 0.f});
    physics::PhysicsBody right = makeBody({15.f, 0.f});
    const physics::AABB previousLeft = left.getAABB();
    const physics::AABB previousRight = right.getAABB();
    left.setPosition({5.f, 0.f});
    left.setVelocity({100.f, 0.f});

    context.expectTrue(
        !duel::resolveHorizontalPlayerCollision(
            left, right, previousLeft, previousRight),
        "exact edge contact should not resolve"
    );
    context.expectNear(left.getPosition().x, 5.f, "edge-touching position");
    context.expectNear(left.getVelocity().x, 100.f, "edge-touching velocity");
}

void testNoClosingMotionIsNoOp(TestContext& context) {
    physics::PhysicsBody left = makeBody({0.f, 0.f});
    physics::PhysicsBody right = makeBody({20.f, 0.f});
    const physics::AABB previousLeft = left.getAABB();
    const physics::AABB previousRight = right.getAABB();
    left.setPosition({2.f, 0.f});
    right.setPosition({23.f, 0.f});

    context.expectTrue(
        !duel::resolveHorizontalPlayerCollision(
            left, right, previousLeft, previousRight),
        "separated players without relative closing should be unchanged"
    );
    context.expectNear(left.getPosition().x, 2.f, "unchanged left position");
    context.expectNear(right.getPosition().x, 23.f, "unchanged right position");
}

void testPreviouslyOverlappingBodiesAreNotSideResolved(TestContext& context) {
    physics::PhysicsBody first = makeBody({0.f, 0.f});
    physics::PhysicsBody second = makeBody({8.f, 0.f});
    const physics::AABB previousFirst = first.getAABB();
    const physics::AABB previousSecond = second.getAABB();
    first.setPosition({3.f, 0.f});

    context.expectTrue(
        !duel::resolveHorizontalPlayerCollision(
            first, second, previousFirst, previousSecond),
        "an existing overlap has no reliable side-entry ordering"
    );
    context.expectNear(first.getPosition().x, 3.f, "existing-overlap position");
}

void testAirborneSideContactPreservesVerticalVelocity(TestContext& context) {
    physics::PhysicsBody left = makeBody({0.f, 20.f});
    physics::PhysicsBody right = makeBody({15.f, 25.f});
    const physics::AABB previousLeft = left.getAABB();
    const physics::AABB previousRight = right.getAABB();
    left.setPosition({8.f, 18.f});
    right.setPosition({15.f, 29.f});
    left.setVelocity({100.f, -240.f});
    right.setVelocity({0.f, 360.f});

    context.expectTrue(
        duel::resolveHorizontalPlayerCollision(
            left, right, previousLeft, previousRight),
        "airborne side contact should resolve horizontally"
    );
    context.expectNear(left.getVelocity().y, -240.f, "left vertical velocity");
    context.expectNear(right.getVelocity().y, 360.f, "right vertical velocity");
}

void testContactFromAboveIsNotResolved(TestContext& context) {
    physics::PhysicsBody upper = makeBody({0.f, 0.f});
    physics::PhysicsBody lower = makeBody({15.f, 25.f});
    const physics::AABB previousUpper = upper.getAABB();
    const physics::AABB previousLower = lower.getAABB();
    upper.setPosition({8.f, 6.f});

    context.expectTrue(
        !duel::resolveHorizontalPlayerCollision(
            upper, lower, previousUpper, previousLower),
        "later vertical entry should remain a vertical contact"
    );
    context.expectNear(upper.getPosition().x, 8.f, "vertical-contact X position");
}

void testDiagonalEntryTiePrefersVertical(TestContext& context) {
    physics::PhysicsBody upperLeft = makeBody({0.f, 0.f});
    physics::PhysicsBody lowerRight = makeBody({15.f, 25.f});
    const physics::AABB previousUpperLeft = upperLeft.getAABB();
    const physics::AABB previousLowerRight = lowerRight.getAABB();
    upperLeft.setPosition({8.f, 8.f});

    context.expectTrue(
        !duel::resolveHorizontalPlayerCollision(
            upperLeft, lowerRight, previousUpperLeft, previousLowerRight),
        "equal entry times should prefer the vertical axis"
    );
    context.expectNear(upperLeft.getPosition().x, 8.f, "corner-contact X position");
}

void testLongFrameCrossingKeepsPreviousOrder(TestContext& context) {
    physics::PhysicsBody left = makeBody({0.f, 0.f});
    physics::PhysicsBody right = makeBody({20.f, 0.f});
    const physics::AABB previousLeft = left.getAABB();
    const physics::AABB previousRight = right.getAABB();
    left.setPosition({35.f, 0.f});
    left.setVelocity({500.f, 0.f});

    context.expectTrue(
        duel::resolveHorizontalPlayerCollision(
            left, right, previousLeft, previousRight),
        "a fully crossed player should still resolve"
    );
    context.expectNear(left.getPosition().x, 10.f, "crossing rollback");
    expectTouching(context, left, right);
}

void testColliderOffsetIsRespected(TestContext& context) {
    physics::PhysicsBody left({0.f, 0.f}, {10.f, 20.f}, {2.f, 3.f});
    physics::PhysicsBody right({17.f, 0.f}, {10.f, 20.f}, {2.f, 3.f});
    const physics::AABB previousLeft = left.getAABB();
    const physics::AABB previousRight = right.getAABB();
    left.setPosition({10.f, 0.f});
    left.setVelocity({100.f, 0.f});

    context.expectTrue(
        duel::resolveHorizontalPlayerCollision(
            left, right, previousLeft, previousRight),
        "offset colliders should resolve by their AABBs"
    );
    context.expectNear(left.getPosition().x, 7.f, "offset body rollback");
    expectTouching(context, left, right);
}

} // namespace

int main() {
    int passedTests = 0;
    int failedTests = 0;
    runTest("left player into stationary player",
            testLeftPlayerRunsIntoStationaryPlayer, passedTests, failedTests);
    runTest("right player into stationary player",
            testRightPlayerRunsIntoStationaryPlayer, passedTests, failedTests);
    runTest("both players share correction",
            testBothPlayersShareCorrection, passedTests, failedTests);
    runTest("outward velocities are preserved",
            testOutwardVelocitiesArePreserved, passedTests, failedTests);
    runTest("exact edge touch is a no-op",
            testExactEdgeTouchIsNoOp, passedTests, failedTests);
    runTest("no closing motion is a no-op",
            testNoClosingMotionIsNoOp, passedTests, failedTests);
    runTest("existing overlap is not side-resolved",
            testPreviouslyOverlappingBodiesAreNotSideResolved,
            passedTests, failedTests);
    runTest("airborne contact preserves Y velocity",
            testAirborneSideContactPreservesVerticalVelocity,
            passedTests, failedTests);
    runTest("contact from above remains vertical",
            testContactFromAboveIsNotResolved, passedTests, failedTests);
    runTest("diagonal tie prefers vertical",
            testDiagonalEntryTiePrefersVertical, passedTests, failedTests);
    runTest("long-frame crossing keeps order",
            testLongFrameCrossingKeepsPreviousOrder, passedTests, failedTests);
    runTest("collider offsets are respected",
            testColliderOffsetIsRespected, passedTests, failedTests);

    std::cout << passedTests << " passed, " << failedTests << " failed.\n";
    return failedTests == 0 ? 0 : 1;
}
