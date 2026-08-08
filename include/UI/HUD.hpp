#pragma once
#include <SFML/Graphics.hpp>
#include "Core/CharacterType.hpp"
#include "Systems/AssetManager.hpp"
#include <string>

#include <optional>

namespace UI {

/**
 * @class HUD
 * @brief Heads-Up Display for gameplay, rendering Score, Coins, Time, World, and character name.
 */
class HUD {
private:
    std::optional<sf::Text> playerNameText;
    std::optional<sf::Text> scoreText;

    std::optional<sf::Sprite> coinSprite;
    std::optional<sf::Text> coinsText;

    std::optional<sf::Text> worldLabelText;
    std::optional<sf::Text> worldText;

    std::optional<sf::Text> timeLabelText;
    std::optional<sf::Text> timeText;

    std::optional<sf::Text> livesLabelText;
    std::optional<sf::Text> livesText;

    std::optional<sf::Text> ammoLabelText;
    std::optional<sf::Text> ammoText;

    int currentScore{0};
    int currentCoins{0};
    float timeRemaining{400.f};
    int currentLives{3};
    int currentAmmo{3};
    int maximumAmmo{3};

    /**
     * @brief Formats a number with leading zeros.
     */
    std::string formatNumber(int number, int width) const;

public:
    /**
     * @brief Default constructor
     */
    HUD() = default;

    /**
     * @brief Initializes HUD components.
     * @param assets The asset manager to load fonts and textures.
     * @param charType The initial character type (Mario/Luigi).
     */
    void init(Systems::AssetManager& assets, CharacterType charType);

    /**
     * @brief Updates the HUD, primarily the countdown timer.
     * @param dt Time delta.
     */
    void update(sf::Time dt);

    /**
     * @brief Renders the HUD to the window.
     * @param window The target window.
     */
    void render(sf::RenderWindow& window) const;

    // Setters for dynamic updating
    void setCharacter(CharacterType charType);
    void setScore(int score);
    void addScore(int amount);
    void setCoins(int coins);
    void addCoins(int amount);
    void setWorld(int level);
    void setTime(float time);
    void setLives(int lives);
    void addLives(int amount);
    void setAmmo(int ammo, int maximum);

    int getScore() const { return currentScore; }
    int getCoins() const { return currentCoins; }
    int getLives() const { return currentLives; }
    int getAmmo() const { return currentAmmo; }
    float getTime() const { return timeRemaining; }
};

} // namespace UI
