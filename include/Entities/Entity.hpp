#pragma once

#include <SFML/Graphics.hpp>

namespace entity {

/**
 * @class Entity
 * @brief Base abstract class for all game entities (Enemies, Items, Projectiles, etc.)
 */
class Entity {
public:
    virtual ~Entity() = default;

    /**
     * @brief Update entity logic over time.
     * @param dt Elapsed time since last frame.
     */
    virtual void update(sf::Time dt) = 0;

    /**
     * @brief Render the entity to the window.
     * @param window Target render window.
     */
    virtual void render(sf::RenderWindow& window) const = 0;

    /**
     * @brief Get global bounding box for collision detection.
     */
    [[nodiscard]] virtual sf::FloatRect getBounds() const = 0;

    // Position accessors
    void setPosition(const sf::Vector2f& pos) { m_position = pos; }
    [[nodiscard]] const sf::Vector2f& getPosition() const noexcept { return m_position; }

    // Velocity accessors
    void setVelocity(const sf::Vector2f& vel) { m_velocity = vel; }
    [[nodiscard]] const sf::Vector2f& getVelocity() const noexcept { return m_velocity; }

    // Status accessors
    void setActive(bool active) noexcept { m_active = active; }
    [[nodiscard]] bool isActive() const noexcept { return m_active; }

    void setAlive(bool alive) noexcept { m_alive = alive; }
    [[nodiscard]] bool isAlive() const noexcept { return m_alive; }

protected:
    Entity(const sf::Vector2f& position = {0.f, 0.f})
        : m_position(position), m_velocity(0.f, 0.f), m_active(true), m_alive(true) {}

    sf::Vector2f m_position;
    sf::Vector2f m_velocity;
    bool m_active{true};
    bool m_alive{true};
};

} // namespace entity
