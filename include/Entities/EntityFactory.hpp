#pragma once

#include "Entities/Entity.hpp"
#include "Entities/Goomba.hpp"
#include "Entities/Koopa.hpp"
#include "Entities/Paratroopa.hpp"
#include "Entities/PiranhaPlant.hpp"
#include "Entities/CoinPop.hpp"
#include "Items/Mushroom.hpp"
#include "Items/FireFlower.hpp"
#include "Items/Star.hpp"
#include <memory>

namespace entity {

enum class EnemyType {
    Goomba,
    BlueKoopa,
    GreenKoopa,
    GreenParatroopa
};

enum class ItemType {
    Mushroom,
    FireFlower,
    Coin,
    Star,
    OneUpMushroom
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
    static WalkingData createEnemyData(EnemyType type, const sf::Vector2f& spawnPos, float initialSpeed);
    static ItemData createItemData(ItemType type, const sf::Vector2f& blockPos);

    static std::unique_ptr<Goomba> createGoomba(const sf::Vector2f& spawnPos, float tileSize, GoombaType type = GoombaType::Normal, float initialSpeed = 72.f);
    static std::unique_ptr<Koopa> createKoopa(const sf::Vector2f& spawnPos, float tileSize, KoopaKind kind = KoopaKind::Green, float initialSpeed = 60.f);
    static std::unique_ptr<Paratroopa> createParatroopa(const sf::Vector2f& spawnPos, float tileSize, float initialSpeed = 60.f, float bounceSpeed = 600.f);
    static std::unique_ptr<PiranhaPlant> createPiranhaPlant(const sf::Vector2f& basePos, float pipeTopY, float scale = 3.f);
    static std::unique_ptr<CoinPop> createCoinPop(const sf::Vector2f& blockPos, float tileSize);
    static std::unique_ptr<items::Mushroom> createMushroom(const sf::Vector2f& blockPos, items::MushroomKind kind = items::MushroomKind::Super);
    static std::unique_ptr<items::FireFlower> createFireFlower(const sf::Vector2f& blockPos);
    static std::unique_ptr<items::Star> createStar(const sf::Vector2f& blockPos);
};

} // namespace entity
