#include "Input/InputHandler.hpp"
#include "Input/PlayerKeyBindings.hpp"

#include <cmath>
#include <initializer_list>
#include <iostream>
#include <set>
#include <string>
#include <vector>

namespace {

using Scancode = sf::Keyboard::Scancode;
using HeldKeys = std::set<Scancode>;

constexpr float kTestEpsilon = 0.001f;

struct TestContext {
    int failures{0};
    std::vector<std::string> diagnostics;

    void expectTrue(bool condition, const std::string& message) {
        if (condition) {
            return;
        }

        ++failures;
        diagnostics.push_back(message);
    }

    void expectAxis(float actual, float expected, const std::string& label) {
        if (std::abs(actual - expected) <= kTestEpsilon) {
            return;
        }

        ++failures;
        diagnostics.push_back(
            label + ": expected " + std::to_string(expected)
            + ", actual " + std::to_string(actual)
        );
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

void update(
    InputHandler& handler,
    std::initializer_list<Scancode> keys
) {
    handler.update(HeldKeys(keys));
}

void testCampaignMovementBindings(TestContext& context) {
    InputHandler handler;

    update(handler, {Scancode::A});
    context.expectAxis(handler.getPlayerInput().moveAxis, -1.0f, "campaign A");

    update(handler, {Scancode::D});
    context.expectAxis(handler.getPlayerInput().moveAxis, 1.0f, "campaign D");

    update(handler, {Scancode::Left});
    context.expectAxis(handler.getPlayerInput().moveAxis, -1.0f, "campaign Left");

    update(handler, {Scancode::Right});
    context.expectAxis(handler.getPlayerInput().moveAxis, 1.0f, "campaign Right");
}

void testOpposingMovementCancels(TestContext& context) {
    InputHandler handler;

    update(handler, {Scancode::A, Scancode::D});
    context.expectAxis(
        handler.getPlayerInput().moveAxis,
        0.0f,
        "campaign A plus D"
    );

    update(handler, {Scancode::Left, Scancode::Right});
    context.expectAxis(
        handler.getPlayerInput().moveAxis,
        0.0f,
        "campaign Left plus Right"
    );
}

void testCampaignJumpAndCrouchBindings(TestContext& context) {
    InputHandler handler;

    for (const Scancode key : {
             Scancode::W,
             Scancode::Up,
             Scancode::Space
         }) {
        handler.reset();
        update(handler, {key});
        context.expectTrue(
            handler.getPlayerInput().jumpHeld,
            "campaign jump binding should set jumpHeld"
        );
    }

    for (const Scancode key : {Scancode::S, Scancode::Down}) {
        handler.reset();
        update(handler, {key});
        context.expectTrue(
            handler.getPlayerInput().crouchHeld,
            "campaign crouch binding should set crouchHeld"
        );
    }
}

void testJumpEdgeTrigger(TestContext& context) {
    InputHandler handler;

    update(handler, {Scancode::W});
    context.expectTrue(
        handler.getPlayerInput().jumpPressed,
        "jump should be pressed on the first held frame"
    );

    update(handler, {Scancode::W});
    context.expectTrue(
        !handler.getPlayerInput().jumpPressed,
        "jump should not repeat while held"
    );

    update(handler, {});
    context.expectTrue(
        !handler.getPlayerInput().jumpHeld,
        "jump should be released after an empty update"
    );

    update(handler, {Scancode::W});
    context.expectTrue(
        handler.getPlayerInput().jumpPressed,
        "jump should trigger again after release"
    );
}

void testCombatEdgeTriggers(TestContext& context) {
    InputHandler handler;

    update(handler, {Scancode::X, Scancode::C});
    context.expectTrue(
        handler.getPlayerInput().shootPressed,
        "shoot should trigger on the first held frame"
    );
    context.expectTrue(
        handler.getPlayerInput().bombPressed,
        "bomb should trigger on the first held frame"
    );

    update(handler, {Scancode::X, Scancode::C});
    context.expectTrue(
        !handler.getPlayerInput().shootPressed,
        "shoot should not repeat while held"
    );
    context.expectTrue(
        !handler.getPlayerInput().bombPressed,
        "bomb should not repeat while held"
    );

    update(handler, {});
    update(handler, {Scancode::X, Scancode::C});
    context.expectTrue(
        handler.getPlayerInput().shootPressed,
        "shoot should trigger again after release"
    );
    context.expectTrue(
        handler.getPlayerInput().bombPressed,
        "bomb should trigger again after release"
    );
}

void testDuelPlayerOneBindings(TestContext& context) {
    InputHandler handler(PlayerKeyBindings::duelPlayerOne());

    update(handler, {Scancode::A});
    context.expectAxis(handler.getPlayerInput().moveAxis, -1.0f, "player one A");
    update(handler, {Scancode::D});
    context.expectAxis(handler.getPlayerInput().moveAxis, 1.0f, "player one D");
    update(handler, {Scancode::W});
    context.expectTrue(handler.getPlayerInput().jumpHeld, "player one W jump");
    update(handler, {Scancode::S});
    context.expectTrue(handler.getPlayerInput().crouchHeld, "player one S crouch");

    update(handler, {Scancode::Left});
    context.expectAxis(handler.getPlayerInput().moveAxis, 0.0f, "player one ignores Left");
    update(handler, {Scancode::Right});
    context.expectAxis(handler.getPlayerInput().moveAxis, 0.0f, "player one ignores Right");
    update(handler, {Scancode::Up});
    context.expectTrue(!handler.getPlayerInput().jumpHeld, "player one ignores Up");
    update(handler, {Scancode::Down});
    context.expectTrue(!handler.getPlayerInput().crouchHeld, "player one ignores Down");
}

void testDuelPlayerTwoBindings(TestContext& context) {
    InputHandler handler(PlayerKeyBindings::duelPlayerTwo());

    update(handler, {Scancode::Left});
    context.expectAxis(handler.getPlayerInput().moveAxis, -1.0f, "player two Left");
    update(handler, {Scancode::Right});
    context.expectAxis(handler.getPlayerInput().moveAxis, 1.0f, "player two Right");
    update(handler, {Scancode::Up});
    context.expectTrue(handler.getPlayerInput().jumpHeld, "player two Up jump");
    update(handler, {Scancode::Down});
    context.expectTrue(handler.getPlayerInput().crouchHeld, "player two Down crouch");

    update(handler, {Scancode::A});
    context.expectAxis(handler.getPlayerInput().moveAxis, 0.0f, "player two ignores A");
    update(handler, {Scancode::D});
    context.expectAxis(handler.getPlayerInput().moveAxis, 0.0f, "player two ignores D");
    update(handler, {Scancode::W});
    context.expectTrue(!handler.getPlayerInput().jumpHeld, "player two ignores W");
    update(handler, {Scancode::S});
    context.expectTrue(!handler.getPlayerInput().crouchHeld, "player two ignores S");
}

void testHandlersHaveIndependentEdgeState(TestContext& context) {
    InputHandler first(PlayerKeyBindings::duelPlayerOne());
    InputHandler second(PlayerKeyBindings::duelPlayerOne());

    update(first, {Scancode::W});
    context.expectTrue(
        first.getPlayerInput().jumpPressed,
        "first handler should see its initial jump edge"
    );

    update(first, {Scancode::W});
    context.expectTrue(
        !first.getPlayerInput().jumpPressed,
        "first handler should consume only its own jump edge"
    );

    update(second, {Scancode::W});
    context.expectTrue(
        second.getPlayerInput().jumpPressed,
        "second handler should retain an independent jump edge"
    );
}

void testDuelShootingBindings(TestContext& context) {
    InputHandler playerOne(PlayerKeyBindings::duelPlayerOne());
    InputHandler playerTwo(PlayerKeyBindings::duelPlayerTwo());

    update(playerOne, {Scancode::F});
    context.expectTrue(
        playerOne.getPlayerInput().shootPressed,
        "player one F should shoot"
    );
    update(playerOne, {});
    update(playerOne, {Scancode::J});
    context.expectTrue(
        !playerOne.getPlayerInput().shootPressed,
        "player one should ignore player two shoot key"
    );

    update(playerTwo, {Scancode::J});
    context.expectTrue(
        playerTwo.getPlayerInput().shootPressed,
        "player two J should shoot"
    );
    update(playerTwo, {});
    update(playerTwo, {Scancode::F});
    context.expectTrue(
        !playerTwo.getPlayerInput().shootPressed,
        "player two should ignore player one shoot key"
    );
}

void testDuelBombBindings(TestContext& context) {
    InputHandler playerOne(PlayerKeyBindings::duelPlayerOne());
    InputHandler playerTwo(PlayerKeyBindings::duelPlayerTwo());

    update(playerOne, {Scancode::G});
    context.expectTrue(
        playerOne.getPlayerInput().bombPressed,
        "player one G should throw a bomb"
    );
    update(playerTwo, {Scancode::K});
    context.expectTrue(
        playerTwo.getPlayerInput().bombPressed,
        "player two K should throw a bomb"
    );

    update(playerOne, {});
    update(playerTwo, {});
    update(playerOne, {Scancode::K});
    update(playerTwo, {Scancode::G});
    context.expectTrue(
        !playerOne.getPlayerInput().bombPressed,
        "player one should ignore player two bomb key"
    );
    context.expectTrue(
        !playerTwo.getPlayerInput().bombPressed,
        "player two should ignore player one bomb key"
    );

    update(playerOne, {});
    update(playerTwo, {});
    update(playerOne, {Scancode::C});
    update(playerTwo, {Scancode::C});
    context.expectTrue(
        !playerOne.getPlayerInput().bombPressed,
        "the campaign bomb key should stay out of the duel bindings"
    );
    context.expectTrue(
        !playerTwo.getPlayerInput().bombPressed,
        "the campaign bomb key should stay out of the duel bindings"
    );
}

void testResetClearsInputAndEdgeHistory(TestContext& context) {
    InputHandler handler;
    update(handler, {
        Scancode::A,
        Scancode::W,
        Scancode::S,
        Scancode::X,
        Scancode::C
    });

    handler.reset();
    const PlayerInput& resetInput = handler.getPlayerInput();
    context.expectAxis(resetInput.moveAxis, 0.0f, "reset move axis");
    context.expectTrue(!resetInput.jumpHeld, "reset should clear jumpHeld");
    context.expectTrue(!resetInput.jumpPressed, "reset should clear jumpPressed");
    context.expectTrue(!resetInput.crouchHeld, "reset should clear crouchHeld");
    context.expectTrue(!resetInput.shootPressed, "reset should clear shootPressed");
    context.expectTrue(!resetInput.bombPressed, "reset should clear bombPressed");

    update(handler, {Scancode::W, Scancode::X, Scancode::C});
    const PlayerInput& afterReset = handler.getPlayerInput();
    context.expectTrue(afterReset.jumpPressed, "reset should clear jump edge history");
    context.expectTrue(afterReset.shootPressed, "reset should clear shoot edge history");
    context.expectTrue(afterReset.bombPressed, "reset should clear bomb edge history");
}

} // namespace

int main() {
    int passedTests = 0;
    int failedTests = 0;

    runTest(
        "campaign movement bindings",
        testCampaignMovementBindings,
        passedTests,
        failedTests
    );
    runTest(
        "opposing movement cancels",
        testOpposingMovementCancels,
        passedTests,
        failedTests
    );
    runTest(
        "campaign jump and crouch bindings",
        testCampaignJumpAndCrouchBindings,
        passedTests,
        failedTests
    );
    runTest("jump edge trigger", testJumpEdgeTrigger, passedTests, failedTests);
    runTest("combat edge triggers", testCombatEdgeTriggers, passedTests, failedTests);
    runTest(
        "duel player one bindings",
        testDuelPlayerOneBindings,
        passedTests,
        failedTests
    );
    runTest(
        "duel player two bindings",
        testDuelPlayerTwoBindings,
        passedTests,
        failedTests
    );
    runTest(
        "handlers have independent edge state",
        testHandlersHaveIndependentEdgeState,
        passedTests,
        failedTests
    );
    runTest(
        "duel shooting bindings",
        testDuelShootingBindings,
        passedTests,
        failedTests
    );
    runTest(
        "duel bomb bindings",
        testDuelBombBindings,
        passedTests,
        failedTests
    );
    runTest(
        "reset clears input and edge history",
        testResetClearsInputAndEdgeHistory,
        passedTests,
        failedTests
    );

    std::cout << "\nInputHandler tests: " << passedTests << " passed, "
              << failedTests << " failed\n";
    return failedTests == 0 ? 0 : 1;
}
