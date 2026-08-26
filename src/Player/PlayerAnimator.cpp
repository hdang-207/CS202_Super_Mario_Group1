#include "Player/PlayerAnimator.hpp"

#include <SFML/Graphics/RenderTarget.hpp>
#include <SFML/Graphics/Sprite.hpp>
#include <SFML/Graphics/Texture.hpp>
#include <algorithm>
#include <cmath>

#include "Core/Config.hpp"
#include "Systems/AssetManager.hpp"

namespace entity {
namespace {
    /// Cell order inside every sheet built by tools/build_character_sheets.py.
    constexpr int kFrameIdle = 0;
    constexpr int kFrameWalk1 = 1;
    constexpr int kFrameWalk2 = 2;
    constexpr int kFrameJump = 3;
    constexpr int kFrameCrouch = 4;

    /**
     * Walk cycle. Passing back through the standing pose between strides is
     * what gives the two drawn stride frames a readable gait, and puts the two
     * footfalls an even half-cycle apart.
     */
    constexpr int kWalkCycle[] = {kFrameIdle, kFrameWalk1, kFrameIdle, kFrameWalk2};
    constexpr int kWalkCycleLength = 4;

    /// Exact six-pose swimming cycle from the original NES character sheet.
    constexpr int kFrameSwimStart = 5;
    constexpr int kSwimCycleLength = 6;
    constexpr float kSwimFrameDuration = 0.16f;

    /// Legs move with the character: a slow shuffle, then a sprint.
    constexpr float kWalkFrameSlow = 0.15f;
    constexpr float kWalkFrameFast = 0.05f;

    constexpr float kTransformDuration = 0.6f;
    constexpr float kTransformFlashInterval = 0.06f;
    constexpr float kBlinkInterval = 0.05f;
    constexpr float kStarInterval = 0.06f;

    /// Multiplied over the artwork, so a Starman recolours it without new art.
    const sf::Color kStarTints[] = {
        sf::Color(255, 255, 255),
        sf::Color(255, 170, 170),
        sf::Color(180, 255, 180),
        sf::Color(170, 200, 255)
    };
    constexpr int kStarTintCount = 4;

    int formIndex(PlayerForm form) {
        return static_cast<int>(form);
    }
}

void PlayerAnimator::init(const Systems::AssetManager& assets, CharacterType character) {
    const std::string prefix = character == CharacterType::Mario ? "Mario" : "Luigi";
    m_sheets[formIndex(PlayerForm::Small)] = &assets.getTexture(prefix + "SmallSheet");
    m_sheets[formIndex(PlayerForm::Super)] = &assets.getTexture(prefix + "SuperSheet");
    m_sheets[formIndex(PlayerForm::Fire)] = &assets.getTexture(prefix + "FireSheet");
}

void PlayerAnimator::reset(PlayerForm form) {
    m_form = form;
    m_previousForm = form;
    m_action = PlayerAction::Idle;
    m_speedRatio = 0.f;
    m_walkTimer = 0.f;
    m_walkStep = 0;
    m_footstep = false;
    m_swimTimer = 0.f;
    m_swimStep = 0;
    m_transformRemaining = 0.f;
    m_transformFlashTimer = 0.f;
    m_showPreviousForm = false;
    m_blinking = false;
    m_blinkTimer = 0.f;
    m_hidden = false;
    m_starPower = false;
    m_starTimer = 0.f;
    m_starStep = 0;
}

void PlayerAnimator::setForm(PlayerForm form) {
    if (form == m_form) {
        return;
    }

    m_previousForm = m_form;
    m_form = form;
    m_transformRemaining = kTransformDuration;
    m_transformFlashTimer = 0.f;
    m_showPreviousForm = true;
}

void PlayerAnimator::setAction(PlayerAction action) noexcept {
    if (action == m_action) {
        return;
    }

    // Restarting the cycle on every change would leave a single-frame stutter
    // whenever Mario brushes a wall, so only the cycles themselves reset.
    if (action != PlayerAction::Walk) {
        m_walkTimer = 0.f;
        m_walkStep = 0;
    }
    if (action != PlayerAction::Swim) {
        m_swimTimer = 0.f;
        m_swimStep = 0;
    }
    m_action = action;
}

void PlayerAnimator::setSpeedRatio(float ratio) noexcept {
    m_speedRatio = std::clamp(ratio, 0.f, 1.f);
}

void PlayerAnimator::setBlinking(bool blinking) noexcept {
    m_blinking = blinking;
    if (!blinking) {
        m_blinkTimer = 0.f;
        m_hidden = false;
    }
}

bool PlayerAnimator::consumeFootstep() noexcept {
    const bool stepped = m_footstep;
    m_footstep = false;
    return stepped;
}

void PlayerAnimator::update(sf::Time dt) {
    const float seconds = dt.asSeconds();

    if (m_transformRemaining > 0.f) {
        m_transformRemaining -= seconds;
        m_transformFlashTimer += seconds;
        if (m_transformFlashTimer >= kTransformFlashInterval) {
            m_transformFlashTimer -= kTransformFlashInterval;
            m_showPreviousForm = !m_showPreviousForm;
        }
        if (m_transformRemaining <= 0.f) {
            m_transformRemaining = 0.f;
            m_showPreviousForm = false;
        }
    }

    if (m_action == PlayerAction::Walk) {
        const float frameDuration = kWalkFrameSlow
            - (kWalkFrameSlow - kWalkFrameFast) * m_speedRatio;
        m_walkTimer += seconds;
        while (m_walkTimer >= frameDuration) {
            m_walkTimer -= frameDuration;
            m_walkStep = (m_walkStep + 1) % kWalkCycleLength;
            // Both stride frames land a foot; the standing frames between them
            // are the lift, so only half the steps make a sound.
            m_footstep = m_footstep || m_walkStep % 2 == 1;
        }
    } else if (m_action == PlayerAction::Swim) {
        m_swimTimer += seconds;
        while (m_swimTimer >= kSwimFrameDuration) {
            m_swimTimer -= kSwimFrameDuration;
            m_swimStep = (m_swimStep + 1) % kSwimCycleLength;
        }
    }

    if (m_blinking) {
        m_blinkTimer += seconds;
        while (m_blinkTimer >= kBlinkInterval) {
            m_blinkTimer -= kBlinkInterval;
            m_hidden = !m_hidden;
        }
    }

    if (m_starPower) {
        m_starTimer += seconds;
        while (m_starTimer >= kStarInterval) {
            m_starTimer -= kStarInterval;
            m_starStep = (m_starStep + 1) % kStarTintCount;
        }
    } else {
        m_starTimer = 0.f;
        m_starStep = 0;
    }
}

const sf::Texture& PlayerAnimator::sheetOf(PlayerForm form) const {
    const sf::Texture* sheet = m_sheets[formIndex(form)];
    // init() is called before the first frame is drawn, so a missing sheet can
    // only mean the asset failed to load; fall back rather than dereference null.
    if (sheet == nullptr) {
        sheet = m_sheets[formIndex(PlayerForm::Small)];
    }
    return *sheet;
}

PlayerForm PlayerAnimator::displayedForm() const noexcept {
    return (m_transformRemaining > 0.f && m_showPreviousForm) ? m_previousForm : m_form;
}

int PlayerAnimator::frameIndex() const noexcept {
    switch (m_action) {
        case PlayerAction::Walk:
            return kWalkCycle[m_walkStep];
        case PlayerAction::Swim:
            return kFrameSwimStart + m_swimStep;
        case PlayerAction::Crouch:
            return kFrameCrouch;
        // The arm-up pose doubles as the skid and the death frame, which is how
        // it reads in the original artwork too.
        case PlayerAction::Skid:
        case PlayerAction::Jump:
        case PlayerAction::Fall:
        case PlayerAction::Dead:
            return kFrameJump;
        case PlayerAction::Idle:
            break;
    }
    return kFrameIdle;
}

sf::Color PlayerAnimator::tint() const {
    return m_starPower ? kStarTints[m_starStep] : sf::Color::White;
}

void PlayerAnimator::draw(sf::RenderTarget& target, sf::Vector2f feetCentre) const {
    if (m_hidden) {
        return;
    }

    const sf::Texture& sheet = sheetOf(displayedForm());
    const int cellWidth = static_cast<int>(sheet.getSize().x) / kFrameCount;
    const int cellHeight = static_cast<int>(sheet.getSize().y);

    sf::Sprite sprite(sheet);
    sprite.setTextureRect(sf::IntRect({frameIndex() * cellWidth, 0}, {cellWidth, cellHeight}));
    sprite.setOrigin({cellWidth / 2.f, static_cast<float>(cellHeight)});
    // Whole-number zoom and a whole-number position: anything else lands the
    // artwork between screen pixels and the sprite shimmers as it moves.
    sprite.setScale({m_facingRight ? Config::kZoom : -Config::kZoom, Config::kZoom});
    sprite.setPosition({std::round(feetCentre.x), std::round(feetCentre.y)});
    sprite.setColor(tint());
    target.draw(sprite);
}

} // namespace entity
