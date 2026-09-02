#pragma once

#include "States/DuelArenas.hpp"
#include "States/State.hpp"

#include <cstddef>
#include <vector>

/**
 * @class DuelArenaSelectionState
 * @brief Lets the two duellists pick their arena before the match starts.
 *
 * Each arena is shown as a minimap built straight from its map file, so the
 * screen never falls out of step with what the duel actually loads. The last
 * entry is a random roll rather than a map of its own.
 */
class DuelArenaSelectionState final : public State {
private:
    /// One arena drawn small: its blocks and the two spawn markers.
    struct ArenaPreview {
        bool loaded{false};
        std::vector<sf::RectangleShape> blocks;
        std::vector<sf::CircleShape> spawnMarkers;
    };

    std::size_t selectedChoice{0};
    std::vector<ArenaPreview> previews;

    sf::Sprite bgSprite;
    sf::RectangleShape darkOverlay;
    sf::RectangleShape previewPanel;
    sf::Text headerText;
    sf::Text arenaNameText;
    sf::Text arenaDescriptionText;
    sf::Text pagerText;
    sf::Text leftArrowText;
    sf::Text rightArrowText;
    sf::Text randomMarkText;
    sf::Text previewErrorText;
    sf::Text hintText;

    void buildPreviews();
    void chooseArena(std::size_t choice);
    void refreshSelectionText();
    void confirmSelection();
    void goBack();
    void playSelectSound();

public:
    DuelArenaSelectionState(
        GameStateManager& gsm,
        Systems::AssetManager& assets,
        std::size_t initiallySelected = 0
    );

    ~DuelArenaSelectionState() override = default;

    void init() override;
    void handleInput(const sf::Event& event) override;
    void update(sf::Time dt) override;
    void render(sf::RenderWindow& window) override;
};
