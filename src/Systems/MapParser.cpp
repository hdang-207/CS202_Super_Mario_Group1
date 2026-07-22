#include "Systems/MapParser.hpp"
#include <fstream>
#include <iostream>
#include <algorithm>

bool MapParser::loadFromFile(const std::string& filepath) {
    std::ifstream file(filepath);
    if (!file.is_open()) {
        std::cerr << "[MapParser] Error: Could not open map file: " << filepath << std::endl;
        return false;
    }

    grid.clear();
    width = 0;
    height = 0;

    std::string line;
    while (std::getline(file, line)) {
        // Strip trailing \r if any (for Windows carriage returns)
        if (!line.empty() && line.back() == '\r') {
            line.pop_back();
        }

        if (line.empty()) {
            continue; // Skip empty lines
        }

        std::vector<char> row(line.begin(), line.end());
        width = std::max(width, row.size());
        grid.push_back(std::move(row));
    }

    height = grid.size();
    file.close();

    std::cout << "[MapParser] Successfully loaded map: " << filepath 
              << " (" << width << "x" << height << " tiles)" << std::endl;
    return true;
}

const std::vector<std::vector<char>>& MapParser::getGrid() const {
    return grid;
}

std::size_t MapParser::getWidth() const {
    return width;
}

std::size_t MapParser::getHeight() const {
    return height;
}

void MapParser::printMap() const {
    std::cout << "[MapParser] Map Grid Output:\n";
    for (const auto& row : grid) {
        for (char tile : row) {
            std::cout << tile;
        }
        std::cout << "\n";
    }
}
