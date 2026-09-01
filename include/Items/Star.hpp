#pragma once

#include "Items/Item.hpp"
#include "Physics/AABB.hpp"

#include <SFML/Graphics/Texture.hpp>
#include <SFML/System/Vector2.hpp>
#include <vector>

namespace sf {
class RenderWindow;
}

namespace items {

enum class StarState { Emerging, Moving };

class Star final : public Item {
public:
    explicit Star(sf::Vector2f blockPosition, const sf::Texture* texture = nullptr, float scale = 3.f);

    void update(sf::Time dt) override;
    void update(float deltaTime);
    void update(float deltaTime, float tileSize, const std::vector<physics::AABB>& solids);
    void render(sf::RenderWindow& window) const override;
    void render(sf::RenderWindow& window, const sf::Texture& texture,
                float scale) const override;

    [[nodiscard]] sf::FloatRect getBounds() const override;
    [[nodiscard]] bool isCollectible() const noexcept override;
    [[nodiscard]] bool hasFallenOut(float worldHeight) const noexcept;

private:
    const sf::Texture* m_texture{nullptr};
    float m_scale{3.f};
    sf::Vector2f m_blockPosition;
    StarState m_state{StarState::Emerging};
    float m_elapsed{0.f};
    float m_size{48.f};
};

} // namespace items
