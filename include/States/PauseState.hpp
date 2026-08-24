#pragma once
#include "States/State.hpp"
#include <SFML/Graphics/Text.hpp>
#include <SFML/Graphics/RectangleShape.hpp>
#include <vector>
#include <string>
#include <optional>

class PlayState;

/**
 * @class PauseState
 * @brief Interactive pause menu overlay state that allows resuming, adjusting BGM/SFX volume, saving/loading, and exiting to main menu.
 */
class PauseState : public State {
private:
    PlayState& playState;
    sf::RectangleShape bgOverlay;
    sf::Text titleText;
    sf::Text promptHintText;
    std::vector<sf::Text> menuTexts;
    std::vector<std::string> menuLabels;
    int selectedIndex{0};
    
    std::optional<sf::RectangleShape> saveNoticeBg;
    std::optional<sf::Text> saveNoticeText;
    float saveNoticeTimer{0.f};

    void updateMenuLabels();

public:
    PauseState(GameStateManager& gsm, Systems::AssetManager& assets, PlayState& playStateRef);
    ~PauseState() override = default;

    void init() override;
    void handleInput(const sf::Event& event) override;
    void update(sf::Time dt) override;
    void render(sf::RenderWindow& window) override;
};
