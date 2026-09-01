#include "Player/Mario.hpp"

namespace entity {

Mario::Mario(
    const sf::Vector2f& position,
    const sf::Vector2f& colliderSize,
    const sf::Vector2f& colliderOffset
)
    : Player(
          position,
          colliderSize,
          colliderOffset,
          PlayerMovementConfig{
              420.0f,  // moveSpeed
              1000.0f, // jumpSpeed
              2000.0f, // groundFriction
              1800.0f, // walkAcceleration
              0.02f,   // airFrictionMultiplier
              0.45f    // jumpCutoff
          }
      ) {}

} // namespace entity
