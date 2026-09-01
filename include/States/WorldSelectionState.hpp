#pragma once

#include "Core/CharacterType.hpp"
#include "States/State.hpp"

/**
 * @class WorldSelectionState
 * @brief Lets a new game choose one of the three independent world routes.
 */
class WorldSelectionState : public State {
private:
    enum class SelectionOption {
        World1,
        World2,
        World3,
        Duel
    };

    CharacterType selectedCharacter;
    SelectionOption selectedOption{SelectionOption::World1};

    sf::Sprite bgSprite;
    sf::RectangleShape darkOverlay;
    sf::Text headerText;
    sf::Text world1Text;
    sf::Text world2Text;
    sf::Text world3Text;
    sf::Text duelText;
    sf::Text routeText;
    sf::Text hintText;

    void selectPreviousOption();
    void selectNextOption();
    void chooseOption(SelectionOption option);
    void startSelectedOption();

public:
    WorldSelectionState(GameStateManager& gsm, Systems::AssetManager& assets,
                        CharacterType character);
    ~WorldSelectionState() override = default;

    void init() override;
    void handleInput(const sf::Event& event) override;
    void update(sf::Time dt) override;
    void render(sf::RenderWindow& window) override;
};
