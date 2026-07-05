#pragma once
#include <SFML/Graphics.hpp>

class GameStateManager;

/**
 * @class State
 * @brief Abstract base class for all game states.
 *
 * Defines the contract/interface for different screens or states of the game
 * (e.g. Menu, Selection, Gameplay). Each state retains a reference to the GameStateManager
 * to trigger state transitions (push, pop, change).
 */
class State {
protected:
    GameStateManager& gsm; ///< Reference to the State Manager for controlling navigation/flow.

public:
    /**
     * @brief Constructor that binds this state to a GameStateManager.
     * @param gsm Reference to the parent GameStateManager.
     */
    State(GameStateManager& gsm) : gsm(gsm) {}

    /**
     * @brief Virtual destructor to ensure correct cleanup of derived classes.
     */
    virtual ~State() = default;

    /**
     * @brief Initializes resources, loads assets, texture settings, etc. when the state starts.
     */
    virtual void init() = 0;

    /**
     * @brief Handles user input event.
     * @param event The SFML event containing event data (key pressed, mouse moved, etc.).
     */
    virtual void handleInput(sf::Event& event) = 0;

    /**
     * @brief Updates the logic of this state.
     * @param dt Time elapsed since the last update.
     */
    virtual void update(sf::Time dt) = 0;

    /**
     * @brief Renders the state contents onto the screen.
     * @param window Reference to the render window to draw objects onto.
     */
    virtual void render(sf::RenderWindow& window) = 0;

    /**
     * @brief Pauses state execution (called when a new state is pushed on top of this one).
     */
    virtual void pause() {}

    /**
     * @brief Resumes state execution (called when the state on top of this one is removed).
     */
    virtual void resume() {}
};

