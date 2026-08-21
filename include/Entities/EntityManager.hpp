#pragma once

#include "Entities/Entity.hpp"
#include <vector>
#include <memory>
#include <functional>
#include <algorithm>

namespace sf {
class RenderWindow;
}

namespace entity {

/**
 * @class EntityManager
 * @brief Manages the lifecycle, deferred spawning, updating, rendering, and queries for game entities.
 */
class EntityManager {
public:
    EntityManager() = default;
    ~EntityManager() = default;

    EntityManager(const EntityManager&) = delete;
    EntityManager& operator=(const EntityManager&) = delete;
    EntityManager(EntityManager&&) noexcept = default;
    EntityManager& operator=(EntityManager&&) noexcept = default;

    /**
     * @brief Adds a new entity to the manager (deferred when updating to prevent iterator invalidation).
     * @param entity The entity to add.
     */
    void addEntity(std::unique_ptr<Entity> entity);

    /**
     * @brief Processes pending entities, updates all active/alive entities, and cleans up dead ones.
     * @param dt Frame delta time.
     */
    void update(sf::Time dt);

    /**
     * @brief Renders all alive entities to the target window.
     * @param window Target render window.
     */
    void render(sf::RenderWindow& window) const;

    /**
     * @brief Clears all entities immediately.
     */
    void clear();

    /**
     * @brief Returns the total number of managed entities.
     */
    [[nodiscard]] std::size_t size() const noexcept;

    /**
     * @brief Checks if there are no managed entities.
     */
    [[nodiscard]] bool empty() const noexcept;

    /**
     * @brief Access the immutable list of current entities.
     */
    [[nodiscard]] const std::vector<std::unique_ptr<Entity>>& getEntities() const noexcept;

    /**
     * @brief Apply a function to every active/alive entity.
     */
    void forEach(const std::function<void(Entity&)>& callback);
    void forEach(const std::function<void(const Entity&)>& callback) const;

    /**
     * @brief Queries entities whose bounding box intersects the given bounds.
     */
    [[nodiscard]] std::vector<Entity*> queryOverlapping(const sf::FloatRect& bounds) const;

private:
    std::vector<std::unique_ptr<Entity>> m_entities;
    std::vector<std::unique_ptr<Entity>> m_pendingEntities;
    bool m_isUpdating{false};

    void processPendingEntities();
    void removeDeadEntities();
};

} // namespace entity
