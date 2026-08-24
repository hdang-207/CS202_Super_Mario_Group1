#pragma once

#include "Entities/Character.hpp"
#include <SFML/Graphics/Sprite.hpp>
#include <SFML/Graphics/Texture.hpp>

namespace physics {
class PhysicsSystem;
struct AABB;
}

namespace entity {

enum class GoombaType {
    Normal,
    Underground
};

class Goomba : public Character {
public:
    Goomba(const sf::Vector2f& position, float tileSize, const sf::Texture* texture = nullptr, GoombaType type = GoombaType::Normal, float initialSpeed = 72.f);
    virtual ~Goomba() override = default;

    void update(float deltaTime) override;
    void update(float deltaTime, physics::PhysicsSystem& physicsSystem, const std::vector<physics::AABB>& solids, float mapWidth, float mapHeight);

    void render(sf::RenderTarget& target) const override;
    void renderWithTexture(sf::RenderWindow& window, const sf::Texture& texture) const;

    void stomp();
    void defeat();

    [[nodiscard]] sf::FloatRect getBounds() const override {
        if (m_stomped) {
            return sf::FloatRect();
        }
        return Character::getBounds();
    }

    [[nodiscard]] bool isStomped() const noexcept { return m_stomped; }
    [[nodiscard]] GoombaType getType() const noexcept { return m_type; }
    [[nodiscard]] int getAnimationFrame() const noexcept { return m_animationFrame; }
    [[nodiscard]] float getTileSize() const noexcept { return m_tileSize; }

private:
    const sf::Texture* m_texture{nullptr};
    GoombaType m_type{GoombaType::Normal};
    float m_tileSize{48.f};
    float m_walkSpeed{72.f};
    float m_animElapsed{0.f};
    int m_animationFrame{0};
    bool m_stomped{false};
    float m_stompTimer{0.f};
};

} // namespace entity
