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

std::unique_ptr<Goomba> EntityFactory::createGoomba(const sf::Vector2f& spawnPos, float tileSize, GoombaType type, float initialSpeed) {
    return std::make_unique<Goomba>(spawnPos, tileSize, type, initialSpeed);
}

std::unique_ptr<Koopa> EntityFactory::createKoopa(const sf::Vector2f& spawnPos, float tileSize, KoopaKind kind, float initialSpeed) {
    return std::make_unique<Koopa>(spawnPos, tileSize, kind, initialSpeed);
}

std::unique_ptr<Paratroopa> EntityFactory::createParatroopa(const sf::Vector2f& spawnPos, float tileSize, float initialSpeed, float bounceSpeed) {
    return std::make_unique<Paratroopa>(spawnPos, tileSize, initialSpeed, bounceSpeed);
}

std::unique_ptr<PiranhaPlant> EntityFactory::createPiranhaPlant(const sf::Vector2f& basePos, float pipeTopY, float scale) {
    return std::make_unique<PiranhaPlant>(basePos, pipeTopY, scale);
}

std::unique_ptr<CoinPop> EntityFactory::createCoinPop(const sf::Vector2f& blockPos, float tileSize) {
    return std::make_unique<CoinPop>(blockPos, tileSize);
}

std::unique_ptr<items::Mushroom> EntityFactory::createMushroom(const sf::Vector2f& blockPos, items::MushroomKind kind) {
    return std::make_unique<items::Mushroom>(blockPos, kind);
}

std::unique_ptr<items::FireFlower> EntityFactory::createFireFlower(const sf::Vector2f& blockPos) {
    return std::make_unique<items::FireFlower>(blockPos);
}

std::unique_ptr<items::Star> EntityFactory::createStar(const sf::Vector2f& blockPos) {
    return std::make_unique<items::Star>(blockPos);
}

} // namespace entity
