#pragma once
#include <SFML/Graphics.hpp>
#include <vector>

namespace Systems {
    /**
     * @class Animation
     * @brief Manages sprite sheet animation sequences and frame timing.
     */
    class Animation {
    private:
        std::vector<sf::IntRect> frames;
        float frameTime;
        float elapsed;
        size_t currentFrame;

    public:
        /**
         * @brief Constructs an Animation sequence from a grid texture.
         * @param texture Sprite sheet texture containing frame grid.
         * @param rows Number of vertical frame rows.
         * @param cols Number of horizontal frame columns.
         * @param duration Time duration (in seconds) to display each frame.
         */
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

        /**
         * @brief Advances animation frame state based on delta time.
         * @param dt Time elapsed since the last update frame.
         */
        void update(sf::Time dt) {
            elapsed += dt.asSeconds();
            if (elapsed >= frameTime) {
                elapsed = 0;
                currentFrame = (currentFrame + 1) % frames.size();
            }
        }

        /**
         * @brief Gets the sub-rectangle texture bounds for the currently active frame.
         * @return Const reference to the current frame IntRect.
         */
        const sf::IntRect& getCurrentFrame() const { return frames[currentFrame]; }
    };
}