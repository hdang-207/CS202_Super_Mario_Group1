#pragma once

#include "Core/CharacterType.hpp"
#include <string>

/**
 * @struct SaveData
 * @brief Encapsulates the game progress data for serialization and deserialization.
 */
struct SaveData {
    int currentLevel{1};                   ///< Current level number (1 or 2).
    int score{0};                          ///< Current player score
    int coins{0};                          ///< Total coins collected
    int lives{3};                          ///< Remaining player lives
    CharacterType selectedCharacter{CharacterType::Mario}; ///< Selected character (Mario or Luigi)
};
