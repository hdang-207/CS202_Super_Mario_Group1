#include "Entities/EntityFactory.hpp"

namespace entity {

WalkingData EntityFactory::createEnemyData(EnemyType type, const sf::Vector2f& spawnPos, float initialSpeed) {
    WalkingData data;
    data.type = type;
    data.position = spawnPos;
    data.velocity = {-initialSpeed, 0.f};
    return data;
}

ItemData EntityFactory::createItemData(ItemType type, const sf::Vector2f& blockPos) {
    ItemData data;
    data.type = type;
    data.blockPosition = blockPos;
    data.position = blockPos;
    return data;
}

std::unique_ptr<Goomba> EntityFactory::createGoomba(const sf::Vector2f& spawnPos, float tileSize, const sf::Texture* texture, GoombaType type, float initialSpeed) {
    return std::make_unique<Goomba>(spawnPos, tileSize, texture, type, initialSpeed);
}

std::unique_ptr<Koopa> EntityFactory::createKoopa(const sf::Vector2f& spawnPos, float tileSize, const sf::Texture* walkTex, const sf::Texture* shellTex, KoopaKind kind, float initialSpeed) {
    return std::make_unique<Koopa>(spawnPos, tileSize, walkTex, shellTex, kind, initialSpeed);
}

std::unique_ptr<Paratroopa> EntityFactory::createParatroopa(const sf::Vector2f& spawnPos, float tileSize, const sf::Texture* walkTex, const sf::Texture* shellTex, float initialSpeed, float bounceSpeed, KoopaKind kind) {
    return std::make_unique<Paratroopa>(spawnPos, tileSize, walkTex, shellTex, initialSpeed, bounceSpeed, kind);
}

std::unique_ptr<Blooper> EntityFactory::createBlooper(const sf::Vector2f& spawnPos, float tileSize, const sf::FloatRect& swimBounds, const sf::Texture* texture) {
    return std::make_unique<Blooper>(spawnPos, tileSize, swimBounds, texture);
}

std::unique_ptr<CheepCheep> EntityFactory::createCheepCheep(const sf::Vector2f& spawnPos, float tileSize, const sf::FloatRect& swimBounds, const sf::Texture* texture, float swimSpeed) {
    return std::make_unique<CheepCheep>(spawnPos, tileSize, swimBounds, texture, swimSpeed);
}

std::unique_ptr<FlyingCheepCheep> EntityFactory::createFlyingCheepCheep(
    const sf::Vector2f& spawnPos, const sf::Vector2f& velocity,
    float tileSize, const sf::FloatRect& flightBounds, const sf::Texture* texture) {
    return std::make_unique<FlyingCheepCheep>(
        spawnPos, velocity, tileSize, flightBounds, texture);
}

std::unique_ptr<HammerBro> EntityFactory::createHammerBro(const sf::Vector2f& spawnPos, float tileSize, const sf::Texture* texture) {
    return std::make_unique<HammerBro>(spawnPos, tileSize, texture);
}

std::unique_ptr<PiranhaPlant> EntityFactory::createPiranhaPlant(const sf::Vector2f& basePos, float pipeTopY, const sf::Texture* texture, float scale) {
    return std::make_unique<PiranhaPlant>(basePos, pipeTopY, texture, scale);
}

std::unique_ptr<CoinPop> EntityFactory::createCoinPop(const sf::Vector2f& blockPos, float tileSize, const sf::Texture* texture) {
    return std::make_unique<CoinPop>(blockPos, tileSize, texture);
}

std::unique_ptr<items::Mushroom> EntityFactory::createMushroom(const sf::Vector2f& blockPos, items::MushroomKind kind, const sf::Texture* texture) {
    return std::make_unique<items::Mushroom>(blockPos, kind, texture);
}

std::unique_ptr<items::FireFlower> EntityFactory::createFireFlower(const sf::Vector2f& blockPos, const sf::Texture* texture) {
    return std::make_unique<items::FireFlower>(blockPos, texture);
}

std::unique_ptr<items::Star> EntityFactory::createStar(const sf::Vector2f& blockPos, const sf::Texture* texture) {
    return std::make_unique<items::Star>(blockPos, texture);
}

} // namespace entity
