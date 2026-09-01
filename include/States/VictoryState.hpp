#pragma once
#include "States/State.hpp"
#include "Systems/SaveData.hpp"
#include <SFML/Graphics/Text.hpp>
#include <SFML/Graphics/RectangleShape.hpp>

class VictoryState : public State {
private:
    sf::RectangleShape bgShape;
    sf::Text victoryText;
    sf::Text promptText;
    sf::Text statsText;
    SaveData progress;

public:
    VictoryState(GameStateManager& gsm, Systems::AssetManager& assets, const SaveData& data);
    ~VictoryState() override = default;

    void init() override;
    void handleInput(const sf::Event& event) override;
    void update(sf::Time dt) override;
    void render(sf::RenderWindow& window) override;
};
