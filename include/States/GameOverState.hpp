#pragma once
#include "States/State.hpp"
#include <SFML/Graphics/Text.hpp>
#include <SFML/Graphics/RectangleShape.hpp>
#include "Core/GameMode.hpp"
#include <SFML/Graphics/Sprite.hpp>
#include <optional>

class GameOverState : public State {
private:
    GameMode m_gameMode;
    std::optional<sf::Sprite> bgSprite;
    sf::RectangleShape darkOverlay; // used to dim the background if needed
    sf::Text gameOverText;
    sf::Text promptText;

public:
    GameOverState(GameStateManager& gsm, Systems::AssetManager& assets, GameMode mode = GameMode::Normal);
    ~GameOverState() override = default;

    void init() override;
    void handleInput(const sf::Event& event) override;
    void update(sf::Time dt) override;
    void render(sf::RenderWindow& window) override;
};
