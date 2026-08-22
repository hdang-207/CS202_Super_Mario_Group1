#pragma once

#include "Entities/Entity.hpp"
#include <SFML/Graphics/Texture.hpp>

namespace entity {

class CoinPop : public Entity {
public:
    CoinPop(const sf::Vector2f& blockPosition, float tileSize, const sf::Texture* texture = nullptr, float initialSpeed = -350.f, float gravity = 1200.f, float lifetime = 0.5f);
    virtual ~CoinPop() override = default;

    void update(sf::Time dt) override;
    void update(float deltaTime);

    void render(sf::RenderWindow& window) const override;
    void renderWithTexture(sf::RenderWindow& window, const sf::Texture& texture, float scale) const;

    [[nodiscard]] sf::FloatRect getBounds() const override;

private:
    const sf::Texture* m_texture{nullptr};
    float m_velocityY{-350.f};
    float m_gravity{1200.f};
    float m_elapsed{0.f};
    float m_lifetime{0.5f};
    float m_scale{3.f};
};

} // namespace entity
