#include "Systems/CameraSystem.hpp"
#include <iostream>

namespace Systems {

CameraSystem::CameraSystem()
    : m_view(sf::FloatRect({0.f, 0.f}, {Config::kViewWidth, Config::kViewHeight})),
      m_maxCameraCenterX(Config::kViewWidth / 2.f),
      m_freeLook(false),
      m_freeLookCentre(m_view.getCenter()) {}

CameraSystem::CameraSystem(const sf::FloatRect& viewRect)
    : m_view(viewRect),
      m_maxCameraCenterX(viewRect.size.x / 2.f),
      m_freeLook(false),
      m_freeLookCentre(m_view.getCenter()) {}

void CameraSystem::reset(sf::Vector2f initialCenter) {
    m_freeLook = false;
    m_maxCameraCenterX = initialCenter.x;
    m_view.setCenter(initialCenter);
    m_freeLookCentre = initialCenter;
}

void CameraSystem::centreCamera(sf::Vector2f target, float mapWidth, float mapHeight) {
    const float halfWidth = Config::kViewWidth / 2.f;
    const float halfHeight = Config::kViewHeight / 2.f;
    const float maxCenterX = std::max(halfWidth, mapWidth - halfWidth);
    const float maxCenterY = std::max(halfHeight, mapHeight - halfHeight);

    m_view.setCenter({
        std::clamp(target.x, halfWidth, maxCenterX),
        std::clamp(target.y, halfHeight, maxCenterY)
    });
}

void CameraSystem::followTarget(sf::Vector2f targetPosition, float mapWidth, float mapHeight) {
    if (m_freeLook) {
        return;
    }
    m_maxCameraCenterX = std::max(m_maxCameraCenterX, targetPosition.x);
    centreCamera({m_maxCameraCenterX, targetPosition.y}, mapWidth, mapHeight);
}

void CameraSystem::pan(float moveAxis, bool boost, sf::Time dt, float mapWidth, float mapHeight) {
    const float speed = kFreeLookSpeed * (boost ? kFreeLookBoost : 1.f);
    m_freeLookCentre.x += moveAxis * speed * dt.asSeconds();
    centreCamera(m_freeLookCentre, mapWidth, mapHeight);
    m_freeLookCentre = m_view.getCenter();
}

void CameraSystem::toggleFreeLook() {
    setFreeLook(!m_freeLook);
}

void CameraSystem::setFreeLook(bool enable) {
    m_freeLook = enable;
    m_freeLookCentre = m_view.getCenter();
    std::cout << "[Core Engine] Camera Free-look " << (m_freeLook ? "ON" : "OFF") << "\n";
}

void CameraSystem::drawFreeLookHint(sf::RenderWindow& window, const sf::Font& font, int currentLevel) const {
    std::string label = "F = MAP VIEW   X = SHOOT   C = BOMB";
    sf::Vector2f position(16.f, Config::kViewHeight - 34.f);
    unsigned size = 16;

    if (m_freeLook) {
        float leftEdge = m_view.getCenter().x - Config::kViewWidth / 2.f;
        int firstColumn = static_cast<int>(leftEdge / Config::kTileSize);
        label = "[MAP VIEW " + std::to_string(Config::worldNumber(currentLevel))
              + "-" + std::to_string(Config::stageNumber(currentLevel))
              + "]  COL " + std::to_string(firstColumn) + "-"
              + std::to_string(firstColumn + Config::kViewTilesX - 1)
              + " | A/D SCROLL | SHIFT FASTER | F EXIT";
        position = {16.f, Config::kViewHeight - 38.f};
        size = 18;
    }

    sf::Text hint(font, label, size);
    hint.setPosition(position);
    hint.setFillColor(m_freeLook ? sf::Color::Yellow : sf::Color::White);
    hint.setOutlineColor(sf::Color::Black);
    hint.setOutlineThickness(3.f);
    window.draw(hint);
}

} // namespace Systems
