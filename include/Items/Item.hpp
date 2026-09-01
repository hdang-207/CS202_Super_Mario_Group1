#pragma once

#include "Entities/Entity.hpp"
#include <SFML/Graphics/Rect.hpp>

namespace sf {
class RenderWindow;
class Texture;
}

namespace items {

class Item : public entity::Entity {
public:
    virtual ~Item() override = default;

    void update(sf::Time /*dt*/) override {}
    void render(sf::RenderWindow& /*window*/) const override {}

    virtual void render(sf::RenderWindow& window, const sf::Texture& texture,
                        float visualScale) const = 0;
    [[nodiscard]] virtual sf::FloatRect getBounds() const override = 0;
    [[nodiscard]] virtual bool isCollectible() const noexcept = 0;

protected:
    explicit Item(const sf::Vector2f& position = {0.f, 0.f})
        : entity::Entity(position) {}
};

} // namespace items
