#pragma once

#include <SFML/Graphics.hpp>

namespace entity {

class Bullet {
public:
    static constexpr float kSize = 8.f;

    Bullet(const sf::Texture& texture, sf::Vector2f position,
           sf::Vector2f velocity, float lifetime);

    void update(sf::Time dt);
    void draw(sf::RenderTarget& target) const;
    void deactivate() noexcept;

    [[nodiscard]] sf::FloatRect bounds() const;
    [[nodiscard]] sf::Vector2f position() const noexcept;
    [[nodiscard]] bool isActive() const noexcept;

private:
    sf::Vector2f m_position;
    sf::Vector2f m_velocity;
    sf::Sprite m_sprite;
    float m_remainingLifetime;
    bool m_active{true};
};

} // namespace entity
