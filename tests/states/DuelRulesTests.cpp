#include "States/DuelRules.hpp"

#include <cmath>
#include <iostream>
#include <string>

namespace {

int failures = 0;

void expect(bool condition, const std::string& message) {
    if (!condition) {
        ++failures;
        std::cout << "[FAIL] " << message << '\n';
    }
}

void testStompFromAbove() {
    const physics::AABB previousAttacker({100.f, 40.f}, {32.f, 46.f});
    const physics::AABB previousVictim({100.f, 90.f}, {32.f, 46.f});
    const physics::AABB currentAttacker({100.f, 52.f}, {32.f, 46.f});
    const physics::AABB currentVictim({100.f, 90.f}, {32.f, 46.f});
    expect(
        duel::isValidStomp(
            previousAttacker,
            previousVictim,
            currentAttacker,
            currentVictim,
            500.f,
            0.f,
            0.f
        ),
        "falling overlap from above should count as a stomp"
    );
}

void testSideContactDoesNotStomp() {
    const physics::AABB previousAttacker({70.f, 100.f}, {32.f, 46.f});
    const physics::AABB previousVictim({100.f, 100.f}, {32.f, 46.f});
    const physics::AABB currentAttacker({82.f, 100.f}, {32.f, 46.f});
    const physics::AABB currentVictim({100.f, 100.f}, {32.f, 46.f});
    expect(
        !duel::isValidStomp(
            previousAttacker,
            previousVictim,
            currentAttacker,
            currentVictim,
            400.f,
            0.f,
            0.f
        ),
        "side contact should not count as a stomp"
    );
}

void testProtectionPreventsRepeatDamage() {
    const physics::AABB previousAttacker({100.f, 40.f}, {32.f, 46.f});
    const physics::AABB previousVictim({100.f, 90.f}, {32.f, 46.f});
    const physics::AABB currentAttacker({100.f, 52.f}, {32.f, 46.f});
    const physics::AABB currentVictim({100.f, 90.f}, {32.f, 46.f});
    expect(
        !duel::isValidStomp(
            previousAttacker,
            previousVictim,
            currentAttacker,
            currentVictim,
            500.f,
            0.f,
            0.5f
        ),
        "damage protection should reject repeated stomp damage"
    );
}

void testDamageAndFallRules() {
    expect(
        std::abs(duel::healthAfterStomp(100.f) - 80.f) < 0.001f,
        "one stomp should remove exactly 20 health"
    );
    expect(
        std::abs(duel::healthAfterStomp(10.f)) < 0.001f,
        "health should clamp at zero"
    );
    expect(
        !duel::hasFallenBelow({{10.f, 720.f}, {32.f, 46.f}}, 720.f),
        "touching the lower boundary should not lose yet"
    );
    expect(
        duel::hasFallenBelow({{10.f, 721.f}, {32.f, 46.f}}, 720.f),
        "moving fully below the arena should lose"
    );
}

void testFireballEnergyAndDamage() {
    expect(
        !duel::canShootFireball(false, 100.f),
        "a player without Fire form should not shoot"
    );
    expect(
        duel::canShootFireball(true, 20.f),
        "exactly 20 energy should permit one shot"
    );
    expect(
        !duel::canShootFireball(true, 19.f),
        "less than 20 energy should reject a shot"
    );
    expect(
        std::abs(duel::energyAfterFireball(100.f) - 80.f) < 0.001f,
        "one fireball should consume exactly 20 energy"
    );
    expect(
        std::abs(duel::healthAfterFireball(100.f) - 90.f) < 0.001f,
        "one fireball hit should remove exactly 10 health"
    );
}

void testManaPickupRestoresEnergy() {
    expect(
        std::abs(duel::energyAfterManaPickup(20.f) - 60.f) < 0.001f,
        "one mana pickup should restore exactly 40 energy"
    );
    expect(
        std::abs(duel::energyAfterManaPickup(80.f) - 100.f) < 0.001f,
        "mana pickup energy should clamp at the maximum"
    );
}

} // namespace

int main() {
    testStompFromAbove();
    testSideContactDoesNotStomp();
    testProtectionPreventsRepeatDamage();
    testDamageAndFallRules();
    testFireballEnergyAndDamage();
    testManaPickupRestoresEnergy();

    if (failures == 0) {
        std::cout << "All Duel rules tests passed.\n";
        return 0;
    }
    std::cout << failures << " Duel rules assertion(s) failed.\n";
    return 1;
}
