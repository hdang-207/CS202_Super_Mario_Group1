#include "Entities/PiranhaPlant.hpp"
#include <SFML/Graphics/RenderWindow.hpp>
#include <SFML/Graphics/Sprite.hpp>
#include <cmath>

namespace entity {

namespace {
    constexpr int kPiranhaFrameWidth = 16;
    constexpr int kPiranhaFrameHeight = 24;
    constexpr int kPiranhaFrameCount = 2;
    constexpr float kPiranhaFrameDuration = 0.22f;
    constexpr float kPiranhaVisibleDuration = 2.0f;
    constexpr float kPiranhaMoveDuration = 0.6f;
    constexpr float kPiranhaHiddenDuration = 1.5f;
    constexpr float kPiranhaCycleDuration =
        kPiranhaVisibleDuration + kPiranhaMoveDuration + kPiranhaHiddenDuration + kPiranhaMoveDuration;
}

PiranhaPlant::PiranhaPlant(const sf::Vector2f& position, float pipeTopY, const sf::Texture* texture, float scale)
    : Entity(position),
      m_texture(texture),
      m_basePosition(position),
      m_pipeTopY(pipeTopY),
      m_scale(scale) {}

void PiranhaPlant::update(sf::Time dt) {
    update(dt.asSeconds());
}

void PiranhaPlant::update(float deltaTime) {
    if (!m_alive) {
        return;
    }

    m_animElapsed += deltaTime;
    while (m_animElapsed >= kPiranhaFrameDuration) {
        m_animElapsed -= kPiranhaFrameDuration;
        m_animationFrame = (m_animationFrame + 1) % kPiranhaFrameCount;
    }

    m_elapsed = std::fmod(m_elapsed + deltaTime, kPiranhaCycleDuration);
    if (m_elapsed < kPiranhaVisibleDuration) {
        m_exposure = 1.f;
    } else if (m_elapsed < kPiranhaVisibleDuration + kPiranhaMoveDuration) {
        const float progress = (m_elapsed - kPiranhaVisibleDuration) / kPiranhaMoveDuration;
        m_exposure = 1.f - progress;
    } else if (m_elapsed < kPiranhaVisibleDuration + kPiranhaMoveDuration + kPiranhaHiddenDuration) {
        m_exposure = 0.f;
    } else {
        const float progress = (m_elapsed - (kPiranhaVisibleDuration + kPiranhaMoveDuration + kPiranhaHiddenDuration)) / kPiranhaMoveDuration;
        m_exposure = progress;
    }

    m_position.y = m_pipeTopY - m_exposure * 23.f * m_scale;
}

void PiranhaPlant::render(sf::RenderWindow& window) const {
    if (!m_alive || !m_texture) {
        return;
    }

    sf::Sprite sprite(*m_texture);
    sprite.setTextureRect(sf::IntRect(
        {m_animationFrame * kPiranhaFrameWidth, 0},
        {kPiranhaFrameWidth, kPiranhaFrameHeight}
    ));
    sprite.setScale({m_scale, m_scale});
    sprite.setPosition(m_position);
    window.draw(sprite);
}

void PiranhaPlant::renderWithTexture(sf::RenderWindow& window, const sf::Texture& texture, float scale) const {
    if (!m_alive) {
        return;
    }

    sf::Sprite sprite(texture);
    sprite.setTextureRect(sf::IntRect(
        {m_animationFrame * kPiranhaFrameWidth, 0},
        {kPiranhaFrameWidth, kPiranhaFrameHeight}
    ));
    sprite.setScale({scale, scale});
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
