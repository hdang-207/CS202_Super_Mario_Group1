#pragma once

#include <SFML/Graphics.hpp>
#include <optional>
#include <string>

namespace UI {

enum class MeterIcon {
    Lightning,
    Heart
};

enum class MeterSize {
    Regular,
    Compact
};

/**
 * @brief A configurable segmented status meter used by the Duel HUD.
 *
 * The mirrored variant keeps its remaining energy next to the player's outer
 * screen edge, so the two meters drain away from one another like a fighting
 * game HUD.
 */
class EnergyBar {
public:
    static constexpr float kDefaultMaximum = 100.f;
    static constexpr float width() noexcept { return kBarWidth; }

    EnergyBar() = default;

    void init(
        const sf::Font& font,
        std::string playerLabel,
        sf::Vector2f position,
        sf::Color energyColor,
        bool mirrored = false,
        MeterIcon icon = MeterIcon::Lightning,
        MeterSize size = MeterSize::Regular
    );

    void setEnergy(float energy);
    void addEnergy(float amount);
    void setMaximum(float maximum);

    [[nodiscard]] float getEnergy() const noexcept { return currentEnergy; }
    [[nodiscard]] float getMaximum() const noexcept { return maximumEnergy; }

    void render(sf::RenderTarget& target) const;

private:
    static constexpr int kSegmentCount = 10;
    static constexpr float kBarWidth = 410.f;

    sf::Vector2f barPosition{};
    sf::Color fillColor{sf::Color::Yellow};
    bool isMirrored{false};
    MeterIcon iconType{MeterIcon::Lightning};
    MeterSize meterSize{MeterSize::Regular};
    float currentEnergy{kDefaultMaximum};
    float maximumEnergy{kDefaultMaximum};
    std::string label;
    std::optional<sf::Text> labelText;
    std::optional<sf::Text> valueText;

    void refreshText();
    void renderTrack(sf::RenderTarget& target) const;
    void renderSegments(sf::RenderTarget& target) const;
    void renderIcon(sf::RenderTarget& target) const;
    [[nodiscard]] float barHeight() const noexcept;
    [[nodiscard]] float iconRadius() const noexcept;
};

} // namespace UI
