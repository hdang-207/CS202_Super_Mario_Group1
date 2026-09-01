#pragma once

#include "Core/CharacterType.hpp"
#include "States/State.hpp"

/**
 * @class WorldSelectionState
 * @brief Lets a new game choose one of the three independent world routes.
 */
class WorldSelectionState : public State {
private:
    CharacterType selectedCharacter;
    int selectedWorld{1};
    bool nightfallMode{false};

    sf::Sprite bgSprite;
    sf::RectangleShape darkOverlay;
    sf::Text headerText;
    sf::Text world1Text;
    sf::Text world2Text;
    sf::Text world3Text;
    sf::Text routeText;
    sf::Text hintText;

    void chooseWorld(int world);
    void startSelectedWorld();

public:
    WorldSelectionState(
        GameStateManager& gsm,
        Systems::AssetManager& assets,
        CharacterType character,
        bool nightfall = false
    );
    ~WorldSelectionState() override = default;

    void init() override;
    void handleInput(const sf::Event& event) override;
    void update(sf::Time dt) override;
    void render(sf::RenderWindow& window) override;
};
