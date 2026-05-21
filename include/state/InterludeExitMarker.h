#ifndef INTERLUDE_EXIT_MARKER_H_
#define INTERLUDE_EXIT_MARKER_H_
#include "state/InterludeExit.h"
#include "gfx/IRenderer.h"
#include "gfx/Rect.h"
#include <cmath>
#include <vector>

namespace nccu {

// H3 visual ground marker — the south-band exit (`InterludeExit.h`) was an
// invisible y>=1900 trigger; cycle 9.A.2 added the "市集中央。逛完後往南
// 離開" / "準備離開市集" toasts on the text/event track, with the visual
// track explicitly deferred to this PR. The marker is a horizontal dashed
// gold line drawn ACROSS the walkable Interlude corridor at the exit y, so
// the player sees the route the moment the camera frames the south band.
//
// Mirrors the H4 QuestGiverIndicator architecture: pure View-layer,
// renders through `IRenderer` only (no raylib include in this header),
// camera-aware (the View draws it INSIDE its CameraScope so the line
// follows world coordinates). The layout helper is split out so the
// regression test can pin the geometry without a GL context — same spy
// renderer idiom as `test_quest_giver_indicator.cpp`.
//
// Visual budget: gold #FFC83D (matches the H4 "!" panel) + a 2-px black
// drop shadow nudged SE so the dashes stay readable on any tile. Dashes
// are 40 px long with a 20 px gap (period 60 px). The optional `phase`
// parameter shifts the pattern horizontally so the line can be animated
// (the View ticks phase by DeltaSeconds * speed); a static phase of 0 is
// the deterministic test default.

// One dash rectangle in world coordinates. Pure geometry; no draw call.
struct InterludeExitMarkerDash {
    gfx::Rect rect;
};

// Layout output for the whole marker: every dash that lies inside the
// south-band corridor. The View renders each dash via DrawRect — keeping
// the per-dash primitive small means tests can assert "at least one dash
// in the band" without a fragile per-pixel comparison.
struct InterludeExitMarkerLayout {
    std::vector<InterludeExitMarkerDash> dashes;
};

// Visual constants — kept inline so both the layout helper and the test
// can read them without a translation-unit dance.
inline constexpr float kInterludeMarkerThickness = 4.0f;
inline constexpr float kInterludeMarkerDashLen   = 40.0f;
inline constexpr float kInterludeMarkerGapLen    = 20.0f;
// Gold #FFC83D, matches the H4 quest-giver indicator panel.
inline constexpr gfx::Color kInterludeMarkerGold{255, 200, 61, 255};
// Drop-shadow tint, translucent so it never punches a solid hole on the
// underlying tiles.
inline constexpr gfx::Color kInterludeMarkerShadow{0, 0, 0, 140};

// Build the dash list. `phase` is in pixels; passing 0 yields the
// deterministic, frame-independent layout used by the unit test. The
// marker is drawn at y == kInterludeExitMinY, the *northern* edge of the
// exit zone — that way the line acts as a visible threshold rather than
// floating in the middle of the corridor.
[[nodiscard]] inline InterludeExitMarkerLayout
LayoutInterludeExitMarker(float phase = 0.0f) {
    InterludeExitMarkerLayout out{};
    const float period = kInterludeMarkerDashLen + kInterludeMarkerGapLen;
    // Wrap phase into [0, period) so a long-running game never overflows
    // and the animation stays well-defined.
    float p = std::fmod(phase, period);
    if (p < 0.0f) p += period;
    // First dash starts at minX - period + p so the leading edge sweeps
    // in from off-screen-left as phase increases. Clamp inside the band
    // on emission so partial dashes still draw something visible.
    const float y = kInterludeExitMinY;
    for (float x = kInterludeExitMinX - period + p;
         x < kInterludeExitMaxX;
         x += period) {
        float dashL = x;
        float dashR = x + kInterludeMarkerDashLen;
        if (dashR <= kInterludeExitMinX) continue;       // fully off west
        if (dashL >= kInterludeExitMaxX) break;          // fully off east
        if (dashL < kInterludeExitMinX) dashL = kInterludeExitMinX;
        if (dashR > kInterludeExitMaxX) dashR = kInterludeExitMaxX;
        out.dashes.push_back(InterludeExitMarkerDash{
            gfx::Rect{dashL, y, dashR - dashL, kInterludeMarkerThickness}});
    }
    return out;
}

// Paint the marker through the injected IRenderer. Each dash gets a
// drop shadow (2 px SE) and the gold body — the same two-pass idiom the
// quest-giver indicator uses, so the line stays readable on the bright
// road tiles south of the market.
inline void DrawInterludeExitMarker(gfx::IRenderer& r, float phase = 0.0f) {
    const InterludeExitMarkerLayout L = LayoutInterludeExitMarker(phase);
    for (const InterludeExitMarkerDash& d : L.dashes) {
        r.DrawRect(gfx::Rect{d.rect.x + 2.0f, d.rect.y + 2.0f,
                             d.rect.width, d.rect.height},
                   kInterludeMarkerShadow);
        r.DrawRect(d.rect, kInterludeMarkerGold);
    }
}

} // namespace nccu

#endif // INTERLUDE_EXIT_MARKER_H_
