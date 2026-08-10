#pragma once
#include <SFML/Graphics.hpp>
#include <string>
#include <vector>
#include <optional>

namespace UI {

class ConsoleOverlay {
private:
    bool active{false};
    std::string inputBuffer;
    std::vector<std::string> history;
    std::vector<std::string> commandHistory;
    int historyIndex{-1};
    
    sf::RectangleShape background;
    std::optional<sf::Text> inputText;
    std::optional<sf::Text> historyText;

public:
    ConsoleOverlay();
    
    void toggle();
    bool isActive() const;
    
    void handleTextInput(char32_t c);
    // Returns true if a command was submitted
    bool handleKeyPress(sf::Keyboard::Key key);
    
    std::string getAndClearInput();
    void addOutput(const std::string& msg);
    
    void render(sf::RenderWindow& window, const sf::Font& font, const sf::View& screenView);
};

} // namespace UI
