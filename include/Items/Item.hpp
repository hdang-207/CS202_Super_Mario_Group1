#pragma once

#include <SFML/Graphics/Rect.hpp>

namespace sf {
class RenderWindow;
class Texture;
}

namespace items {

class Item {
public:
    virtual ~Item() = default;

    virtual void render(sf::RenderWindow& window, const sf::Texture& texture,
                        float visualScale) const = 0;
    [[nodiscard]] virtual sf::FloatRect getBounds() const = 0;
    [[nodiscard]] virtual bool isCollectible() const noexcept = 0;
};

} // namespace items
