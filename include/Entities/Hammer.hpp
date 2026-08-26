#pragma once

#include <SFML/Graphics.hpp>

namespace entity {

/**
 * @class Hammer
 * @brief One hammer in flight, thrown up and forward by a Hammer Bro.
 *
 * It is deliberately not an Entity: hammers ignore the level's blocks, cannot
 * be stomped and only ever hurt the avatar, so the level owns them beside the
 * fireballs instead of handing them to the entity manager.
 */
class Hammer {
public:
    Hammer(const sf::Texture& texture, sf::Vector2f position, sf::Vector2f velocity,
           float scale);

    /// @brief Advances the arc and the spin. @p gravity is the level's own.
    void update(sf::Time dt, float gravity);

    void draw(sf::RenderTarget& target) const;

    [[nodiscard]] sf::FloatRect bounds() const;
    [[nodiscard]] sf::Vector2f position() const noexcept { return m_position; }

private:
    static constexpr int kSourceSize = 16;
    static constexpr int kFrameCount = 4;

    const sf::Texture* m_texture;
    sf::Vector2f m_position;
    sf::Vector2f m_velocity;
    float m_scale;
    float m_animElapsed{0.f};
    int m_frame{0};
};

} // namespace entity
