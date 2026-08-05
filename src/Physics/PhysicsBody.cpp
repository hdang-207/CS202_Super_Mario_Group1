#include "Physics/PhysicsBody.hpp"

namespace physics {

PhysicsBody::PhysicsBody(
    const sf::Vector2f& position,
    const sf::Vector2f& colliderSize,
    const sf::Vector2f& colliderOffset
)
    : m_position(position),
      m_colliderSize(colliderSize),
      m_colliderOffset(colliderOffset) {}


void PhysicsBody::beginPhysicsStep() {
    m_grounded = false;
    m_hitCeiling = false;
    m_hitWallLeft = false;
    m_hitWallRight = false;
}

void PhysicsBody::setPosition(const sf::Vector2f& position) {
    m_position = position;

}

const sf::Vector2f& PhysicsBody::getPosition() const noexcept {
    return m_position;
}

void PhysicsBody::setVelocity(const sf::Vector2f& velocity) {
    m_velocity = velocity;
}

void PhysicsBody::addVelocity(const sf::Vector2f& amount) {
    m_velocity += amount;
}

const sf::Vector2f& PhysicsBody::getVelocity() const noexcept{
    return m_velocity;
}

void PhysicsBody::setAcceleration(const sf::Vector2f& acceleration) {
    m_acceleration = acceleration;
}

const sf::Vector2f& PhysicsBody::getAcceleration() const noexcept {
    return m_acceleration;
}

void PhysicsBody::addAcceleration(const sf::Vector2f& amount) {
    m_acceleration += amount;
}

void PhysicsBody::clearAcceleration() {
    m_acceleration = {0.0f, 0.0f};
}

void PhysicsBody::setCollider(const sf::Vector2f& size, const sf::Vector2f& offset) {
    m_colliderSize = size;
    m_colliderOffset = offset;
}

const sf::Vector2f& PhysicsBody::getColliderSize() const noexcept {
    return m_colliderSize;
}

const sf::Vector2f& PhysicsBody::getColliderOffset() const noexcept {
    return m_colliderOffset;
}

AABB PhysicsBody::getAABB() const noexcept {
    return AABB(m_position + m_colliderOffset, m_colliderSize);
}

void PhysicsBody::setGrounded(bool grounded) {
    m_grounded = grounded;
}

bool PhysicsBody::isGrounded() const noexcept {
    return m_grounded;
}

bool PhysicsBody::hitCeiling() const noexcept {
    return m_hitCeiling;
}

void PhysicsBody::setHitCeiling(bool hitCeiling) {
    m_hitCeiling = hitCeiling;
}

void PhysicsBody::setHitWallLeft(bool hitWallLeft) {
    m_hitWallLeft = hitWallLeft;
}

bool PhysicsBody::hitWallLeft() const noexcept {
    return m_hitWallLeft;
}

void PhysicsBody::setHitWallRight(bool hitWallRight) {
    m_hitWallRight = hitWallRight;
}

bool PhysicsBody::hitWallRight() const noexcept {
    return m_hitWallRight;
}

} // namespace physics