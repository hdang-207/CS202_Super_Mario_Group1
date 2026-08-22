#pragma once
#include "States/State.hpp"
#include "Systems/SaveData.hpp"
#include <SFML/Graphics/Text.hpp>
#include <SFML/Graphics/RectangleShape.hpp>

/**
 * @class LevelCompleteState
 * @brief Dedicated state shown upon completing a level, displaying scores, coins, and options to continue or save.
 */
class LevelCompleteState : public State {
private:
    sf::RectangleShape bgShape;
    sf::Text titleText;
    sf::Text statsText;
    sf::Text promptText;
    SaveData progress;
    float m_elapsedTime{0.f};

public:
    LevelCompleteState(GameStateManager& gsm, Systems::AssetManager& assets, const SaveData& data);
    ~LevelCompleteState() override = default;

    void init() override;
    void handleInput(const sf::Event& event) override;
    void update(sf::Time dt) override;
    void render(sf::RenderWindow& window) override;
};
