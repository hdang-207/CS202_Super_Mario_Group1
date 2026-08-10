#pragma once

#include "Entities/Entity.hpp"
#include <memory>

namespace entity {

enum class EnemyType {
    Goomba,
    BlueKoopa
};

enum class ItemType {
    Mushroom,
    FireFlower,
    Coin
};

struct WalkingData {
    EnemyType type;
    sf::Vector2f position;
    sf::Vector2f velocity;
};

struct ItemData {
    ItemType type;
    sf::Vector2f blockPosition;
    sf::Vector2f position;
};

/**
 * @class EntityFactory
 * @brief Factory class implementing the Factory Method Pattern to construct game entities.
 */
class EntityFactory {
public:
    /**
     * @brief Creates a walking enemy entity structure/object based on requested type.
     * @param type Enemy type (Goomba, BlueKoopa).
     * @param spawnPos Initial world coordinates.
     * @param initialSpeed Initial horizontal speed.
     */
    static WalkingData createEnemyData(EnemyType type, const sf::Vector2f& spawnPos, float initialSpeed);

    /**
     * @brief Creates an item entity structure based on requested type.
     * @param type Item type (Mushroom, FireFlower, Coin).
     * @param blockPos Position of the block generating the item.
     */
    static ItemData createItemData(ItemType type, const sf::Vector2f& blockPos);
};

} // namespace entity
