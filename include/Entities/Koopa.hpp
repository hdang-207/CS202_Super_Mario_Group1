#pragma once

#include "Entities/Character.hpp"
#include <SFML/Graphics/Sprite.hpp>
#include <SFML/Graphics/Texture.hpp>

namespace physics {
class PhysicsSystem;
struct AABB;
}

namespace entity {

enum class KoopaKind {
    Green,
    BlueUnderground
};

enum class KoopaState {
    Walking,
    ShellIdle,
    ShellMoving
};

class Koopa : public Character {
public:
    Koopa(const sf::Vector2f& position, float tileSize, const sf::Texture* walkingTexture = nullptr, const sf::Texture* shellTexture = nullptr, KoopaKind kind = KoopaKind::Green, float initialSpeed = 60.f);
    virtual ~Koopa() override = default;

    void update(float deltaTime) override;
    void update(float deltaTime, physics::PhysicsSystem& physicsSystem, const std::vector<physics::AABB>& solids, float mapWidth, float mapHeight);

    void render(sf::RenderTarget& target) const override;
    void renderWithTexture(sf::RenderWindow& window, const sf::Texture& walkingTexture, const sf::Texture& shellTexture) const;

    void stomp();
    void kick(bool directionRight, float kickSpeed = 480.f);
    void defeat();

    [[nodiscard]] bool isShell() const noexcept {
        return m_state == KoopaState::ShellIdle || m_state == KoopaState::ShellMoving;
    }
    [[nodiscard]] KoopaState getState() const noexcept { return m_state; }
    [[nodiscard]] KoopaKind getKind() const noexcept { return m_kind; }
    [[nodiscard]] int getAnimationFrame() const noexcept { return m_animationFrame; }
    [[nodiscard]] float getTileSize() const noexcept { return m_tileSize; }

protected:
    const sf::Texture* m_walkingTexture{nullptr};
    const sf::Texture* m_shellTexture{nullptr};
    KoopaKind m_kind{KoopaKind::Green};
    KoopaState m_state{KoopaState::Walking};
    float m_tileSize{48.f};
    float m_walkSpeed{60.f};
    float m_shellSpeed{480.f};
    float m_animElapsed{0.f};
    int m_animationFrame{0};
    bool m_facingRight{false};
};

} // namespace entity
