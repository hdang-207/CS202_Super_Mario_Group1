#pragma once
#include <string>
#include <vector>
#include <cstddef>

/**
 * @class MapParser
 * @brief Parses level text files into 2D character grid maps for game levels.
 */
class MapParser {
private:
    std::vector<std::vector<char>> grid;
    std::size_t width{0};
    std::size_t height{0};

public:
    MapParser() = default;

    /**
     * @brief Loads and parses a map file (.txt).
     * @param filepath Path to the text map file.
     * @return True if loaded successfully, false otherwise.
     */
    bool loadFromFile(const std::string& filepath);

    /**
     * @brief Returns the 2D grid matrix of characters.
     */
    const std::vector<std::vector<char>>& getGrid() const;

    /**
     * @brief Returns grid width in columns.
     */
    std::size_t getWidth() const;

    /**
     * @brief Returns grid height in rows.
     */
    std::size_t getHeight() const;

    /**
     * @brief Utility method to print map matrix to console for debugging.
     */
    void printMap() const;
};
