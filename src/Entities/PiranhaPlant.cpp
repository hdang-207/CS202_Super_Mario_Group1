#include "Entities/PiranhaPlant.hpp"
#include <SFML/Graphics/RenderWindow.hpp>
#include <cmath>

namespace entity {

namespace {
    constexpr float kPiranhaVisibleDuration = 1.f;
    constexpr float kPiranhaMoveDuration = 0.55f;
    constexpr float kPiranhaHiddenDuration = 1.f;
}

PiranhaPlant::PiranhaPlant(const sf::Vector2f& basePosition, float pipeTopY, float scale)
    : Entity(basePosition),
      m_pipeTopY(pipeTopY),
      m_scale(scale) {
    m_position.y = m_pipeTopY - 23.f * m_scale;
}

void PiranhaPlant::update(sf::Time dt) {
    update(dt.asSeconds());
}

void PiranhaPlant::update(float deltaTime) {
    if (!m_alive) {
        return;
    }

    const float cycleDuration = kPiranhaVisibleDuration
        + kPiranhaMoveDuration + kPiranhaHiddenDuration
        + kPiranhaMoveDuration;

    m_elapsed = std::fmod(m_elapsed + deltaTime, cycleDuration);

    if (m_elapsed < kPiranhaVisibleDuration) {
        m_exposure = 1.f;
    } else if (m_elapsed < kPiranhaVisibleDuration + kPiranhaMoveDuration) {
        const float progress = (m_elapsed - kPiranhaVisibleDuration) / kPiranhaMoveDuration;
        m_exposure = 1.f - progress;
    } else if (m_elapsed < kPiranhaVisibleDuration + kPiranhaMoveDuration + kPiranhaHiddenDuration) {
        m_exposure = 0.f;
    } else {
        const float progress = (m_elapsed - kPiranhaVisibleDuration - kPiranhaMoveDuration - kPiranhaHiddenDuration) / kPiranhaMoveDuration;
        m_exposure = progress;
    }

    m_position.y = m_pipeTopY - m_exposure * 23.f * m_scale;
}

void PiranhaPlant::render(sf::RenderWindow& /*window*/) const {
    // Default render
}

void PiranhaPlant::renderWithTexture(sf::RenderWindow& window, const sf::Texture& texture) const {
    if (!m_alive) {
        return;
    }

    sf::Sprite sprite(texture);
    sprite.setScale({m_scale, m_scale});
    sprite.setPosition(m_position);
    window.draw(sprite);
}

sf::FloatRect PiranhaPlant::getBounds() const {
    return sf::FloatRect(
        {m_position.x + m_scale, m_position.y + m_scale},
        {16.f * m_scale, 23.f * m_scale}
    );
}

} // namespace entity
