#pragma once

#include "Core/CharacterType.hpp"
#include "Core/GameMode.hpp"
#include <string>

/**
 * @struct SaveData
 * @brief Encapsulates the game progress data for serialization and deserialization.
 */
struct SaveData {
    int currentLevel{1};                   ///< Stable save index (1 through Config::kFinalLevel).
    int score{0};                          ///< Current player score
    int coins{0};                          ///< Total coins collected
    int lives{3};                          ///< Remaining player lives
    CharacterType selectedCharacter{CharacterType::Mario}; ///< Selected character (Mario or Luigi)
    bool world13OneUpUnlocked{false};      ///< Unlocks World 2-1's hidden 1-Up after collecting 21 coins in World 1-3.
    bool world23AllCoinsCollected{false};  ///< Unlocks World 3-1's hidden 1-Up after a perfect World 2-3 clear.
    GameMode gameMode{GameMode::Normal};   ///< The selected game mode (Normal, Nightfall, Inferno).
};
