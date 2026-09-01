#include "UI/NightfallOverlay.hpp"
#include <cmath>

namespace {
    constexpr float kPi = 3.14159265358979f;
}

namespace UI {

NightfallOverlay::NightfallOverlay(float viewWidth, float viewHeight,
                                   float lightRadius, int segments)
    : m_viewWidth(viewWidth), m_viewHeight(viewHeight),
      m_lightRadius(lightRadius), m_segments(segments) {}

void NightfallOverlay::ensureMaskTexture() {
    if (!m_maskTexture.has_value()) {
        m_maskTexture.emplace(sf::Vector2u{
            static_cast<unsigned>(m_viewWidth),
            static_cast<unsigned>(m_viewHeight)
        });
    }
}

void NightfallOverlay::draw(sf::RenderTarget& target, sf::Vector2f lightCenter,
                            const sf::View& gameView) {
    ensureMaskTexture();
    auto& mask = m_maskTexture.value();

    // --- Step 1: Fill the mask texture with near-opaque black ---
    mask.clear(sf::Color(0, 0, 0, 255));

    // --- Step 2: Compute the light position in mask-texture space ---
    // The mask texture uses a simple identity view matching the game view size.
    // Convert world-space lightCenter to a position relative to the game view.
    const sf::Vector2f viewCenter = gameView.getCenter();
    const sf::Vector2f viewSize = gameView.getSize();
    const sf::Vector2f screenPos = {
        (lightCenter.x - viewCenter.x + viewSize.x / 2.f) * (m_viewWidth / viewSize.x),
        (lightCenter.y - viewCenter.y + viewSize.y / 2.f) * (m_viewHeight / viewSize.y)
    };

    // Set an identity view on the mask texture
    sf::View maskView(sf::FloatRect({0.f, 0.f}, {m_viewWidth, m_viewHeight}));
    mask.setView(maskView);

    // --- Step 3: Punch a radial gradient hole using TriangleFan + BlendNone ---
    // Center = fully transparent (light), rim = fully opaque (matches darkness).
    // BlendNone directly replaces the mask pixels with the vertex colors.
    sf::VertexArray fan(sf::PrimitiveType::TriangleFan, m_segments + 2);

    // Center vertex: fully transparent → reveals the game underneath
    fan[0].position = screenPos;
    fan[0].color = sf::Color(0, 0, 0, 0);

    const float angleStep = 2.f * kPi / static_cast<float>(m_segments);
    for (int i = 0; i <= m_segments; ++i) {
        const float angle = static_cast<float>(i) * angleStep;
        fan[i + 1].position = {
            screenPos.x + std::cos(angle) * m_lightRadius,
            screenPos.y + std::sin(angle) * m_lightRadius
        };
        // Rim: matches the surrounding darkness
        fan[i + 1].color = sf::Color(0, 0, 0, 255);
    }

    mask.draw(fan, sf::BlendNone);
    mask.display();

    // --- Step 4: Draw the mask onto the main window ---
    // BlendMultiply darkens the scene: white areas are unchanged, black areas become dark.
    // Since our mask has alpha-based darkness, we use BlendAlpha (default) so the
    // transparent center lets the scene show through and the opaque edges darken it.
    const sf::View savedView = target.getView();

    // Set the same viewport as the game view so the overlay aligns perfectly
    sf::View overlayView(sf::FloatRect({0.f, 0.f}, {m_viewWidth, m_viewHeight}));
    overlayView.setViewport(gameView.getViewport());
    target.setView(overlayView);

    sf::Sprite maskSprite(mask.getTexture());
    target.draw(maskSprite, sf::BlendAlpha);

    target.setView(savedView);
}

} // namespace UI
