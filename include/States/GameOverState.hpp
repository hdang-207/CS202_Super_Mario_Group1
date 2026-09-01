#pragma once
#include "States/State.hpp"
#include <SFML/Graphics/Text.hpp>
#include <SFML/Graphics/RectangleShape.hpp>

class GameOverState : public State {
private:
    sf::RectangleShape bgShape;
    sf::Text gameOverText;
    sf::Text promptText;

public:
    GameOverState(GameStateManager& gsm, Systems::AssetManager& assets);
    ~GameOverState() override = default;

    void init() override;
    void handleInput(const sf::Event& event) override;
    void update(sf::Time dt) override;
    void render(sf::RenderWindow& window) override;
};
