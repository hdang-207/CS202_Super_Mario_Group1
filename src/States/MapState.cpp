#include "States/MapState.hpp"
#include <iostream>

MapState::MapState(GameStateManager& gsm) : State(gsm) {}

void MapState::init() {
    std::cout << "[Core Engine] MapState Initialized.\n";

    // Scatter a handful of decorative clouds across the sky at varying heights.
    clouds.emplace_back(sf::Vector2f(60.f, 50.f));
    clouds.emplace_back(sf::Vector2f(400.f, 110.f));
    clouds.emplace_back(sf::Vector2f(750.f, 60.f));
    clouds.emplace_back(sf::Vector2f(980.f, 140.f));
}

void MapState::handleInput(const sf::Event& event) {
    // Skeleton placeholder - map input (movement, pause, etc.) will be added later.
}

void MapState::update(sf::Time dt) {
    // Skeleton placeholder - map/entity logic will be added later.
}

void MapState::render(sf::RenderWindow& window) {
    // Classic Mario sky-blue background.
    window.clear(sf::Color(107, 140, 255));

    for (auto& cloud : clouds) {
        cloud.render(window);
    }
}
