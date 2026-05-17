#include "doctest/doctest.h"
#include "MessageView.h"
#include "gfx/IRenderer.h"
#include <string>
#include <vector>

namespace {

// Spy IRenderer — same shape as the proven spy in
// test_dialog_box_render.cpp / test_ending_card_render.cpp: counts
// rects and captures text so DrawHudMessage's draw calls can be
// asserted without a GL context.
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

TEST_CASE("Empty HUD message draws nothing regardless of age") {
    Spy s;
    nccu::DrawHudMessage(s, "", 0.0f, 800.0f, 450.0f);
    nccu::DrawHudMessage(s, "", 2.0f, 800.0f, 450.0f);
    CHECK(s.rects == 0);
    CHECK(s.texts.empty());
}

TEST_CASE("Fresh HUD message draws a backdrop + the text") {
    Spy s;
    nccu::DrawHudMessage(s, "撿到一頁學霸的筆記。", 0.0f, 800.0f, 450.0f);
    CHECK(s.rects >= 1);                       // backdrop (+ accent bar)
    REQUIRE(s.texts.size() >= 1);
    CHECK(s.texts[0] == "撿到一頁學霸的筆記。"); // short → single line
}

TEST_CASE("HUD message past its TTL draws nothing") {
    Spy s;
    nccu::DrawHudMessage(s, "已過期的旁白", nccu::kHudTtl + 0.5f,
                         800.0f, 450.0f);
    CHECK(s.rects == 0);
    CHECK(s.texts.empty());
}

TEST_CASE("HUD message at the TTL boundary draws nothing") {
    Spy s;
    nccu::DrawHudMessage(s, "邊界", nccu::kHudTtl, 800.0f, 450.0f);
    CHECK(s.rects == 0);
    CHECK(s.texts.empty());
}

TEST_CASE("Long CJK string wraps onto multiple lines (no space breaks)") {
    Spy s;
    // ~40 CJK glyphs against a narrow viewport → must split.
    const std::string longLine =
        "這是一段很長的章節清關旁白用來測試在沒有空白字元的中文字串"
        "上面是否能夠依照量得的寬度自動換行而不會切斷一個字。";
    nccu::DrawHudMessage(s, longLine, 0.5f, 480.0f, 320.0f);
    CHECK(s.texts.size() >= 2);                 // wrapped
    // No wrapped fragment may exceed the source length, and joining
    // them must reproduce the source exactly (no glyph dropped/split).
    std::string joined;
    for (const std::string& ln : s.texts) joined += ln;
    CHECK(joined == longLine);
}

TEST_CASE("Explicit newline in the message forces a line break") {
    Spy s;
    nccu::DrawHudMessage(s, "第一行\n第二行", 0.0f, 800.0f, 450.0f);
    REQUIRE(s.texts.size() == 2);
    CHECK(s.texts[0] == "第一行");
    CHECK(s.texts[1] == "第二行");
}
