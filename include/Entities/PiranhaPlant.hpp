#pragma once

#include "Entities/Entity.hpp"
#include <SFML/Graphics/Texture.hpp>

namespace entity {

class PiranhaPlant : public Entity {
public:
    PiranhaPlant(const sf::Vector2f& position, float pipeTopY, const sf::Texture* texture = nullptr, float scale = 3.f);
    virtual ~PiranhaPlant() override = default;

    void update(sf::Time dt) override;
    void update(float deltaTime);

    void render(sf::RenderWindow& window) const override;
    void renderWithTexture(sf::RenderWindow& window, const sf::Texture& texture, float scale) const;

    [[nodiscard]] sf::FloatRect getBounds() const override;
    [[nodiscard]] float getExposure() const noexcept { return m_exposure; }
    [[nodiscard]] bool isHazardous() const noexcept { return m_exposure >= 0.65f && m_alive; }

private:
    const sf::Texture* m_texture{nullptr};
    sf::Vector2f m_basePosition;
    float m_pipeTopY{0.f};
    float m_scale{3.f};
    float m_elapsed{0.f};
    float m_exposure{1.f};
    int m_animationFrame{0};
    float m_animElapsed{0.f};
};

} // namespace entity
