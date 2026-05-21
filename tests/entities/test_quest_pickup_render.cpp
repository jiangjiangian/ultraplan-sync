#include "doctest/doctest.h"
#include "entities/QuestFlagPickup.h"
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

} // namespace

// The quest form used to have an empty Render() (mirrored CashPickup), so it
// was invisible and the 助教 fetch quest was uncompletable without
// foreknowledge. It now draws a single ground marker via the injected
// IRenderer — same placeholder convention as NPC::Render, no sprite or text.
TEST_CASE("QuestFlagPickup::Render draws one visible ground marker") {
    QuestFlagPickup item(nccu::gfx::Vec2{120.0f, 80.0f}, "Flag_FoundForm");
    CountingRenderer spy;
    item.Render(spy);

    REQUIRE(spy.rects.size() == 1);   // was 0 before — proves it is visible
    CHECK(spy.sprites == 0);          // placeholder marker, no sprite
    CHECK(spy.texts == 0);            // an Item never draws text
    CHECK(spy.rects[0].c == nccu::gfx::Colors::Yellow);
}
