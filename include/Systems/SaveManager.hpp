#pragma once

#include "Systems/SaveData.hpp"
#include <string>

/**
 * @class SaveManager
 * @brief Decoupled system class responsible for saving and loading game progress data to/from disk files.
 */
class SaveManager {
public:
    SaveManager() = default;

    /**
     * @brief Saves game progress data to a text file.
     * @param filepath Target file path for the save file.
     * @param data SaveData structure containing progress attributes.
     * @return True if save operation succeeded, false otherwise.
     */
    static bool saveToFile(const std::string& filepath, const SaveData& data);

    /**
     * @brief Loads game progress data from a text file.
     * @param filepath Source file path of the save file.
     * @param outData Reference to SaveData structure where loaded data will be stored.
     * @return True if load operation succeeded and parsed correctly, false otherwise.
     */
    static bool loadFromFile(const std::string& filepath, SaveData& outData);

    /**
     * @brief Saves game progress data to disk (alias for Task 5.2 requirement).
     * @param filepath Target file path for the save file.
     * @param data SaveData structure containing progress attributes.
     * @return True if save operation succeeded, false otherwise.
     */
    static bool saveProgress(const std::string& filepath, const SaveData& data);

    /**
     * @brief Loads game progress data from disk (alias for Task 5.2 requirement).
     * @param filepath Source file path of the save file.
     * @param outData Reference to SaveData structure where loaded data will be stored.
     * @return True if load operation succeeded and parsed correctly, false otherwise.
     */
    static bool loadProgress(const std::string& filepath, SaveData& outData);

    /**
     * @brief Checks if a valid save file exists at the specified path.
     * @param filepath Target file path to check.
     * @return True if file exists and contains readable save data, false otherwise.
     */
    static bool hasSaveFile(const std::string& filepath);

    /**
     * @brief Deletes the save file at the specified path.
     * @param filepath Target file path to delete.
     * @return True if file was deleted successfully, false otherwise.
     */
    static bool deleteSaveFile(const std::string& filepath);
};
