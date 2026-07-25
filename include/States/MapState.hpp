#pragma once
#include "States/State.hpp"
#include "Entities/Cloud.hpp"
#include <vector>

/**
 * @class MapState
 * @brief Represents the game world/level view.
 *
 * Work-in-progress skeleton for the level map. Currently renders the sky
 * background and decorative clouds; ground, tiles, and entities will be
 * layered in as the map is built out.
 */
class MapState : public State {
public:
    /**
     * @brief Constructor for MapState.
     * @param gsm Reference to the GameStateManager.
     */
    MapState(GameStateManager& gsm);

    /**
     * @brief Destructor.
     */
    ~MapState() override = default;

    /**
     * @brief Initializes the map, scattering background clouds.
     */
    void init() override;

    /**
     * @brief Handles input for the map (currently a skeleton placeholder).
     * @param event The event being polled.
     */
    void handleInput(const sf::Event& event) override;

    /**
     * @brief Updates map logic (currently a skeleton placeholder).
     * @param dt Time elapsed since last frame.
     */
    void update(sf::Time dt) override;

    /**
     * @brief Renders the sky background and clouds.
     * @param window Graphical window to draw into.
     */
    void render(sf::RenderWindow& window) override;

private:
    std::vector<Cloud> clouds; ///< Decorative background clouds.
};
