#include "Entities/EntityFactory.hpp"

namespace entity {

WalkingData EntityFactory::createEnemyData(EnemyType type, const sf::Vector2f& spawnPos, float initialSpeed) {
    WalkingData data;
    data.type = type;
    data.position = spawnPos;
    data.velocity = sf::Vector2f(-initialSpeed, 0.f);
    return data;
}

ItemData EntityFactory::createItemData(ItemType type, const sf::Vector2f& blockPos) {
    ItemData data;
    data.type = type;
    data.blockPosition = blockPos;
    data.position = blockPos;
    return data;
}

} // namespace entity
