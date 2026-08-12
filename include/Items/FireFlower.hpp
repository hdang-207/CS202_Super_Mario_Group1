#pragma once

#include "Items/Item.hpp"

#include <SFML/Graphics/Texture.hpp>
#include <SFML/System/Vector2.hpp>

namespace sf {
class RenderWindow;
}

namespace items {

enum class FireFlowerState { Emerging, Ready };

class FireFlower final : public Item {
public:
    explicit FireFlower(sf::Vector2f blockPosition);

    void update(float deltaTime, float tileSize);
    void render(sf::RenderWindow& window, const sf::Texture& texture,
                float tileSize) const override;

    [[nodiscard]] sf::FloatRect getBounds() const override;
    [[nodiscard]] bool isCollectible() const noexcept override;

private:
    sf::Vector2f m_blockPosition;
    sf::Vector2f m_position;
    FireFlowerState m_state{FireFlowerState::Emerging};
    float m_elapsed{0.f};
    float m_size{0.f};
};

} // namespace items
