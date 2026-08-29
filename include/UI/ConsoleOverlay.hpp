#pragma once
#include <SFML/Graphics.hpp>
#include <deque>
#include <optional>
#include <string>
#include <vector>

namespace UI {

class ConsoleOverlay {
public:
    ConsoleOverlay() = default;

    void toggle();
    [[nodiscard]] bool isActive() const noexcept { return m_active; }

    /// Feed a single Unicode character typed by the user.
    void handleTextInput(char32_t unicode);

    /// Handle special keys (Enter, Backspace, Up/Down history).
    /// Returns true when the user presses Enter (command submitted).
    bool handleKeyPress(sf::Keyboard::Scancode key);

    /// Retrieve the submitted command string and clear the input buffer.
    std::string getAndClearInput();

    /// Push a line of output (command result) into the console history.
    void addOutput(const std::string& msg);

    /// Draw the overlay on top of the current view.
    void render(sf::RenderWindow& window, const sf::Font& font, const sf::View& gameView);

private:
    bool m_active{false};
    std::string m_inputBuffer;
    std::deque<std::string> m_history;                 // output lines
    std::vector<std::string> m_commandHistory;         // previously entered commands
    int m_historyIndex{-1};                            // -1 = not browsing history
    static constexpr std::size_t kMaxHistory = 50;

    sf::RectangleShape m_background;
    std::optional<sf::Text> m_inputText;
    std::optional<sf::Text> m_historyText;
};

} // namespace UI
