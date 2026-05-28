#include "doctest/doctest.h"
#include "dialog/DialogState.h"
#include "dialog/DialogView.h"
#include "engine/render/IRenderer.h"
#include <string>
#include <vector>

namespace {

// Spy IRenderer — mirrors the proven CountingRenderer pattern from
// test_umbrella_render.cpp, but also captures the text strings so we can
// assert the line / choice content without a GL context.
struct Spy final : nccu::gfx::IRenderer {
    int rects = 0;
    int sprites = 0;
    std::vector<std::string> texts;

    void DrawRect(nccu::gfx::Rect, nccu::gfx::Color) override { ++rects; }
    void DrawSprite(const nccu::gfx::Texture&, nccu::gfx::Rect,
                    nccu::gfx::Rect, nccu::gfx::Color) override { ++sprites; }
    void DrawText(std::string_view t, nccu::gfx::Vec2, int,
                  nccu::gfx::Color) override { texts.emplace_back(t); }
};

} // namespace

TEST_CASE("Inactive dialog draws nothing") {
    nccu::DialogState d;
    Spy s;
    nccu::DrawDialog(s, d);
    CHECK(s.rects == 0);
    CHECK(s.texts.empty());
}

TEST_CASE("Active line dialog draws a panel + the current line text") {
    nccu::DialogState d;
    d.Open({"hello"});
    Spy s;
    nccu::DrawDialog(s, d);
    CHECK(s.rects >= 1);                 // panel fill (+ border rect)
    REQUIRE(s.texts.size() == 1);
    CHECK(s.texts[0] == "hello");
}

TEST_CASE("Choice mode draws one text per option, selected prefixed '> '") {
    nccu::DialogState d;
    d.Open({"intro"}, {{"refuse", 0, "", false},
                        {"accept", -5, "f", false}});
    d.Advance();                         // -> choice mode
    Spy s;
    nccu::DrawDialog(s, d);
    REQUIRE(s.texts.size() == 2);
    CHECK(s.texts[0].rfind("> ", 0) == 0);   // option 0 selected
    CHECK(s.texts[1].rfind("  ", 0) == 0);   // option 1 not
}
