#pragma once
#include <SFML/Graphics.hpp>
#include "States/GameStateManager.hpp"
#include "Systems/AssetManager.hpp"

/**
 * @class Game
 * @brief Core engine class managing the main game loop, windows, and core subsystems.
 *
 * This class coordinates the game lifecycle. It initializes the SFML RenderWindow,
 * maintains a fixed-timestep game loop to ensure physics/logic update stability,
 * and delegates input, updating, and rendering to the GameStateManager.
 */
class Game {
private:
    sf::RenderWindow window; ///< Main graphical window of the application.
    Systems::AssetManager assets; 
    GameStateManager gsm;    ///< Manager handling transition and lifecycle of game states.
    
    /// Target time duration for each frame update (1/60th of a second).
    static const sf::Time TimePerFrame;

    /**
     * @brief Polls and handles all window events, delegating inputs to the active state.
     */
    void processEvents();

    /**
     * @brief Updates the game logic.
     * @param dt Time elapsed since the last update cycle.
     */
    void update(sf::Time dt);

    /**
     * @brief Renders the current state to the main window.
     */
    void render();

public:
    /**
     * @brief Constructor. Configures window settings, sets frame rate limits, and pushes the initial state.
     */
    Game();

    /**
     * @brief Destructor.
     */
    ~Game() = default;

    /**
     * @brief Starts and runs the main loop of the game.
     * Uses a fixed delta time update pattern to prevent performance-based physics/logic fluctuations.
     */
    void run();
};

