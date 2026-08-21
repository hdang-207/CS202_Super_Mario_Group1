#pragma once

#include "Entities/Entity.hpp"
#include <SFML/Graphics/Sprite.hpp>
#include <SFML/Graphics/Texture.hpp>

namespace entity {

class PiranhaPlant : public Entity {
public:
    PiranhaPlant(const sf::Vector2f& basePosition, float pipeTopY, float scale = 3.f);
    virtual ~PiranhaPlant() override = default;

    void update(sf::Time dt) override;
    void update(float deltaTime);

    void render(sf::RenderWindow& window) const override;
    void renderWithTexture(sf::RenderWindow& window, const sf::Texture& texture) const;

    [[nodiscard]] sf::FloatRect getBounds() const override;
    [[nodiscard]] float getExposure() const noexcept { return m_exposure; }
    [[nodiscard]] bool isDangerous() const noexcept { return m_alive && m_exposure >= 0.65f; }

private:
    float m_pipeTopY;
    float m_scale;
    float m_elapsed{0.f};
    float m_exposure{1.f};
};

} // namespace entity
