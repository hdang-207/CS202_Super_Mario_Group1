#pragma once

#include "Items/Item.hpp"

#include <SFML/Graphics/Texture.hpp>
#include <SFML/System/Vector2.hpp>

namespace sf {
class RenderWindow;
}

namespace items {

enum class ManaOrbState { Emerging, Ready };

class ManaOrb final : public Item {
public:
    ManaOrb(sf::Vector2f blockPosition,
            const sf::Texture* texture = nullptr,
            float scale = 3.f);

    void update(sf::Time dt) override;
    void update(float deltaTime);
    void update(float deltaTime, float tileSize);
    void render(sf::RenderWindow& window) const override;
    void render(sf::RenderWindow& window, const sf::Texture& texture,
                float tileSize) const override;

    [[nodiscard]] sf::FloatRect getBounds() const override;
    [[nodiscard]] bool isCollectible() const noexcept override;

private:
    const sf::Texture* m_texture{nullptr};
    float m_scale{3.f};
    sf::Vector2f m_blockPosition;
    ManaOrbState m_state{ManaOrbState::Emerging};
    float m_elapsed{0.f};
    float m_size{48.f};
};

} // namespace items
