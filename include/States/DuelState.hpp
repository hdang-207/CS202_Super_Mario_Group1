#pragma once

#include "States/State.hpp"

/**
 * @class DuelState
 * @brief Minimal navigation state for the future local two-player Duel Mode.
 */
class DuelState final : public State {
private:
    sf::Sprite bgSprite;
    sf::RectangleShape darkOverlay;
    sf::Text titleText;
    sf::Text placeholderText;
    sf::Text hintText;

public:
    DuelState(GameStateManager& gsm, Systems::AssetManager& assets);
    ~DuelState() override = default;

    void init() override;
    void handleInput(const sf::Event& event) override;
    void update(sf::Time dt) override;
    void render(sf::RenderWindow& window) override;
};
