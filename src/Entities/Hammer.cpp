#include "Entities/Hammer.hpp"

namespace entity {

namespace {
    constexpr float kSpinFrameDuration = 0.05f;
    /// The drawn hammer is smaller than its cell; the hit box follows the head.
    constexpr float kHitInsetTiles = 0.25f;
}

Hammer::Hammer(const sf::Texture& texture, sf::Vector2f position, sf::Vector2f velocity,
               float scale)
    : m_texture(&texture), m_position(position), m_velocity(velocity), m_scale(scale) {}

void Hammer::update(sf::Time dt, float gravity) {
    const float seconds = dt.asSeconds();
    m_velocity.y += gravity * seconds;
    m_position += m_velocity * seconds;

    m_animElapsed += seconds;
    while (m_animElapsed >= kSpinFrameDuration) {
        m_animElapsed -= kSpinFrameDuration;
        m_frame = (m_frame + 1) % kFrameCount;
    }
}

void Hammer::draw(sf::RenderTarget& target) const {
    sf::Sprite sprite(*m_texture);
    sprite.setTextureRect(sf::IntRect({m_frame * kSourceSize, 0}, {kSourceSize, kSourceSize}));
    sprite.setScale({m_scale, m_scale});
    sprite.setPosition(m_position);
    target.draw(sprite);
}

sf::FloatRect Hammer::bounds() const {
    const float cell = kSourceSize * m_scale;
    const float inset = cell * kHitInsetTiles;
    return sf::FloatRect({m_position.x + inset, m_position.y + inset},
                         {cell - inset * 2.f, cell - inset * 2.f});
}

} // namespace entity
