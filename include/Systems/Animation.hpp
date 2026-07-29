#pragma once
#include <SFML/Graphics.hpp>
#include <vector>

namespace Systems {
    class Animation {
    private:
        std::vector<sf::IntRect> frames;
        float frameTime;
        float elapsed;
        size_t currentFrame;

    public:
        Animation(sf::Texture& texture, int rows, int cols, float duration) {
            frameTime = duration;
            elapsed = 0.0f;
            currentFrame = 0;

            sf::Vector2u size = texture.getSize();
            int width = size.x / cols;
            int height = size.y / rows;

            for (int y = 0; y < rows; y++) {
                for (int x = 0; x < cols; x++) {
                    frames.push_back(sf::IntRect({x * width, y * height}, {width, height}));
                }
            }
        }

        void update(sf::Time dt) {
            elapsed += dt.asSeconds();
            if (elapsed >= frameTime) {
                elapsed = 0;
                currentFrame = (currentFrame + 1) % frames.size();
            }
        }

        const sf::IntRect& getCurrentFrame() const { return frames[currentFrame]; }
    };
}