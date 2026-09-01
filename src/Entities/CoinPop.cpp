#include "Entities/CoinPop.hpp"
#include <SFML/Graphics/RenderWindow.hpp>
#include <SFML/Graphics/Sprite.hpp>

namespace entity {

namespace {
    constexpr int kSourceTileSize = 16;
    constexpr float kFrameDuration = 0.08f;
}

CoinPop::CoinPop(const sf::Vector2f& blockPosition, float tileSize, const sf::Texture* texture, float initialSpeed, float gravity, float lifetime)
    : Entity({blockPosition.x, blockPosition.y - tileSize}),
      m_texture(texture),
      m_velocityY(initialSpeed),
      m_gravity(gravity),
      m_lifetime(lifetime),
      m_scale(tileSize / static_cast<float>(kSourceTileSize)) {}

void CoinPop::update(sf::Time dt) {
    update(dt.asSeconds());
}

void CoinPop::update(float deltaTime) {
    if (!m_alive) {
        return;
    }

    m_elapsed += deltaTime;
    m_position.y += m_velocityY * deltaTime;
    m_velocityY += m_gravity * deltaTime;

    if (m_elapsed >= m_lifetime) {
        m_alive = false;
    }
}

void CoinPop::render(sf::RenderWindow& window) const {
    if (!m_alive || !m_texture) {
        return;
    }

    sf::Sprite coinSprite(*m_texture);
    coinSprite.setScale({m_scale, m_scale});

    int frame = static_cast<int>(m_elapsed / kFrameDuration) % 4;
    coinSprite.setTextureRect(sf::IntRect(
        {frame * kSourceTileSize, 0},
        {kSourceTileSize, kSourceTileSize}
    ));
    coinSprite.setPosition(m_position);
    window.draw(coinSprite);
}

void CoinPop::renderWithTexture(sf::RenderWindow& window, const sf::Texture& texture, float scale) const {
    if (!m_alive) {
        return;
    }

    sf::Sprite coinSprite(texture);
    coinSprite.setScale({scale, scale});

    int frame = static_cast<int>(m_elapsed / kFrameDuration) % 4;
    coinSprite.setTextureRect(sf::IntRect(
        {frame * kSourceTileSize, 0},
        {kSourceTileSize, kSourceTileSize}
    ));
    coinSprite.setPosition(m_position);
    window.draw(coinSprite);
}

sf::FloatRect CoinPop::getBounds() const {
    return sf::FloatRect(
        m_position,
        {static_cast<float>(kSourceTileSize) * m_scale, static_cast<float>(kSourceTileSize) * m_scale}
    );
}

} // namespace entity
