#ifndef QUEST_GIVER_INDICATOR_H_
#define QUEST_GIVER_INDICATOR_H_
#include "engine/render/IRenderer.h"
#include "engine/math/Rect.h"

namespace nccu {

// "!" overlay above quest-giver NPCs. Pure View-layer: takes the NPC's
// hit box (24x24, world coords) and draws a small panel-backed bang so
// the player can spot the dialog hook at a glance (H4). No raylib, no
// World access — every primitive flows through IRenderer, so the headless
// spy in test_quest_giver_indicator.cpp can record it. Drawn INSIDE the
// CameraScope by View so the indicator follows the NPC in world space.
//
// Layout: a 16x16 gold square (#FFC83D ≈ raylib Gold), centred over the
// hitBox top edge, lifted 20 px above the NPC sprite top (the sprite is
// bottom-anchored on the hitBox, 32-tall, so its top is hitBox.y - 8 in
// world coords). The "!" glyph sits in the middle of the square.
struct QuestGiverIndicatorLayout {
    nccu::engine::math::Rect panel;     // background square
    nccu::engine::math::Vec2 textPos;   // top-left of "!" within the panel
    int       textSize;  // font size for "!"
};

// Compute where the indicator should land for a given NPC hitBox.
// Extracted from DrawQuestGiverIndicator so the test can spot-check the
// geometry without mocking the renderer.
[[nodiscard]] inline QuestGiverIndicatorLayout
LayoutQuestGiverIndicator(nccu::engine::math::Rect hitBox) noexcept {
    constexpr float kSize    = 16.0f;
    constexpr float kLiftPx  = 20.0f;  // gap between sprite top & icon bottom
    constexpr float kSpriteH = 32.0f;  // Pipoya cell; NPC::Render anchor
    // The NPC sprite is bottom-anchored on hitBox (see NPC::Render): its
    // top sits at hitBox.y + hitBox.height - kSpriteH. Lift the icon
    // further up so it floats clear of the head.
    const float spriteTopY = hitBox.y + hitBox.height - kSpriteH;
    const float iconY      = spriteTopY - kLiftPx - kSize;
    const float iconX      = hitBox.x + (hitBox.width - kSize) * 0.5f;
    QuestGiverIndicatorLayout out{};
    out.panel    = nccu::engine::math::Rect{iconX, iconY, kSize, kSize};
    out.textSize = 14;
    // raylib's bitmap font: "!" is ~3 px wide at size 14, ~10 px tall.
    // Nudge into the panel so the glyph reads as centred.
    out.textPos  = nccu::engine::math::Vec2{iconX + 6.0f, iconY + 1.0f};
    return out;
}

// Paint the indicator through the injected IRenderer. The panel is
// drawn with a slight shadow underneath so it pops on bright tiles.
inline void DrawQuestGiverIndicator(nccu::engine::render::IRenderer& r,
                                    nccu::engine::math::Rect hitBox) {
    const QuestGiverIndicatorLayout L = LayoutQuestGiverIndicator(hitBox);
    // Drop shadow: same rect nudged 2 px SE, dark translucent.
    r.DrawRect(nccu::engine::math::Rect{L.panel.x + 2.0f, L.panel.y + 2.0f,
                         L.panel.width, L.panel.height},
               nccu::engine::math::Color{0, 0, 0, 140});
    // Gold square — readable on any tile.
    r.DrawRect(L.panel, nccu::engine::math::Color{255, 200, 61, 255});
    // Black "!" glyph centred in the square.
    r.DrawText("!", L.textPos, L.textSize, nccu::engine::math::Colors::Black);
}

} // namespace nccu

#endif // QUEST_GIVER_INDICATOR_H_
