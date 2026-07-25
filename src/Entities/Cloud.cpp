#include "Entities/Cloud.hpp"

namespace {
    // Pixel pattern of a classic puffy cloud silhouette.
    // ' ' = empty, 'L' = light gray fill, 'D' = darker gray underside shading.
    const std::vector<std::string> kCloudPattern = {
        "      LLLL        LLLL     ",
        "     LLLLLL      LLLLLL    ",
        "    LLLLLLLL    LLLLLLLL   ",
        "   LLLLLLLLLLLLLLLLLLLLLL  ",
        "  LLLLLLLLLLLLLLLLLLLLLLLL ",
        " LLLLLLLLLLLLLLLLLLLLLLLLLL",
        "LLLLLLLLLLLLLLLLLLLLLLLLLLLL",
        "LLLLLLLLLLLLLLLLLLLLLLLLLLLL",
        "LLLLLLLLLLLLLLLLLLLLLLLLLLLL",
        " DDDDDDDDDDDDDDDDDDDDDDDDDD ",
        "  DDDDDDDDDDDDDDDDDDDDDDDD  "
    };

    const sf::Color kFillColor(190, 190, 190);
    const sf::Color kShadeColor(130, 130, 130);
}

Cloud::Cloud(sf::Vector2f position, float pixelSize) {
    build(position, pixelSize);
}

void Cloud::build(sf::Vector2f position, float pixelSize) {
    for (std::size_t row = 0; row < kCloudPattern.size(); ++row) {
        const std::string& line = kCloudPattern[row];
        for (std::size_t col = 0; col < line.size(); ++col) {
            char pixel = line[col];
            if (pixel == ' ') {
                continue;
            }

            sf::RectangleShape block(sf::Vector2f(pixelSize, pixelSize));
            block.setPosition(position + sf::Vector2f(col * pixelSize, row * pixelSize));
            block.setFillColor(pixel == 'L' ? kFillColor : kShadeColor);
            blocks.push_back(block);
        }
    }
}

void Cloud::render(sf::RenderWindow& window) {
    for (auto& block : blocks) {
        window.draw(block);
    }
}
