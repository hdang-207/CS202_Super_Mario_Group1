#pragma once

#include "Physics/AABB.hpp"

#include <algorithm>

namespace duel {

inline constexpr float kMaximumHealth = 100.f;
inline constexpr float kMaximumEnergy = 100.f;
inline constexpr float kStompDamage = 20.f;
inline constexpr float kFireballDamage = 10.f;
inline constexpr float kFireballEnergyCost = 20.f;
inline constexpr float kManaPickupRestore = 40.f;
inline constexpr float kStompEntryTolerance = 12.f;
inline constexpr float kMinimumRelativeStompSpeed = 80.f;

inline bool isValidStomp(
    const physics::AABB& previousAttacker,
    const physics::AABB& previousVictim,
    const physics::AABB& currentAttacker,
    const physics::AABB& currentVictim,
    float attackerVelocityY,
    float victimVelocityY,
    float victimDamageProtection
) {
    if (victimDamageProtection > 0.f
        || !currentAttacker.intersects(currentVictim)) {
        return false;
    }

    const bool enteredFromAbove = previousAttacker.bottom()
        <= previousVictim.top() + kStompEntryTolerance;
    const float relativeVerticalSpeed = attackerVelocityY - victimVelocityY;
    return enteredFromAbove
        && relativeVerticalSpeed >= kMinimumRelativeStompSpeed;
}

inline float healthAfterStomp(float currentHealth) {
    return std::clamp(currentHealth - kStompDamage, 0.f, kMaximumHealth);
}

inline bool canShootFireball(bool hasFirePower, float currentEnergy) {
    return hasFirePower && currentEnergy >= kFireballEnergyCost;
}

inline float energyAfterFireball(float currentEnergy) {
    return std::clamp(
        currentEnergy - kFireballEnergyCost,
        0.f,
        kMaximumEnergy
    );
}

inline float energyAfterManaPickup(float currentEnergy) {
    return std::clamp(
        currentEnergy + kManaPickupRestore,
        0.f,
        kMaximumEnergy
    );
}

inline float healthAfterFireball(float currentHealth) {
    return std::clamp(
        currentHealth - kFireballDamage,
        0.f,
        kMaximumHealth
    );
}

inline bool hasFallenBelow(const physics::AABB& playerBounds, float arenaHeight) {
    return playerBounds.top() > arenaHeight;
}

} // namespace duel
