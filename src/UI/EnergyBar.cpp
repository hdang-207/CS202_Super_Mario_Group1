#include "UI/EnergyBar.hpp"

#include <algorithm>
#include <cstdint>
#include <cmath>
#include <utility>

namespace UI {
namespace {

sf::Color scaledColor(sf::Color color, float scale) {
    const auto scaledChannel = [scale](std::uint8_t channel) {
        return static_cast<std::uint8_t>(std::clamp(
            static_cast<float>(channel) * scale,
            0.f,
            255.f
        ));
    };
    return {
        scaledChannel(color.r),
        scaledChannel(color.g),
        scaledChannel(color.b),
        color.a
    };
}

void rightAlign(sf::Text& text, float x) {
    const sf::FloatRect bounds = text.getLocalBounds();
    text.setOrigin({bounds.position.x + bounds.size.x, 0.f});
    text.setPosition({x, text.getPosition().y});
}

} // namespace

void EnergyBar::init(
    const sf::Font& font,
    std::string playerLabel,
    sf::Vector2f position,
    sf::Color energyColor,
    bool mirrored,
    MeterIcon icon,
    MeterSize size
) {
    label = std::move(playerLabel);
    barPosition = position;
    fillColor = energyColor;
    isMirrored = mirrored;
    iconType = icon;
    meterSize = size;

    const bool compact = meterSize == MeterSize::Compact;
    labelText.emplace(font, "", compact ? 13u : 18u);
    labelText->setFillColor(sf::Color::White);
    labelText->setOutlineColor(sf::Color::Black);
    labelText->setOutlineThickness(2.f);

    valueText.emplace(font, "", compact ? 12u : 16u);
    valueText->setFillColor(sf::Color::White);
    valueText->setOutlineColor(sf::Color::Black);
    valueText->setOutlineThickness(2.f);

    setMaximum(kDefaultMaximum);
    setEnergy(kDefaultMaximum);
}

void EnergyBar::setEnergy(float energy) {
    currentEnergy = std::clamp(energy, 0.f, maximumEnergy);
    refreshText();
}

void EnergyBar::addEnergy(float amount) {
    setEnergy(currentEnergy + amount);
}

void EnergyBar::setMaximum(float maximum) {
    maximumEnergy = std::max(1.f, maximum);
    currentEnergy = std::clamp(currentEnergy, 0.f, maximumEnergy);
    refreshText();
}

void EnergyBar::refreshText() {
    if (!labelText || !valueText) {
        return;
    }

    const float percentage = maximumEnergy > 0.f
        ? currentEnergy / maximumEnergy * 100.f
        : 0.f;
    labelText->setString(label);
    valueText->setString(std::to_string(static_cast<int>(std::round(percentage))) + "%");

    const float radius = iconRadius();
    const float textY = barPosition.y - (meterSize == MeterSize::Compact ? 20.f : 28.f);
    labelText->setOrigin({0.f, 0.f});
    valueText->setOrigin({0.f, 0.f});
    labelText->setPosition({barPosition.x + radius + 4.f, textY});
    valueText->setPosition({barPosition.x + kBarWidth - 5.f, textY + 2.f});

    if (isMirrored) {
        valueText->setPosition({barPosition.x + 5.f, textY + 2.f});
        labelText->setPosition({barPosition.x + kBarWidth - radius - 4.f, textY});
        rightAlign(*labelText, barPosition.x + kBarWidth - radius - 4.f);
    } else {
        rightAlign(*valueText, barPosition.x + kBarWidth - 5.f);
    }
}

void EnergyBar::render(sf::RenderTarget& target) const {
    renderTrack(target);
    renderSegments(target);
    renderIcon(target);
    if (labelText) {
        target.draw(*labelText);
    }
    if (valueText) {
        target.draw(*valueText);
    }
}

void EnergyBar::renderTrack(sf::RenderTarget& target) const {
    const float height = barHeight();
    const float outlineThickness = meterSize == MeterSize::Compact ? 3.f : 4.f;
    sf::RectangleShape shadow({kBarWidth, height});
    shadow.setPosition(barPosition + sf::Vector2f{4.f, 5.f});
    shadow.setFillColor(sf::Color(0, 0, 0, 105));
    target.draw(shadow);

    sf::ConvexShape arrow(3);
    const float arrowX = isMirrored ? barPosition.x - 12.f
                                    : barPosition.x + kBarWidth + 12.f;
    const float baseX = isMirrored ? barPosition.x : barPosition.x + kBarWidth;
    arrow.setPoint(0, {baseX, barPosition.y});
    arrow.setPoint(1, {arrowX, barPosition.y + height / 2.f});
    arrow.setPoint(2, {baseX, barPosition.y + height});
    arrow.setFillColor(sf::Color(27, 18, 14));
    arrow.setOutlineColor(sf::Color(7, 5, 4));
    arrow.setOutlineThickness(3.f);
    target.draw(arrow);

    sf::RectangleShape track({kBarWidth, height});
    track.setPosition(barPosition);
    track.setFillColor(sf::Color(27, 18, 14));
    track.setOutlineColor(sf::Color(7, 5, 4));
    track.setOutlineThickness(outlineThickness);
    target.draw(track);
}

void EnergyBar::renderSegments(sf::RenderTarget& target) const {
    const bool compact = meterSize == MeterSize::Compact;
    const float outerPadding = compact ? 4.f : 7.f;
    const float iconOverlap = iconRadius() - (compact ? 2.f : 3.f);
    const float segmentGap = compact ? 2.f : 3.f;

    const float contentX = barPosition.x + (isMirrored ? outerPadding : iconOverlap);
    const float contentWidth = kBarWidth - iconOverlap - outerPadding;
    const float segmentWidth =
        (contentWidth - segmentGap * static_cast<float>(kSegmentCount - 1))
        / static_cast<float>(kSegmentCount);
    const float segmentHeight = barHeight() - outerPadding * 2.f;
    const float energyInSegments = maximumEnergy > 0.f
        ? currentEnergy / maximumEnergy * static_cast<float>(kSegmentCount)
        : 0.f;

    for (int visualIndex = 0; visualIndex < kSegmentCount; ++visualIndex) {
        const float x = contentX
            + static_cast<float>(visualIndex) * (segmentWidth + segmentGap);
        const int logicalIndex = isMirrored
            ? kSegmentCount - 1 - visualIndex
            : visualIndex;
        const float filledFraction = std::clamp(
            energyInSegments - static_cast<float>(logicalIndex),
            0.f,
            1.f
        );

        sf::RectangleShape emptySegment({segmentWidth, segmentHeight});
        emptySegment.setPosition({x, barPosition.y + outerPadding});
        emptySegment.setFillColor(sf::Color(48, 36, 30));
        emptySegment.setOutlineColor(sf::Color(10, 7, 5));
        emptySegment.setOutlineThickness(1.f);
        target.draw(emptySegment);

        if (filledFraction <= 0.f) {
            continue;
        }

        const float filledWidth = segmentWidth * filledFraction;
        const float filledX = isMirrored ? x + segmentWidth - filledWidth : x;
        sf::RectangleShape filledSegment({filledWidth, segmentHeight});
        filledSegment.setPosition({filledX, barPosition.y + outerPadding});
        filledSegment.setFillColor(fillColor);
        target.draw(filledSegment);

        sf::RectangleShape highlight({filledWidth, compact ? 2.f : 3.f});
        highlight.setPosition({filledX, barPosition.y + outerPadding + 1.f});
        highlight.setFillColor(scaledColor(fillColor, 1.45f));
        target.draw(highlight);
    }
}

void EnergyBar::renderIcon(sf::RenderTarget& target) const {
    const float radius = iconRadius();
    const sf::Vector2f center{
        isMirrored ? barPosition.x + kBarWidth : barPosition.x,
        barPosition.y + barHeight() / 2.f
    };

    sf::CircleShape shadow(radius);
    shadow.setOrigin({radius, radius});
    shadow.setPosition(center + sf::Vector2f{4.f, 5.f});
    shadow.setFillColor(sf::Color(0, 0, 0, 110));
    target.draw(shadow);

    sf::CircleShape outer(radius);
    outer.setOrigin({radius, radius});
    outer.setPosition(center);
    outer.setFillColor(sf::Color(22, 14, 11));
    outer.setOutlineColor(sf::Color(7, 5, 4));
    outer.setOutlineThickness(3.f);
    target.draw(outer);

    const float innerRadius = radius - (meterSize == MeterSize::Compact ? 4.f : 6.f);
    sf::CircleShape inner(innerRadius);
    inner.setOrigin({innerRadius, innerRadius});
    inner.setPosition(center);
    inner.setFillColor(scaledColor(fillColor, 0.58f));
    inner.setOutlineColor(scaledColor(fillColor, 1.25f));
    inner.setOutlineThickness(2.f);
    target.draw(inner);

    const float symbolScale = radius / 31.f;
    if (iconType == MeterIcon::Heart) {
        const float lobeRadius = 9.f * symbolScale;
        sf::CircleShape leftLobe(lobeRadius);
        leftLobe.setOrigin({lobeRadius, lobeRadius});
        leftLobe.setPosition(center + sf::Vector2f{-8.f, -6.f} * symbolScale);
        leftLobe.setFillColor(sf::Color::White);
        target.draw(leftLobe);

        sf::CircleShape rightLobe(lobeRadius);
        rightLobe.setOrigin({lobeRadius, lobeRadius});
        rightLobe.setPosition(center + sf::Vector2f{8.f, -6.f} * symbolScale);
        rightLobe.setFillColor(sf::Color::White);
        target.draw(rightLobe);

        sf::ConvexShape heartPoint(3);
        heartPoint.setPoint(0, center + sf::Vector2f{-17.f, -4.f} * symbolScale);
        heartPoint.setPoint(1, center + sf::Vector2f{17.f, -4.f} * symbolScale);
        heartPoint.setPoint(2, center + sf::Vector2f{0.f, 20.f} * symbolScale);
        heartPoint.setFillColor(sf::Color::White);
        target.draw(heartPoint);
    } else {
        sf::ConvexShape bolt(7);
        bolt.setPoint(0, center + sf::Vector2f{-4.f, -19.f} * symbolScale);
        bolt.setPoint(1, center + sf::Vector2f{10.f, -19.f} * symbolScale);
        bolt.setPoint(2, center + sf::Vector2f{2.f, -4.f} * symbolScale);
        bolt.setPoint(3, center + sf::Vector2f{12.f, -4.f} * symbolScale);
        bolt.setPoint(4, center + sf::Vector2f{-10.f, 20.f} * symbolScale);
        bolt.setPoint(5, center + sf::Vector2f{-3.f, 4.f} * symbolScale);
        bolt.setPoint(6, center + sf::Vector2f{-13.f, 4.f} * symbolScale);
        bolt.setFillColor(sf::Color::White);
        bolt.setOutlineColor(sf::Color(255, 255, 255, 180));
        bolt.setOutlineThickness(1.f);
        target.draw(bolt);
    }
}

float EnergyBar::barHeight() const noexcept {
    return meterSize == MeterSize::Compact ? 18.f : 30.f;
}

float EnergyBar::iconRadius() const noexcept {
    return meterSize == MeterSize::Compact ? 18.f : 31.f;
}

} // namespace UI
