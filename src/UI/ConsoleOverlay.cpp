#include "UI/ConsoleOverlay.hpp"

namespace UI {

ConsoleOverlay::ConsoleOverlay() {
    background.setFillColor(sf::Color(0, 0, 0, 180)); // Semi-transparent black
    
    addOutput("Super Mario Bros. Debug Console");
    addOutput("Type 'help' for a list of commands.");
}

void ConsoleOverlay::toggle() {
    active = !active;
    if (active) {
        inputBuffer.clear();
        historyIndex = -1;
    }
}

bool ConsoleOverlay::isActive() const {
    return active;
}

void ConsoleOverlay::handleTextInput(char32_t c) {
    if (!active) return;
    
    // Ignore control characters (like enter, backspace, tab, esc)
    if (c < 32 || c == 127) return;
    
    // Ignore the console toggle key ('/')
    if (c == '/') return;

    inputBuffer += static_cast<char>(c);
}

bool ConsoleOverlay::handleKeyPress(sf::Keyboard::Key key) {
    if (!active) return false;

    if (key == sf::Keyboard::Key::Enter) {
        if (!inputBuffer.empty()) {
            commandHistory.push_back(inputBuffer);
            historyIndex = -1;
            return true; // Signal that a command is ready
        }
    } else if (key == sf::Keyboard::Key::Backspace) {
        if (!inputBuffer.empty()) {
            inputBuffer.pop_back();
        }
    } else if (key == sf::Keyboard::Key::Up) {
        if (!commandHistory.empty()) {
            if (historyIndex == -1) {
                historyIndex = commandHistory.size() - 1;
            } else if (historyIndex > 0) {
                historyIndex--;
            }
            inputBuffer = commandHistory[historyIndex];
        }
    } else if (key == sf::Keyboard::Key::Down) {
        if (historyIndex != -1) {
            if (historyIndex < commandHistory.size() - 1) {
                historyIndex++;
                inputBuffer = commandHistory[historyIndex];
            } else {
                historyIndex = -1;
                inputBuffer.clear();
            }
        }
    }
    
    return false;
}

std::string ConsoleOverlay::getAndClearInput() {
    std::string cmd = inputBuffer;
    inputBuffer.clear();
    return cmd;
}

void ConsoleOverlay::addOutput(const std::string& msg) {
    history.push_back(msg);
    // Keep only last 8 lines
    if (history.size() > 8) {
        history.erase(history.begin());
    }
}

void ConsoleOverlay::render(sf::RenderWindow& window, const sf::Font& font, const sf::View& screenView) {
    if (!active) return;
    
    if (!inputText) {
        inputText.emplace(font, "");
        inputText->setCharacterSize(14);
        inputText->setFillColor(sf::Color::Yellow);
        
        historyText.emplace(font, "");
        historyText->setCharacterSize(12);
        historyText->setFillColor(sf::Color::Green);
    }
    
    inputText->setFont(font);
    historyText->setFont(font);

    // Position relative to the current view
    sf::Vector2f viewSize = screenView.getSize();
    sf::Vector2f viewCenter = screenView.getCenter();
    
    float consoleHeight = viewSize.y * 0.4f; // Bottom 40% of screen
    float consoleY = viewCenter.y + viewSize.y / 2.f - consoleHeight;
    float consoleX = viewCenter.x - viewSize.x / 2.f;
    
    background.setSize({viewSize.x, consoleHeight});
    background.setPosition({consoleX, consoleY});
    window.draw(background);
    
    // Draw history
    std::string histStr;
    for (const auto& line : history) {
        histStr += line + "\n";
    }
    historyText->setString(histStr);
    historyText->setPosition({consoleX + 10.f, consoleY + 10.f});
    window.draw(*historyText);
    
    // Draw input line
    // Add blinking cursor
    static sf::Clock cursorClock;
    bool showCursor = (cursorClock.getElapsedTime().asMilliseconds() % 1000) < 500;
    
    inputText->setString("> " + inputBuffer + (showCursor ? "|" : ""));
    inputText->setPosition({consoleX + 10.f, consoleY + consoleHeight - 25.f});
    window.draw(*inputText);
}

} // namespace UI
