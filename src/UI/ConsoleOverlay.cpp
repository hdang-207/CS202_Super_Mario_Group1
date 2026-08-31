#include "UI/ConsoleOverlay.hpp"
#include <algorithm>

namespace UI {

void ConsoleOverlay::toggle() {
    m_active = !m_active;
    if (m_active) {
        m_inputBuffer.clear();
        m_historyIndex = -1;
    }
}

void ConsoleOverlay::handleTextInput(char32_t c) {
    if (!m_active) return;
    if (c < 32 || c == 127) return;   // control chars
    if (c == '/') return;             // ignore toggle key echo
    m_inputBuffer += static_cast<char>(c);
}

bool ConsoleOverlay::handleKeyPress(sf::Keyboard::Scancode key) {
    if (!m_active) return false;

    if (key == sf::Keyboard::Scancode::Enter || key == sf::Keyboard::Scancode::NumpadEnter) {
        if (!m_inputBuffer.empty()) {
            m_commandHistory.push_back(m_inputBuffer);
            m_historyIndex = -1;
            return true;   // signal: command ready
        }
        return false;
    }

    if (key == sf::Keyboard::Scancode::Backspace) {
        if (!m_inputBuffer.empty()) m_inputBuffer.pop_back();
        return false;
    }

    // Browse command history with Up / Down
    if (key == sf::Keyboard::Scancode::Up) {
        if (!m_commandHistory.empty()) {
            if (m_historyIndex < 0) {
                m_historyIndex = static_cast<int>(m_commandHistory.size()) - 1;
            } else if (m_historyIndex > 0) {
                --m_historyIndex;
            }
            m_inputBuffer = m_commandHistory[static_cast<std::size_t>(m_historyIndex)];
        }
        return false;
    }
    if (key == sf::Keyboard::Scancode::Down) {
        if (m_historyIndex >= 0) {
            ++m_historyIndex;
            if (m_historyIndex >= static_cast<int>(m_commandHistory.size())) {
                m_historyIndex = -1;
                m_inputBuffer.clear();
            } else {
                m_inputBuffer = m_commandHistory[static_cast<std::size_t>(m_historyIndex)];
            }
        }
        return false;
    }

    return false;
}

std::string ConsoleOverlay::getAndClearInput() {
    std::string cmd = m_inputBuffer;
    m_inputBuffer.clear();
    return cmd;
}

void ConsoleOverlay::addOutput(const std::string& msg) {
    m_history.push_back(msg);
    while (m_history.size() > kMaxHistory) {
        m_history.pop_front();
    }
}

void ConsoleOverlay::render(sf::RenderWindow& window, const sf::Font& font,
                            const sf::View& gameView) {
    if (!m_active) return;

    // Lazy-init SFML text objects (SFML 3 requires font on construction)
    if (!m_inputText) {
        m_inputText.emplace(font, "");
        m_inputText->setCharacterSize(14);
        m_inputText->setFillColor(sf::Color::Yellow);

        m_historyText.emplace(font, "");
        m_historyText->setCharacterSize(12);
        m_historyText->setFillColor(sf::Color::Green);
    }
    m_inputText->setFont(font);
    m_historyText->setFont(font);

    sf::Vector2f viewSize  = gameView.getSize();
    sf::Vector2f viewCenter = gameView.getCenter();

    float consoleHeight = viewSize.y * 0.4f;
    float consoleY = viewCenter.y + viewSize.y / 2.f - consoleHeight;
    float consoleX = viewCenter.x - viewSize.x / 2.f;

    m_background.setSize({viewSize.x, consoleHeight});
    m_background.setPosition({consoleX, consoleY});
    m_background.setFillColor(sf::Color(0, 0, 0, 200));
    window.draw(m_background);

    // Build history string (show last N lines that fit)
    std::string histStr;
    for (const auto& line : m_history) {
        histStr += line + "\n";
    }
    m_historyText->setString(histStr);
    m_historyText->setPosition({consoleX + 10.f, consoleY + 10.f});
    window.draw(*m_historyText);

    // Input line with blinking cursor
    static sf::Clock cursorClock;
    bool showCursor = (cursorClock.getElapsedTime().asMilliseconds() % 1000) < 500;
    m_inputText->setString("> " + m_inputBuffer + (showCursor ? "|" : ""));
    m_inputText->setPosition({consoleX + 10.f, consoleY + consoleHeight - 25.f});
    window.draw(*m_inputText);
}

} // namespace UI
