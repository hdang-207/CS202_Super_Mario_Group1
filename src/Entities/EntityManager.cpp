#include "Entities/EntityManager.hpp"
#include "Entities/Goomba.hpp"
#include "Entities/HammerBro.hpp"
#include "Entities/Koopa.hpp"
#include "Entities/Paratroopa.hpp"
#include "Entities/PiranhaPlant.hpp"
#include "Items/Star.hpp"
#include "Items/Mushroom.hpp"
#include "Physics/PhysicsSystem.hpp"
#include "Core/Config.hpp"
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

void EntityManager::update(
    sf::Time dt,
    physics::PhysicsSystem& physicsSystem,
    const std::function<std::vector<physics::AABB>(const sf::FloatRect&)>& broadphaseSolidQuery,
    float mapWidth,
    float mapHeight,
    float tileSize,
    float cameraCenterX) {
    m_isUpdating = true;
    float seconds = dt.asSeconds();

    const float cameraLeft = cameraCenterX - Config::kViewWidth / 2.f - tileSize * 2.f;
    const float cameraRight = cameraCenterX + Config::kViewWidth / 2.f + tileSize * 2.f;

    for (auto& entity : m_entities) {
        if (!entity || !entity->isAlive()) {
            continue;
        }

        // Activate off-screen enemies when camera approaches them
        if (!entity->isActive()) {
            float posX = entity->getPosition().x;
            if (posX >= cameraLeft && posX <= cameraRight) {
                entity->setActive(true);
            } else {
                continue;
            }
        }

        // Physics-driven solid collision updates for characters & moving items
        if (auto* goomba = dynamic_cast<Goomba*>(entity.get())) {
            sf::FloatRect broadBounds(
                {goomba->getPosition().x - tileSize, goomba->getPosition().y - tileSize},
                {tileSize * 3.f, tileSize * 3.f});
            std::vector<physics::AABB> solids = broadphaseSolidQuery(broadBounds);
            goomba->update(seconds, physicsSystem, solids, mapWidth, mapHeight);
        } else if (auto* paratroopa = dynamic_cast<Paratroopa*>(entity.get())) {
            sf::FloatRect broadBounds(
                {paratroopa->getPosition().x - tileSize, paratroopa->getPosition().y - tileSize},
                {tileSize * 3.f, tileSize * 3.5f});
            std::vector<physics::AABB> solids = broadphaseSolidQuery(broadBounds);
            paratroopa->update(seconds, physicsSystem, solids, mapWidth, mapHeight);
        } else if (auto* hammerBro = dynamic_cast<HammerBro*>(entity.get())) {
            sf::FloatRect broadBounds(
                {hammerBro->getPosition().x - tileSize, hammerBro->getPosition().y - tileSize},
                {tileSize * 3.f, tileSize * 4.f});
            std::vector<physics::AABB> solids = broadphaseSolidQuery(broadBounds);
            hammerBro->update(seconds, physicsSystem, solids, mapWidth, mapHeight);
        } else if (auto* koopa = dynamic_cast<Koopa*>(entity.get())) {
            sf::FloatRect broadBounds(
                {koopa->getPosition().x - tileSize, koopa->getPosition().y - tileSize},
                {tileSize * 3.f, tileSize * 3.5f});
            std::vector<physics::AABB> solids = broadphaseSolidQuery(broadBounds);
            koopa->update(seconds, physicsSystem, solids, mapWidth, mapHeight);
        } else if (auto* mushroom = dynamic_cast<items::Mushroom*>(entity.get())) {
            sf::FloatRect broadBounds(
                {mushroom->getPosition().x - tileSize * 2.f, mushroom->getPosition().y - tileSize * 2.f},
                {tileSize * 5.f, tileSize * 6.f});
            std::vector<physics::AABB> solids = broadphaseSolidQuery(broadBounds);
            mushroom->update(seconds, tileSize, physicsSystem, solids);
        } else if (auto* star = dynamic_cast<items::Star*>(entity.get())) {
            sf::FloatRect broadBounds(
                {star->getPosition().x - tileSize * 2.f, star->getPosition().y - tileSize * 2.f},
                {tileSize * 5.f, tileSize * 6.f});
            std::vector<physics::AABB> solids = broadphaseSolidQuery(broadBounds);
            star->update(seconds, tileSize, solids);
        } else {
            entity->update(dt);
        }
    }

    // Moving Koopa shell collisions against other enemies
    for (auto& entity : m_entities) {
        if (!entity || !entity->isAlive() || !entity->isActive()) continue;
        if (auto* koopa = dynamic_cast<Koopa*>(entity.get())) {
            if (koopa->getState() == KoopaState::ShellMoving) {
                sf::FloatRect shellBounds = koopa->getBounds();
                for (auto& other : m_entities) {
                    if (!other || other.get() == entity.get() || !other->isAlive() || !other->isActive()) continue;
                    if (auto* otherChar = dynamic_cast<Character*>(other.get())) {
                        if (shellBounds.findIntersection(otherChar->getBounds()).has_value()) {
                            otherChar->setAlive(false);
                        }
                    }
                }
            }
        }
    }

    m_isUpdating = false;

    processPendingEntities();
    removeDeadEntities();
}

void EntityManager::renderPiranhas(sf::RenderWindow& window) const {
    for (const auto& entity : m_entities) {
        if (entity && entity->isAlive() && entity->isActive()) {
            if (dynamic_cast<const PiranhaPlant*>(entity.get())) {
                entity->render(window);
            }
        }
    }
}

void EntityManager::render(sf::RenderWindow& window) const {
    for (const auto& entity : m_entities) {
        if (entity && entity->isAlive() && entity->isActive()) {
            if (!dynamic_cast<const PiranhaPlant*>(entity.get())) {
                entity->render(window);
            }
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

std::vector<Entity*> EntityManager::queryOverlapping(const sf::FloatRect& searchArea) const {
    std::vector<Entity*> results;
    for (const auto& entity : m_entities) {
        if (!entity || !entity->isAlive() || !entity->isActive()) {
            continue;
        }
        if (entity->getBounds().findIntersection(searchArea).has_value()) {
            results.push_back(entity.get());
        }
    }
    return results;
}

void EntityManager::processPendingEntities() {
    if (m_pendingEntities.empty()) {
        return;
    }
    m_entities.reserve(m_entities.size() + m_pendingEntities.size());
    for (auto& pending : m_pendingEntities) {
        m_entities.push_back(std::move(pending));
    }
    m_pendingEntities.clear();
}

void EntityManager::removeDeadEntities() {
    m_entities.erase(
        std::remove_if(m_entities.begin(), m_entities.end(),
            [](const std::unique_ptr<Entity>& entity) {
                return !entity || !entity->isAlive();
            }),
        m_entities.end()
    );
}

} // namespace entity
