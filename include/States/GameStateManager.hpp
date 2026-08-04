#pragma once
#include <vector>
#include <memory>
#include <SFML/Graphics.hpp>
#include "State.hpp"

/**
 * @class GameStateManager
 * @brief Manages the state stack and execution of active game states.
 *
 * Utilizes the State Pattern to handle game transitions. To prevent segmentation faults/crashes
 * when a state requests to modify the active state stack (e.g. popping itself during input handling),
 * all modifications are queued as "pending changes" and processed at a safe point in the frame loop.
 */
class GameStateManager {
private:
    std::vector<std::unique_ptr<State>> states; ///< Stack of game states. Only the top state is updated and rendered.
    sf::RenderWindow* window{nullptr};         ///< Pointer to main window for coordinate mapping
    
    /// @brief Enum defining the kinds of modifications we can perform on the state stack.
    enum class Action {
        Push,   ///< Push a new state on top of the stack, pausing the previous top state.
        Pop,    ///< Pop the current top state, resuming the previous state if any.
        Change, ///< Replace the current top state with a new one.
        Clear   ///< Pop and clear all states in the stack.
    };

    /// @brief Representation of a state change request that is deferred.
    struct PendingChange {
        Action action;                  ///< Type of stack operation.
        std::unique_ptr<State> state;   ///< Target state (null for Pop/Clear).
    };

    std::vector<PendingChange> pendingChanges; ///< Queue of state changes waiting to be applied.

    /**
     * @brief Processes and applies all queued stack operations sequentially.
     * Called in processStateChanges().
     */
    void applyPendingChanges();

public:
    /**
     * @brief Default constructor.
     */
    GameStateManager() = default;

    void setWindow(sf::RenderWindow* win) { window = win; }
    sf::RenderWindow* getWindow() const { return window; }

    /**
     * @brief Default destructor. Clears all remaining active states.
     */
    ~GameStateManager() = default;

    /**
     * @brief Queues a push command for a new state.
     * @param state Unique pointer to the state to be pushed.
     */
    void pushState(std::unique_ptr<State> state);

    /**
     * @brief Queues a pop command to remove the current top state.
     */
    void popState();

    /**
     * @brief Queues a change command to swap the current top state with a new state.
     * @param state Unique pointer to the new state.
     */
    void changeState(std::unique_ptr<State> state);

    /**
     * @brief Queues a clear command to purge all states from the manager.
     */
    void clearStates();

    /**
     * @brief Forwards system/input events to the active state at the top of the stack.
     * @param event The SFML event to handle.
     */
    void handleInput(const sf::Event& event);

    /**
     * @brief Updates the active state at the top of the stack.
     * @param dt Time elapsed since the last frame.
     */
    void update(sf::Time dt);

    /**
     * @brief Draws the active state at the top of the stack.
     * @param window The window to render onto.
     */
    void render(sf::RenderWindow& window);

    /**
     * @brief Checks if there are no states currently loaded.
     * @return True if the stack is empty, false otherwise.
     */
    bool isEmpty() const;

    /**
     * @brief Public interface to trigger processing of queued transitions.
     * Typically called at the beginning of each frame in the main game loop.
     */
    void processStateChanges();
};

