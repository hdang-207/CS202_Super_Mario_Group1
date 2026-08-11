#pragma once

#include <SFML/Graphics.hpp>

namespace combat {

class Bomb {
public:
    static constexpr float kSize = 12.f;

    Bomb(sf::Vector2f position, bool facingRight);

    void update(float deltaTime);
    void render(sf::RenderTarget& target) const;

    [[nodiscard]] sf::FloatRect getBounds() const;
    [[nodiscard]] const sf::Vector2f& getPosition() const noexcept;
    [[nodiscard]] bool isActive() const noexcept;
    [[nodiscard]] bool fuseExpired() const noexcept;
    void deactivate() noexcept;

private:
    sf::Vector2f m_position;
    sf::Vector2f m_velocity;
    float m_fuseRemaining{1.5f};
    bool m_active{true};
};

} // namespace combat
