#include "Systems/SaveManager.hpp"
#include "Core/Config.hpp"
#include <fstream>
#include <iostream>

namespace {
    bool isValidSave(const SaveData& data) {
        const bool validCharacter = data.selectedCharacter == CharacterType::Mario
                                 || data.selectedCharacter == CharacterType::Luigi;
        return data.currentLevel >= 1 && data.currentLevel <= Config::kFinalLevel
            && data.score >= 0 && data.coins >= 0 && data.coins < 100
            && data.lives > 0 && validCharacter;
    }
}

bool SaveManager::saveToFile(const std::string& filepath, const SaveData& data) {
    if (!isValidSave(data)) {
        std::cerr << "[SaveManager Error] Refusing to save invalid progress data.\n";
        return false;
    }

    std::ofstream outFile(filepath);
    if (!outFile.is_open()) {
        std::cerr << "[SaveManager Error] Failed to open save file for writing: " << filepath << std::endl;
        return false;
    }

    // Write progress properties line by line
    outFile << data.currentLevel << "\n";
    outFile << data.score << "\n";
    outFile << data.coins << "\n";
    outFile << data.lives << "\n";
    outFile << static_cast<int>(data.selectedCharacter) << "\n";
    outFile << static_cast<int>(data.world23AllCoinsCollected) << "\n";
    outFile << static_cast<int>(data.world13OneUpUnlocked) << "\n";
    outFile << static_cast<int>(data.gameMode) << "\n";

    outFile.close();
    std::cout << "[SaveManager] Successfully saved progress to: " << filepath << std::endl;
    return true;
}

bool SaveManager::loadFromFile(const std::string& filepath, SaveData& outData) {
    std::ifstream inFile(filepath);
    if (!inFile.is_open()) {
        std::cerr << "[SaveManager Error] Failed to open save file for reading: " << filepath << std::endl;
        return false;
    }

    SaveData loaded;
    int charTypeInt = 0;
    if (inFile >> loaded.currentLevel >> loaded.score >> loaded.coins
               >> loaded.lives >> charTypeInt) {
        loaded.selectedCharacter = static_cast<CharacterType>(charTypeInt);
        // The optional flags were added after the original five-field format.
        // Reaching EOF at either point is a valid legacy save and keeps that
        // bonus locked until its coin challenge is cleared in this playthrough.
        int world23AllCoinsInt = 0;
        bool optionalFieldValid = true;
        if (inFile >> world23AllCoinsInt) {
            optionalFieldValid = world23AllCoinsInt == 0 || world23AllCoinsInt == 1;
            loaded.world23AllCoinsCollected = world23AllCoinsInt == 1;

            int world13OneUpInt = 0;
            if (inFile >> world13OneUpInt) {
                optionalFieldValid = optionalFieldValid
                                  && (world13OneUpInt == 0 || world13OneUpInt == 1);
                loaded.world13OneUpUnlocked = world13OneUpInt == 1;

                int modeInt = 0;
                if (inFile >> modeInt) {
                    optionalFieldValid = optionalFieldValid
                                      && (modeInt >= 0 && modeInt <= 3);
                    loaded.gameMode = static_cast<GameMode>(modeInt);
                } else if (!inFile.eof()) {
                    optionalFieldValid = false;
                }
            } else if (!inFile.eof()) {
                optionalFieldValid = false;
            }
        } else if (!inFile.eof()) {
            optionalFieldValid = false;
        }

        if (optionalFieldValid && isValidSave(loaded)) {
            outData = loaded;
            inFile.close();
            std::cout << "[SaveManager] Successfully loaded progress from: "
                      << filepath << std::endl;
            return true;
        }
    }

    std::cerr << "[SaveManager Error] Invalid or corrupted save file format: " << filepath << std::endl;
    inFile.close();
    return false;
}

bool SaveManager::saveProgress(const std::string& filepath, const SaveData& data) {
    return saveToFile(filepath, data);
}

bool SaveManager::loadProgress(const std::string& filepath, SaveData& outData) {
    return loadFromFile(filepath, outData);
}

bool SaveManager::hasSaveFile(const std::string& filepath) {
    std::ifstream inFile(filepath);
    if (!inFile.is_open()) {
        return false;
    }
    inFile.close();
    SaveData dummy;
    return loadFromFile(filepath, dummy);
}

bool SaveManager::deleteSaveFile(const std::string& filepath) {
    if (std::remove(filepath.c_str()) == 0) {
        std::cout << "[SaveManager] Successfully deleted save file: " << filepath << std::endl;
        return true;
    }
    return false;
}
