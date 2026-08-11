#include "Player/Luigi.hpp"

namespace entity {

Luigi::Luigi(
    const sf::Vector2f& position,
    const sf::Vector2f& colliderSize,
    const sf::Vector2f& colliderOffset
)
    : Player(
          position,
          colliderSize,
          colliderOffset,
          PlayerMovementConfig{
              390.0f,  // moveSpeed (Slightly slower top speed)
              1150.0f, // jumpSpeed (Higher jump)
              1300.0f  // groundFriction (Lower friction -> longer slide momentum)
          }
      ) {}

} // namespace entity
