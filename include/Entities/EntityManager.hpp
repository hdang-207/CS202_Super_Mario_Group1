#pragma once

#include "Entities/Entity.hpp"
#include <vector>
#include <memory>
#include <functional>
#include <algorithm>

namespace sf {
class RenderWindow;
}

namespace physics {
class PhysicsSystem;
struct AABB;
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
     * @brief Simple update fallback.
     * @param dt Frame delta time.
     */
    void update(sf::Time dt);

    /**
     * @brief Processes pending entities, updates all active/alive entities with solid collisions.
     * @param dt Frame delta time.
     * @param physicsSystem Physics collision solver.
     * @param broadphaseSolidQuery Lambda returning solid bounding boxes in a region.
     * @param mapWidth Pixel width of the level.
     * @param mapHeight Pixel height of the level.
     * @param tileSize Size of one tile in pixels.
     * @param cameraCenterX Current center X coordinate of the game camera.
     */
    void update(
        sf::Time dt,
        physics::PhysicsSystem& physicsSystem,
        const std::function<std::vector<physics::AABB>(const sf::FloatRect&)>& broadphaseSolidQuery,
        float mapWidth,
        float mapHeight,
        float tileSize,
        float cameraCenterX
    );

    /**
     * @brief Renders piranha plant entities (called before tileMap so they emerge cleanly behind pipe lips).
     * @param window Target render window.
     */
    void renderPiranhas(sf::RenderWindow& window) const;

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
     * @brief Iterate through entities with a custom function.
     */
    void forEach(const std::function<void(Entity&)>& callback);

    /**
     * @brief Query entities whose bounding box intersects a given search box (Broadphase / Narrowphase query).
     * @param searchArea Bounding box to query against.
     * @return Vector of raw pointers to colliding entities.
     */
    [[nodiscard]] std::vector<Entity*> queryOverlapping(const sf::FloatRect& searchArea) const;

private:
    void processPendingEntities();
    void removeDeadEntities();

    std::vector<std::unique_ptr<Entity>> m_entities;
    std::vector<std::unique_ptr<Entity>> m_pendingEntities;
    bool m_isUpdating{false};
};

} // namespace entity
