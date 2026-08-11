#pragma once

#include "Items/Item.hpp"

#include <SFML/Graphics/Texture.hpp>
#include <SFML/System/Vector2.hpp>

namespace sf {
class RenderWindow;
}

class TileMap;

namespace items {

enum class StarState { Emerging, Moving };

class Star final : public Item {
public:
    explicit Star(sf::Vector2f blockPosition);

    void update(float deltaTime, const TileMap& tileMap);
    void render(sf::RenderWindow& window, const sf::Texture& texture,
                float scale) const override;

    [[nodiscard]] sf::FloatRect getBounds() const override;
    [[nodiscard]] bool isCollectible() const noexcept override;
    [[nodiscard]] bool hasFallenOut(float worldHeight) const noexcept;

private:
    sf::Vector2f m_blockPosition;
    sf::Vector2f m_position;
    sf::Vector2f m_velocity{0.f, 0.f};
    StarState m_state{StarState::Emerging};
    float m_elapsed{0.f};
    float m_size{0.f};
};

} // namespace items
