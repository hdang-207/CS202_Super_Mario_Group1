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
public:
    enum class MenuPhase {
        TitleScreen,
        ActionMenu
    };

private:
    sf::Sprite bgSprite;           ///< Background menu sprite
    sf::RectangleShape titleBox;   ///< Frame/box container for game title
    sf::Text titleText;            ///< Text display for main game title
    sf::Text promptText;           ///< Text prompt instructing user to press key
    float blinkTimer = 0.0f;       ///< Timer for blinking text effect
    bool showPrompt = true;        ///< Toggle visibility of prompt text

    MenuPhase currentPhase{MenuPhase::TitleScreen};
    sf::RectangleShape modalOverlay;
    sf::Text promptHintText;
    std::vector<sf::Text> menuTexts;
    std::vector<std::string> menuLabels;
    int selectedIndex{0};
    bool hasSaveAvailable{false};

    void updateMenuLabels();
    void refreshSaveStatus();

public:
    /**
     * @brief Constructor for IntroMenuState.
     * @param gsm Reference to the GameStateManager.
     * @param assets Reference to the central AssetManager.
     */
    IntroMenuState(GameStateManager& gsm, Systems::AssetManager& assets);

    /**
     * @brief Destructor.
     */
    ~IntroMenuState() override = default;

    /**
     * @brief Initializes the intro menu (loads assets and sets up UI elements).
     */
    void init() override;

    /**
     * @brief Listens for keyboard input to navigate between frames and select options.
     * @param event The event being polled.
     */
    void handleInput(const sf::Event& event) override;

    /**
     * @brief Updates menu animations or blinking timers.
     * @param dt Time elapsed since last frame.
     */
    void update(sf::Time dt) override;

    /**
     * @brief Renders the intro menu background and graphical elements.
     * @param window Graphical window to draw into.
     */
    void render(sf::RenderWindow& window) override;
};
