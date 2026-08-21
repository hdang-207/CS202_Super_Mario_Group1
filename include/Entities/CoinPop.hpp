#pragma once

#include "Entities/Entity.hpp"
#include <SFML/Graphics/Sprite.hpp>
#include <SFML/Graphics/Texture.hpp>

namespace entity {

class CoinPop : public Entity {
public:
    explicit CoinPop(const sf::Vector2f& blockPosition, float tileSize, float initialSpeed = -480.f, float gravity = 1400.f, float lifetime = 0.7f);
    virtual ~CoinPop() override = default;

    void update(sf::Time dt) override;
    void update(float deltaTime);

    void render(sf::RenderWindow& window) const override;
    void renderWithTexture(sf::RenderWindow& window, const sf::Texture& texture, float scale) const;

    [[nodiscard]] sf::FloatRect getBounds() const override;
    [[nodiscard]] bool isExpired() const noexcept { return m_elapsed >= m_lifetime; }

private:
    float m_velocityY{-480.f};
    float m_gravity{1400.f};
    float m_lifetime{0.7f};
    float m_elapsed{0.f};
    float m_scale{3.f};
};

} // namespace entity
