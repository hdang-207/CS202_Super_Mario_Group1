#pragma once
#include "States/State.hpp"
#include "Systems/SaveData.hpp"
#include <SFML/Graphics/Text.hpp>
#include <SFML/Graphics/RectangleShape.hpp>
#include <SFML/Graphics/Sprite.hpp>

/**
 * @class RespawnState
 * @brief Authentic SMB 1985 life screen pause state. Shows world title and remaining lives for 2.5s before restarting or game over.
 */
class RespawnState : public State {
private:
    sf::RectangleShape bgShape;
    sf::Text worldText;
    sf::Text livesText;
    sf::Text promptText;
    sf::Sprite characterSprite;
    SaveData progress;
    float displayTimer{2.5f};

public:
    RespawnState(GameStateManager& gsm, Systems::AssetManager& assets, const SaveData& data);
    ~RespawnState() override = default;

    void init() override;
    void handleInput(const sf::Event& event) override;
    void update(sf::Time dt) override;
    void render(sf::RenderWindow& window) override;
};
