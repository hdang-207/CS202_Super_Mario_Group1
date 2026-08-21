#include "Entities/EntityManager.hpp"
#include <SFML/Graphics/RenderWindow.hpp>

namespace entity {

void EntityManager::addEntity(std::unique_ptr<Entity> entity) {
    if (!entity) {
        return;
    }
    if (m_isUpdating) {
        m_pendingEntities.push_back(std::move(entity));
    } else {
        m_entities.push_back(std::move(entity));
    }
}

void EntityManager::update(sf::Time dt) {
    m_isUpdating = true;

    for (auto& entity : m_entities) {
        if (entity && entity->isAlive() && entity->isActive()) {
            entity->update(dt);
        }
    }

    m_isUpdating = false;

    processPendingEntities();
    removeDeadEntities();
}

void EntityManager::render(sf::RenderWindow& window) const {
    for (const auto& entity : m_entities) {
        if (entity && entity->isAlive() && entity->isActive()) {
            entity->render(window);
        }
    }
}

void EntityManager::clear() {
    m_entities.clear();
    m_pendingEntities.clear();
}

std::size_t EntityManager::size() const noexcept {
    return m_entities.size();
}

bool EntityManager::empty() const noexcept {
    return m_entities.empty();
}

const std::vector<std::unique_ptr<Entity>>& EntityManager::getEntities() const noexcept {
    return m_entities;
}

void EntityManager::forEach(const std::function<void(Entity&)>& callback) {
    for (auto& entity : m_entities) {
        if (entity && entity->isAlive()) {
            callback(*entity);
        }
    }
}

void EntityManager::forEach(const std::function<void(const Entity&)>& callback) const {
    for (const auto& entity : m_entities) {
        if (entity && entity->isAlive()) {
            callback(*entity);
        }
    }
}

std::vector<Entity*> EntityManager::queryOverlapping(const sf::FloatRect& bounds) const {
    std::vector<Entity*> result;
    for (const auto& entity : m_entities) {
        if (entity && entity->isAlive() && entity->isActive()) {
            if (entity->getBounds().findIntersection(bounds).has_value()) {
                result.push_back(entity.get());
            }
        }
    }
    return result;
}

void EntityManager::processPendingEntities() {
    if (m_pendingEntities.empty()) {
        return;
    }
    for (auto& pending : m_pendingEntities) {
        if (pending) {
            m_entities.push_back(std::move(pending));
        }
    }
    m_pendingEntities.clear();
}

void EntityManager::removeDeadEntities() {
    m_entities.erase(
        std::remove_if(
            m_entities.begin(),
            m_entities.end(),
            [](const std::unique_ptr<Entity>& entity) {
                return !entity || !entity->isAlive();
            }
        ),
        m_entities.end()
    );
}

} // namespace entity
