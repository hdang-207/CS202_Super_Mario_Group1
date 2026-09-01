#pragma once

#include "Core/Config.hpp"
#include <SFML/Graphics/Font.hpp>
#include <SFML/Graphics/RenderWindow.hpp>
#include <SFML/Graphics/Text.hpp>
#include <SFML/Graphics/View.hpp>
#include <SFML/System/Time.hpp>
#include <SFML/System/Vector2.hpp>
#include <algorithm>
#include <string>

namespace Systems {

/**
 * @class CameraSystem
 * @brief Manages 2D camera scrolling, boundary clamping, SMB 1985 one-way scroll locking, and Free-look mode.
 */
class CameraSystem {
public:
    static constexpr float kFreeLookSpeed = 900.f;
    static constexpr float kFreeLookBoost = 3.f;

    CameraSystem();
    explicit CameraSystem(const sf::FloatRect& viewRect);

    /**
     * @brief Resets camera position and scroll limits for a new level.
     */
    void reset(sf::Vector2f initialCenter = {Config::kViewWidth / 2.f, Config::kViewHeight / 2.f});

    /**
     * @brief Centers camera on target coordinate with boundary clamping.
     */
    void centreCamera(sf::Vector2f target, float mapWidth, float mapHeight);

    /**
     * @brief Updates camera to follow player target position and maintains one-way scroll lock.
     */
    void followTarget(sf::Vector2f targetPosition, float mapWidth, float mapHeight);

    /**
     * @brief Pans camera manually during free-look mode.
     */
    void pan(float moveAxis, bool boost, sf::Time dt, float mapWidth, float mapHeight);

    /**
     * @brief Toggles free-look mode on/off.
     */
    void toggleFreeLook();

    /**
     * @brief Sets free-look mode explicitly.
     */
    void setFreeLook(bool enable);

    [[nodiscard]] bool isFreeLook() const noexcept { return m_freeLook; }
    [[nodiscard]] sf::Vector2f getCenter() const noexcept { return m_view.getCenter(); }
    [[nodiscard]] float getMaxCameraCenterX() const noexcept { return m_maxCameraCenterX; }
    void setMaxCameraCenterX(float x) noexcept { m_maxCameraCenterX = x; }

    [[nodiscard]] sf::View& getView() noexcept { return m_view; }
    [[nodiscard]] const sf::View& getView() const noexcept { return m_view; }

    void setViewport(const sf::FloatRect& viewport) { m_view.setViewport(viewport); }

    /**
     * @brief Renders the free-look hint overlay to the window.
     */
    void drawFreeLookHint(sf::RenderWindow& window, const sf::Font& font, int currentLevel) const;

private:
    sf::View m_view;
    float m_maxCameraCenterX{Config::kViewWidth / 2.f};
    bool m_freeLook{false};
    sf::Vector2f m_freeLookCentre;
};

} // namespace Systems
