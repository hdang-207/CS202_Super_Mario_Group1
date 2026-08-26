#pragma once

#include "Entities/Entity.hpp"
#include "Entities/Goomba.hpp"
#include "Entities/Koopa.hpp"
#include "Entities/Paratroopa.hpp"
#include "Entities/Blooper.hpp"
#include "Entities/CheepCheep.hpp"
#include "Entities/FlyingCheepCheep.hpp"
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

    static std::unique_ptr<Goomba> createGoomba(const sf::Vector2f& spawnPos, float tileSize, const sf::Texture* texture = nullptr, GoombaType type = GoombaType::Normal, float initialSpeed = 72.f);
    static std::unique_ptr<Koopa> createKoopa(const sf::Vector2f& spawnPos, float tileSize, const sf::Texture* walkTex = nullptr, const sf::Texture* shellTex = nullptr, KoopaKind kind = KoopaKind::Green, float initialSpeed = 60.f);
    static std::unique_ptr<Paratroopa> createParatroopa(const sf::Vector2f& spawnPos, float tileSize, const sf::Texture* walkTex = nullptr, const sf::Texture* shellTex = nullptr, float initialSpeed = 60.f, float bounceSpeed = 600.f);
    static std::unique_ptr<Blooper> createBlooper(const sf::Vector2f& spawnPos, float tileSize, const sf::FloatRect& swimBounds, const sf::Texture* texture = nullptr);
    static std::unique_ptr<CheepCheep> createCheepCheep(const sf::Vector2f& spawnPos, float tileSize, const sf::FloatRect& swimBounds, const sf::Texture* texture = nullptr, float swimSpeed = 96.f);
    static std::unique_ptr<FlyingCheepCheep> createFlyingCheepCheep(
        const sf::Vector2f& spawnPos, const sf::Vector2f& velocity,
        float tileSize, const sf::FloatRect& flightBounds,
        const sf::Texture* texture = nullptr);
    static std::unique_ptr<PiranhaPlant> createPiranhaPlant(const sf::Vector2f& basePos, float pipeTopY, const sf::Texture* texture = nullptr, float scale = 3.f);
    static std::unique_ptr<CoinPop> createCoinPop(const sf::Vector2f& blockPos, float tileSize, const sf::Texture* texture = nullptr);
    static std::unique_ptr<items::Mushroom> createMushroom(const sf::Vector2f& blockPos, items::MushroomKind kind = items::MushroomKind::Super, const sf::Texture* texture = nullptr);
    static std::unique_ptr<items::FireFlower> createFireFlower(const sf::Vector2f& blockPos, const sf::Texture* texture = nullptr);
    static std::unique_ptr<items::Star> createStar(const sf::Vector2f& blockPos, const sf::Texture* texture = nullptr);
};

} // namespace entity
