#include "doctest/doctest.h"
#include "TrueUmbrella.h"
#include "CursedUmbrella.h"
#include "gfx/IRenderer.h"

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

bool SameRect(nccu::gfx::Rect a, nccu::gfx::Rect b) {
    return a.x == b.x && a.y == b.y && a.width == b.width && a.height == b.height;
}

} // namespace

// Template Method evidence: the 3-rect glyph skeleton lives in
// TransparentUmbrella::Render; each leaf only supplies its canopy tint via
// the constructor. Same geometry + same DarkGray handle for every subclass;
// only the two canopy rects' colour differs.
TEST_CASE("TransparentUmbrella::Render emits the 3-rect glyph via IRenderer") {
    using nccu::gfx::Vec2;
    using nccu::gfx::Rect;
    namespace Colors = nccu::gfx::Colors;

    const Vec2 pos{100.0f, 200.0f};
    const Rect canopyTop {102.0f, 204.0f, 16.0f, 3.0f};
    const Rect canopyBody{100.0f, 207.0f, 20.0f, 3.0f};
    const Rect handle    {109.0f, 210.0f,  2.0f, 9.0f};

    SUBCASE("TrueUmbrella canopy uses its own tint, handle is DarkGray") {
        TrueUmbrella u{pos};
        CountingRenderer spy;
        u.Render(spy);

        REQUIRE(spy.rects.size() == 3);
        CHECK(spy.sprites == 0);              // pure vector glyph, no sprite
        CHECK(spy.texts == 0);                // umbrellas never draw text
        CHECK(SameRect(spy.rects[0].r, canopyTop));
        CHECK(SameRect(spy.rects[1].r, canopyBody));
        CHECK(SameRect(spy.rects[2].r, handle));
        CHECK(spy.rects[0].c == nccu::gfx::Color{180, 230, 255, 255});
        CHECK(spy.rects[1].c == nccu::gfx::Color{180, 230, 255, 255});
        CHECK(spy.rects[2].c == Colors::DarkGray);
    }

    SUBCASE("CursedUmbrella reuses the skeleton, only the tint changes") {
        CursedUmbrella u{pos};
        CountingRenderer spy;
        u.Render(spy);

        REQUIRE(spy.rects.size() == 3);
        CHECK(SameRect(spy.rects[0].r, canopyTop));   // identical geometry
        CHECK(SameRect(spy.rects[1].r, canopyBody));
        CHECK(SameRect(spy.rects[2].r, handle));
        CHECK(spy.rects[0].c == nccu::gfx::Color{120, 100, 140, 255});
        CHECK(spy.rects[2].c == Colors::DarkGray);    // handle unchanged
    }
}
