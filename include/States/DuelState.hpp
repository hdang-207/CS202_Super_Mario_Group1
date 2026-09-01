#pragma once

#include "States/State.hpp"
#include "Systems/MapParser.hpp"
#include "Systems/TileMap.hpp"

/**
 * @class DuelState
 * @brief Loads and previews the static arena for local Duel Mode.
 */
class DuelState final : public State {
private:
    MapParser mapParser;
    TileMap tileMap;
    bool arenaLoaded{false};

    sf::RectangleShape skyBackground;
    sf::Text titleText;
    sf::Text errorText;
    sf::Text hintText;

public:
    DuelState(GameStateManager& gsm, Systems::AssetManager& assets);
    ~DuelState() override = default;

    void init() override;
    void handleInput(const sf::Event& event) override;
    void update(sf::Time dt) override;
    void render(sf::RenderWindow& window) override;
};
