#include "UI/HUD.hpp"
#include "Core/Config.hpp"
#include "Core/EventSystem.hpp"
#include <iomanip>
#include <sstream>

namespace UI {

std::string HUD::formatNumber(int number, int width) const {
    std::ostringstream ss;
    ss << std::setw(width) << std::setfill('0') << number;
    return ss.str();
}

void HUD::init(Systems::AssetManager& assets, CharacterType charType) {
    const sf::Font& font = assets.getFont("MarioFont");
    
    unsigned int fontSize = 24;
    float topMargin = 20.f;
    
    // 1. Player Name & Score (Left)
    playerNameText.emplace(font, "", fontSize);
    playerNameText->setFillColor(sf::Color::White);
    playerNameText->setPosition({80.f, topMargin});
    setCharacter(charType);
    
    scoreText.emplace(font, "", fontSize);
    scoreText->setFillColor(sf::Color::White);
    scoreText->setPosition({80.f, topMargin + 24.f});
    setScore(0);
    
    // 2. Coins (Center-Left)
    coinSprite.emplace(assets.getTexture("CoinIcon"));
    // Scale coin to be slightly larger and prominent
    float coinScale = 28.f / coinSprite->getLocalBounds().size.y;
    coinSprite->setScale({coinScale, coinScale});
    coinSprite->setPosition({295.f, topMargin + 22.f});
    
    coinsText.emplace(font, "", fontSize);
    coinsText->setFillColor(sf::Color::White);
    coinsText->setPosition({330.f, topMargin + 24.f});
    setCoins(0);
    
    // 3. World (Center-Right)
    worldLabelText.emplace(font, "WORLD", fontSize);
    worldLabelText->setFillColor(sf::Color::White);
    worldLabelText->setPosition({600.f, topMargin});
    
    worldText.emplace(font, "1-1", fontSize);
    worldText->setFillColor(sf::Color::White);
    worldText->setPosition({625.f, topMargin + 24.f});
    
    // 4. Time (Center-Right to Right)
    timeLabelText.emplace(font, "TIME", fontSize);
    timeLabelText->setFillColor(sf::Color::White);
    timeLabelText->setPosition({850.f, topMargin});
    
    timeText.emplace(font, "", fontSize);
    timeText->setFillColor(sf::Color::White);
    timeText->setPosition({865.f, topMargin + 24.f});
    setTime(400.f);
    
    // 5. Lives (Far Right)
    livesLabelText.emplace(font, "LIVES", fontSize);
    livesLabelText->setFillColor(sf::Color::White);
    livesLabelText->setPosition({1050.f, topMargin});
    
    livesText.emplace(font, "", fontSize);
    livesText->setFillColor(sf::Color::White);
    livesText->setPosition({1075.f, topMargin + 24.f});
    setLives(3);
}

void HUD::setCharacter(CharacterType charType) {
    if (playerNameText) {
        if (charType == CharacterType::Mario) {
            playerNameText->setString("MARIO");
        } else {
            playerNameText->setString("LUIGI");
        }
    }
}

void HUD::setScore(int score) {
    currentScore = score;
    if (scoreText) {
        scoreText->setString(formatNumber(currentScore, 6));
    }
}

void HUD::addScore(int amount) {
    setScore(currentScore + amount);
}

void HUD::setCoins(int coins) {
    currentCoins = coins;
    if (coinsText) {
        coinsText->setString("x" + formatNumber(currentCoins, 2));
    }
}

void HUD::addCoins(int amount) {
    currentCoins += amount;
    if (currentCoins >= 100) {
        currentCoins -= 100;
        // In the future: EventSystem::broadcast(PlayerEarnedLife)
    }
    setCoins(currentCoins);
}

void HUD::setTime(float time) {
    timeRemaining = time;
    if (timeRemaining < 0.f) timeRemaining = 0.f;
    if (timeText) {
        timeText->setString(formatNumber(static_cast<int>(timeRemaining), 3));
    }
}

void HUD::setLives(int lives) {
    currentLives = lives;
    if (currentLives < 0) currentLives = 0;
    if (livesText) {
        livesText->setString(formatNumber(currentLives, 2));
    }
}

void HUD::addLives(int amount) {
    setLives(currentLives + amount);
}

void HUD::update(sf::Time dt) {
    if (timeRemaining > 0.f) {
        timeRemaining -= dt.asSeconds() * 2.5f; // Fast mario time
        setTime(timeRemaining);
    }
}

void HUD::render(sf::RenderWindow& window) const {
    if (playerNameText) window.draw(*playerNameText);
    if (scoreText) window.draw(*scoreText);
    
    if (coinSprite) window.draw(*coinSprite);
    if (coinsText) window.draw(*coinsText);
    
    if (worldLabelText) window.draw(*worldLabelText);
    if (worldText) window.draw(*worldText);
    
    if (timeLabelText) window.draw(*timeLabelText);
    if (timeText) window.draw(*timeText);
    
    if (livesLabelText) window.draw(*livesLabelText);
    if (livesText) window.draw(*livesText);
}

} // namespace UI
