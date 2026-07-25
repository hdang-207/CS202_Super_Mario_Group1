#pragma once
#include "States/State.hpp"

/**
 * @class IntroMenuState
 * @brief Represents the initial intro menu screen when the game is loaded.
 *
 * Implements the State interface. It prompts the user to press Enter to proceed
 * to character selection.
 */
class IntroMenuState : public State {
private:
    sf::Sprite bgSprite;           // Hình nền Menu
    sf::RectangleShape titleBox;  // Khung nền vuông cho Tên Game
    sf::Text titleText;
    sf::Music bgMusic;

public:
    /**
     * @brief Constructor for IntroMenuState.
     * @param gsm Reference to the GameStateManager.
     */
    IntroMenuState(GameStateManager& gsm, Systems::AssetManager& assets);

    /**
     * @brief Destructor.
     */
    ~IntroMenuState() override = default;

    /**
     * @brief Initializes the intro menu (prints welcome logs).
     */
    void init() override;

    /**
     * @brief Listens for Enter key press to transition to the character selection screen.
     * @param event The event being polled.
     */
    void handleInput(const sf::Event& event) override;

    /**
     * @brief Updates any menu animations or logic (currently skeleton placeholder).
     * @param dt Time elapsed since last frame.
     */
    void update(sf::Time dt) override;

    /**
     * @brief Renders the intro menu background and graphical elements.
     * @param window Graphical window to draw into.
     */
    void render(sf::RenderWindow& window) override;
};

