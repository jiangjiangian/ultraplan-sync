#include "doctest/doctest.h"
#include "quest/Flags.h"
#include "entities/QuestFlagPickup.h"
#include "entities/Player.h"
#include "engine/render/IRenderer.h"
#include "gfx/UmbrellaGlyph.h"
#include "quest/Chapter1Quest.h"

#include <vector>

namespace {

// Spy IRenderer: records every primitive instead of touching a GL context,
// so the polymorphic Render() path is testable headless.
struct CountingRenderer final : nccu::gfx::IRenderer {
    struct RectCall { nccu::gfx::Rect r; nccu::gfx::Color c; };
    std::vector<RectCall> rects;
    int sprites = 0;
    int texts = 0;

    void DrawRect(nccu::gfx::Rect r, nccu::gfx::Color c) override {
        rects.push_back({r, c});
    }
    void DrawSprite(const nccu::gfx::Texture&, nccu::gfx::Rect,
                    nccu::gfx::Rect, nccu::gfx::Color) override {
        ++sprites;
    }
    void DrawText(std::string_view, nccu::gfx::Vec2, int,
                  nccu::gfx::Color) override {
        ++texts;
    }
};

bool HasColor(const CountingRenderer& s, nccu::gfx::Color want) {
    for (const auto& rc : s.rects) if (rc.c == want) return true;
    return false;
}

} // namespace

// The quest form used to have an empty Render() (mirrored CashPickup), so it
// was invisible; then it drew ONE Yellow square for every quest item. T2
// makes the marker TYPE-AWARE (the reported bug: the Ch1 transparent umbrella
// read as a yellow square / 紙張). A PAPER quest item (申請書 / 筆記) now draws
// a WHITE sheet; the 苦主 umbrella draws the shared BLUE umbrella glyph. Still
// rect-only — an Item never draws a sprite or text (architecture rule).
TEST_CASE("QuestFlagPickup::Render: a paper quest item draws a WHITE sheet") {
    QuestFlagPickup form(nccu::gfx::Vec2{120.0f, 80.0f}, nccu::kFlagFoundForm);
    CountingRenderer spy;
    form.Render(spy);

    CHECK(spy.sprites == 0);          // placeholder marker, no sprite
    CHECK(spy.texts == 0);            // an Item never draws text
    REQUIRE(spy.rects.size() >= 1);
    // The owner: 紙張請用白色 — the sheet's body is White (NOT the old Yellow).
    CHECK(HasColor(spy, nccu::gfx::Colors::White));
    CHECK_FALSE(HasColor(spy, nccu::gfx::Colors::Yellow));
}

TEST_CASE("QuestFlagPickup::Render: the 苦主 umbrella draws the BLUE umbrella glyph") {
    // The Ch1 victim transparent umbrella sets Flag_HasVictimUmbrella → it must
    // render as the 真傘 (blue) umbrella, the SAME shared glyph the in-world
    // umbrellas / ending card use — not a yellow square.
    QuestFlagPickup umb(nccu::gfx::Vec2{200.0f, 300.0f},
                        nccu::kFlagHasVictimUmbrella,
                        "撿到一把眼熟的透明傘");
    CountingRenderer spy;
    umb.Render(spy);

    CHECK(spy.sprites == 0);
    CHECK(spy.texts == 0);
    REQUIRE(spy.rects.size() >= 3);   // a real umbrella silhouette
    // Its signature colour is 真傘 blue (the shared look colour), never Yellow.
    CHECK(HasColor(spy,
        nccu::gfx::UmbrellaLookColor(nccu::gfx::UmbrellaLook::TrueBlue)));
    CHECK_FALSE(HasColor(spy, nccu::gfx::Colors::Yellow));
}
