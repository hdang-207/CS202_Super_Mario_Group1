#pragma once

#include <SFML/Graphics/Color.hpp>
#include <SFML/System/Time.hpp>
#include <SFML/System/Vector2.hpp>

#include "Core/CharacterType.hpp"

namespace sf {
class RenderTarget;
class Texture;
}

namespace Systems {
class AssetManager;
}

namespace entity {

/// Which sprite sheet the avatar is drawn from; mirrors the power-up ladder.
enum class PlayerForm {
    Small,
    Super,
    Fire
};

/**
 * @brief What the avatar is visibly doing.
 *
 * Picked from the physics body rather than from the keys held, so the picture
 * always agrees with where the player actually is - walking into a wall shows
 * him standing still, not running on the spot.
 */
enum class PlayerAction {
    Idle,
    Walk,
    Skid,   ///< Turning around at speed: the classic heels-first pose.
    Jump,
    Fall,
    Crouch,
    Climb,  ///< Hauling up a vine, alternating the two gripping poses.
    Swim,
    Dead
};

/**
 * @class PlayerAnimator
 * @brief Owns everything about how Mario/Luigi look: form, pose, and flashes.
 *
 * The sheets it reads are built by tools/build_character_sheets.py, which cuts
 * @ref kFrameCount equally sized cells out of the NES artwork and keeps each
 * pose at the offset the original artist drew it at. That is what lets the
 * avatar be drawn at the project's whole-number Config::kZoom with no per-frame
 * rescaling, so a Small Mario is exactly one tile tall and a Super Mario two,
 * and the walk cycle never wobbles: every frame already agrees with the rest
 * about where the ground and the centre line are.
 */
class PlayerAnimator {
public:
    /// Cells per sheet: nine land poses, followed by six NES swim poses.
    static constexpr int kFrameCount = 15;

    /// @brief Looks up the three form sheets belonging to one character.
    void init(const Systems::AssetManager& assets, CharacterType character);

    /// @brief Returns to a standing pose in @p form with every flash cleared.
    void reset(PlayerForm form);

    /**
     * @brief Switches sheets, playing the grow/shrink flash on the way.
     *
     * The flash is not decoration: the original game freezes play while it
     * runs, which is what stops a mushroom picked up under a low ceiling from
     * shoving Mario through it. Callers should hold off gameplay while
     * @ref isTransforming stays true.
     */
    void setForm(PlayerForm form);

    [[nodiscard]] PlayerForm getForm() const noexcept { return m_form; }

    void setAction(PlayerAction action) noexcept;

    [[nodiscard]] PlayerAction getAction() const noexcept { return m_action; }

    void setFacingRight(bool facingRight) noexcept { m_facingRight = facingRight; }

    [[nodiscard]] bool isFacingRight() const noexcept { return m_facingRight; }

    /// @brief 0 when stopped, 1 at top speed; sets how fast the legs move.
    void setSpeedRatio(float ratio) noexcept;

    /// @brief Blinks the avatar in and out, for the moments after taking a hit.
    void setBlinking(bool blinking) noexcept;

    /// @brief Cycles the avatar's colours while a Starman is running.
    void setStarPower(bool starPower) noexcept { m_starPower = starPower; }

    void update(sf::Time dt);

    /// @brief True while the grow/shrink flash still has frames left to play.
    [[nodiscard]] bool isTransforming() const noexcept { return m_transformRemaining > 0.f; }

    /// @brief Reports one footfall, clearing it, so the step sound plays once.
    [[nodiscard]] bool consumeFootstep() noexcept;

    /// @brief Draws the avatar standing on @p feetCentre, its feet centred there.
    void draw(sf::RenderTarget& target, sf::Vector2f feetCentre) const;

private:
    [[nodiscard]] const sf::Texture& sheetOf(PlayerForm form) const;

    /// @brief The form on screen right now, which alternates mid-transform.
    [[nodiscard]] PlayerForm displayedForm() const noexcept;

    [[nodiscard]] int frameIndex() const noexcept;

    [[nodiscard]] sf::Color tint() const;

    const sf::Texture* m_sheets[3]{nullptr, nullptr, nullptr};

    PlayerForm m_form{PlayerForm::Small};
    PlayerForm m_previousForm{PlayerForm::Small};
    PlayerAction m_action{PlayerAction::Idle};
    bool m_facingRight{true};
    float m_speedRatio{0.f};

    float m_walkTimer{0.f};
    int m_walkStep{0};
    bool m_footstep{false};

    float m_swimTimer{0.f};
    int m_swimStep{0};

    float m_climbTimer{0.f};
    int m_climbStep{0};

    float m_transformRemaining{0.f};
    float m_transformFlashTimer{0.f};
    bool m_showPreviousForm{false};

    bool m_blinking{false};
    float m_blinkTimer{0.f};
    bool m_hidden{false};

    bool m_starPower{false};
    float m_starTimer{0.f};
    int m_starStep{0};
};

} // namespace entity
