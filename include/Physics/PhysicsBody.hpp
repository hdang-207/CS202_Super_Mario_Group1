#pragma once

#include <SFML/System/Vector2.hpp>

#include "AABB.hpp"

namespace physics {

class PhysicsBody {
public:
    PhysicsBody() = default;

    PhysicsBody(
        const sf::Vector2f& position,
        const sf::Vector2f& colliderSize,
        const sf::Vector2f& colliderOffset = {0.0f, 0.0f}
    );

    // Được gọi ở đầu mỗi physics step.
    // Chỉ reset kết quả collision tạm thời.
    void beginPhysicsStep();

    void setPosition(const sf::Vector2f& position);

    [[nodiscard]]
    const sf::Vector2f& getPosition() const noexcept;

    void setVelocity(const sf::Vector2f& velocity);

    [[nodiscard]]
    const sf::Vector2f& getVelocity() const noexcept;

    void addVelocity(const sf::Vector2f& amount);

    void setAcceleration(const sf::Vector2f& acceleration);

    [[nodiscard]]
    const sf::Vector2f& getAcceleration() const noexcept;

    void addAcceleration(const sf::Vector2f& amount);

    void clearAcceleration();

    void setCollider(
        const sf::Vector2f& size,
        const sf::Vector2f& offset = {0.0f, 0.0f}
    );

    [[nodiscard]]
    const sf::Vector2f& getColliderSize() const noexcept;

    [[nodiscard]]
    const sf::Vector2f& getColliderOffset() const noexcept;

    // Trả về collider tại vị trí thật trong world.
    [[nodiscard]]
    AABB getAABB() const noexcept;

    void setGrounded(bool grounded);

    [[nodiscard]]
    bool isGrounded() const noexcept;

    void setHitCeiling(bool hitCeiling);

    [[nodiscard]]
    bool hitCeiling() const noexcept;

    void setHitWallLeft(bool hitWallLeft);

    [[nodiscard]]
    bool hitWallLeft() const noexcept;

    void setHitWallRight(bool hitWallRight);

    [[nodiscard]]
    bool hitWallRight() const noexcept;

private:
    sf::Vector2f m_position{0.0f, 0.0f};

    sf::Vector2f m_velocity{0.0f, 0.0f};


    sf::Vector2f m_acceleration{0.0f, 0.0f};

    // Kích thước vùng collision.
    sf::Vector2f m_colliderSize{0.0f, 0.0f};

    // Độ lệch giữa position của entity và collider.
    sf::Vector2f m_colliderOffset{0.0f, 0.0f};

    // Kết quả collision của physics step hiện tại.
    bool m_grounded = false;
    bool m_hitCeiling = false;
    bool m_hitWallLeft = false;
    bool m_hitWallRight = false;
};

} // namespace physics