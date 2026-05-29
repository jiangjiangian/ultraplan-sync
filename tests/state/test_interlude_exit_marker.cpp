#include "doctest/doctest.h"
#include "game/state/InterludeExitMarker.h"
#include "game/state/InterludeExit.h"
#include "engine/render/IRenderer.h"

#include <string>
#include <vector>

namespace {

// Spy IRenderer — mirrors the CountingRenderer pattern in
// test_quest_giver_indicator.cpp / test_quest_pickup_render.cpp. Records
// every primitive so the View-side helper is testable headless (no GL
// context, no raylib draw call escapes into the world).
struct Spy final : nccu::gfx::IRenderer {
    struct RectCall { nccu::gfx::Rect r; nccu::gfx::Color c; };
    struct TextCall { std::string s; nccu::gfx::Vec2 pos; int size;
                      nccu::gfx::Color c; };
    std::vector<RectCall> rects;
    std::vector<TextCall> texts;
    int sprites = 0;

    void DrawRect(nccu::gfx::Rect r, nccu::gfx::Color c) override {
        rects.push_back({r, c});
    }
    void DrawSprite(const nccu::gfx::Texture&, nccu::gfx::Rect,
                    nccu::gfx::Rect, nccu::gfx::Color) override {
        ++sprites;
    }
    void DrawText(std::string_view text, nccu::gfx::Vec2 pos, int size,
                  nccu::gfx::Color c) override {
        texts.push_back({std::string{text}, pos, size, c});
    }
};

} // namespace

// H3 visual ground marker: the south-band exit zone (InterludeExit.h)
// was a silent y>=1900 trigger; cycle 9.A.2 wired the text/event
// feedback but explicitly deferred the visual to this PR. These tests
// pin down the pure layout helper and the IRenderer paint path so the
// regression cannot creep back in.

TEST_CASE("InterludeExitMarker: layout produces dashes along the band") {
    // The deterministic phase-0 layout must produce at least one dash;
    // a regression that returned an empty list (zero-step loop, off-by-one
    // boundary) would silently void the entire visual.
    const auto L = nccu::LayoutInterludeExitMarker(0.0f);
    REQUIRE(!L.dashes.empty());

    // Every dash lives at the NORTH edge of the exit zone (y == minY),
    // inside the corridor's x range, and uses the configured thickness.
    // A single off-band dash would mean the marker is pointing at the
    // wrong line — the visual contract.
    for (const auto& d : L.dashes) {
        CHECK(d.rect.y == doctest::Approx(nccu::kInterludeExitMinY));
        CHECK(d.rect.height == doctest::Approx(
            nccu::kInterludeMarkerThickness));
        CHECK(d.rect.x >= nccu::kInterludeExitMinX);
        CHECK(d.rect.x + d.rect.width <= nccu::kInterludeExitMaxX);
        // A clipped dash can be shorter than dashLen but never longer.
        CHECK(d.rect.width <= nccu::kInterludeMarkerDashLen +
                              0.001f);
        CHECK(d.rect.width > 0.0f);
    }
}

TEST_CASE("InterludeExitMarker: spans the corridor (multiple dashes)") {
    // The corridor is 1800 px wide (150..1950). With 40-px dashes + 20-px
    // gaps (period 60), a complete sweep should yield ~30 dashes. Pin
    // a generous lower bound so a future tweak to the dash size stays
    // honest about "covers the corridor".
    const auto L = nccu::LayoutInterludeExitMarker(0.0f);
    CHECK(L.dashes.size() >= 20);
}

TEST_CASE("InterludeExitMarker: phase 0 ≡ phase period (periodic)") {
    // The phase parameter wraps on the dash period; a phase equal to one
    // full period must yield the same dash list as phase 0 — otherwise
    // the animation accumulator would drift the visual over time.
    const float period = nccu::kInterludeMarkerDashLen +
                         nccu::kInterludeMarkerGapLen;
    const auto A = nccu::LayoutInterludeExitMarker(0.0f);
    const auto B = nccu::LayoutInterludeExitMarker(period);
    REQUIRE(A.dashes.size() == B.dashes.size());
    for (std::size_t i = 0; i < A.dashes.size(); ++i) {
        CHECK(A.dashes[i].rect.x == doctest::Approx(B.dashes[i].rect.x));
        CHECK(A.dashes[i].rect.width ==
              doctest::Approx(B.dashes[i].rect.width));
    }
}

TEST_CASE("InterludeExitMarker: phase shifts dashes east by phase pixels") {
    // A small phase offset (< gap length) moves the leading edge of each
    // unclipped dash east by exactly that amount — the visual sweep.
    const auto A = nccu::LayoutInterludeExitMarker(0.0f);
    const auto B = nccu::LayoutInterludeExitMarker(10.0f);
    // Pick a dash safely inside the corridor (not clipped at either
    // edge) and compare the x position.
    REQUIRE(A.dashes.size() >= 5);
    REQUIRE(B.dashes.size() >= 5);
    const auto& a = A.dashes[5];
    const auto& b = B.dashes[5];
    // If neither dash is clipped (both at full width), the shift is +10.
    if (a.rect.width == doctest::Approx(nccu::kInterludeMarkerDashLen) &&
        b.rect.width == doctest::Approx(nccu::kInterludeMarkerDashLen)) {
        CHECK(b.rect.x - a.rect.x == doctest::Approx(10.0f));
    }
}

TEST_CASE("InterludeExitMarker: draw emits shadow+gold per dash") {
    // The painter does two rects per dash (drop shadow + gold body) —
    // the same two-pass idiom the H4 indicator uses. A regression that
    // skipped the shadow would still draw something, but the marker would
    // be unreadable on bright tiles.
    Spy spy;
    nccu::DrawInterludeExitMarker(spy, 0.0f);
    const auto layout = nccu::LayoutInterludeExitMarker(0.0f);
    REQUIRE(!layout.dashes.empty());
    CHECK(spy.rects.size() == layout.dashes.size() * 2);
    CHECK(spy.texts.empty());
    CHECK(spy.sprites == 0);
}

TEST_CASE("InterludeExitMarker: gold body uses #FFC83D, shadow is dark") {
    // Pin the colour contract — the gold matches the H4 quest-giver
    // indicator panel, so the two visual cues share a palette.
    Spy spy;
    nccu::DrawInterludeExitMarker(spy, 0.0f);
    REQUIRE(spy.rects.size() >= 2);

    // The painter emits shadow first, then gold for each dash. Spot-check
    // the first pair.
    const auto shadow = spy.rects[0];
    const auto gold   = spy.rects[1];

    CHECK(shadow.c.r == 0);
    CHECK(shadow.c.g == 0);
    CHECK(shadow.c.b == 0);
    CHECK(shadow.c.a < 255);   // translucent

    CHECK(gold.c.r == 255);
    CHECK(gold.c.g == 200);
    CHECK(gold.c.b == 61);
    CHECK(gold.c.a == 255);

    // Shadow is offset 2 px SE from the gold body.
    CHECK(shadow.r.x - gold.r.x == doctest::Approx(2.0f));
    CHECK(shadow.r.y - gold.r.y == doctest::Approx(2.0f));
}

TEST_CASE("InterludeExitMarker: marker lives at the exit zone NORTH edge") {
    // The marker MUST sit on the exit-zone threshold (y==minY), not below
    // it — otherwise the player crosses into the trigger band before
    // seeing the line, which defeats the "advance warning" intent.
    Spy spy;
    nccu::DrawInterludeExitMarker(spy, 0.0f);
    REQUIRE(!spy.rects.empty());
    // Both shadow and body draw at this y (shadow nudged 2 px south).
    bool foundOnEdge = false;
    for (const auto& rc : spy.rects) {
        if (rc.r.y == doctest::Approx(nccu::kInterludeExitMinY)) {
            foundOnEdge = true;
            break;
        }
    }
    CHECK(foundOnEdge);
}
