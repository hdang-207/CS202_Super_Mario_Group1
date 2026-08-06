#include "States/GameStateManager.hpp"
#include <iostream>

void GameStateManager::pushState(std::unique_ptr<State> state) {
    // Queue state push to prevent mid-frame modification crashes
    pendingChanges.push_back({Action::Push, std::move(state)});
}

void GameStateManager::popState() {
    // Queue state pop
    pendingChanges.push_back({Action::Pop, nullptr});
}

void GameStateManager::changeState(std::unique_ptr<State> state) {
    // Queue replacement of current state
    pendingChanges.push_back({Action::Change, std::move(state)});
}

void GameStateManager::clearStates() {
    // Queue clearing all states
    pendingChanges.push_back({Action::Clear, nullptr});
}

void GameStateManager::applyPendingChanges() {
    // State::init() is allowed to queue another transition. Work on a snapshot so
    // that such a request is safely deferred to the next frame instead of
    // invalidating this loop's iterators.
    std::vector<PendingChange> changes;
    changes.swap(pendingChanges);

    for (auto& change : changes) {
        switch (change.action) {
            case Action::Push:
                // Pause the previous top state if one exists
                if (!states.empty()) {
                    states.back()->pause();
                }
                states.push_back(std::move(change.state));
                states.back()->init(); // Initialize the new active state
                break;

            case Action::Pop:
                // Remove the active state from the stack
                if (!states.empty()) {
                    states.pop_back();
                }
                // Resume the newly exposed top state if one exists
                if (!states.empty()) {
                    states.back()->resume();
                }
                break;

            case Action::Change:
                // Swap the active state out
                if (!states.empty()) {
                    states.pop_back();
                }
                states.push_back(std::move(change.state));
                states.back()->init(); // Initialize the replacement state
                break;

            case Action::Clear:
                // Clear all states in the stack (automatically triggers destructors via unique_ptr)
                states.clear();
                break;
        }
    }
}

void GameStateManager::processStateChanges() {
    applyPendingChanges();
}

void GameStateManager::handleInput(const sf::Event& event) {
    // Forward input event to the top-most active state
    if (!states.empty()) {
        states.back()->handleInput(event);
    }
}

void GameStateManager::update(sf::Time dt) {
    // Forward update timestep to the top-most active state
    if (!states.empty()) {
        states.back()->update(dt);
    }
}

void GameStateManager::render(sf::RenderWindow& window) {
    // Forward rendering window to the top-most active state
    if (!states.empty()) {
        states.back()->render(window);
    }
}

bool GameStateManager::isEmpty() const {
    return states.empty();
}
