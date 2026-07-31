#include "Systems/SaveManager.hpp"
#include <fstream>
#include <iostream>

bool SaveManager::saveToFile(const std::string& filepath, const SaveData& data) {
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

    int charTypeInt = 0;
    if (inFile >> outData.currentLevel >> outData.score >> outData.coins >> outData.lives >> charTypeInt) {
        outData.selectedCharacter = static_cast<CharacterType>(charTypeInt);
        inFile.close();
        std::cout << "[SaveManager] Successfully loaded progress from: " << filepath << std::endl;
        return true;
    }

    std::cerr << "[SaveManager Error] Invalid or corrupted save file format: " << filepath << std::endl;
    inFile.close();
    return false;
}
