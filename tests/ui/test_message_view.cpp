#include "doctest/doctest.h"
#include "ui/MessageView.h"
#include "game/dialog/DialogLayout.h"
#include "engine/render/IRenderer.h"
#include "engine/math/Rect.h"
#include "engine/math/Vec2.h"
#include <string>
#include <vector>

namespace {

// Spy IRenderer — same shape as the proven spy in
// test_dialog_box_render.cpp / test_ending_card_render.cpp: counts
// rects and captures text so DrawHudMessage's draw calls can be
// asserted without a GL context. UI-B-3 also captures each text's draw
// X and the widest backdrop rect, so the centring + within-box wrap can
// be asserted (the draw position is the only observable of "centred").
struct Spy final : nccu::engine::render::IRenderer {
    int rects = 0;
    int sprites = 0;
    std::vector<std::string>   texts;
    std::vector<float>         textX;
    int                        fontSize = 0;
    nccu::engine::math::Rect            backdrop{0, 0, 0, 0};   // widest rect seen

    void DrawRect(nccu::engine::math::Rect r, nccu::engine::math::Color) override {
        ++rects;
        if (r.width > backdrop.width) backdrop = r;     // the box is widest
    }
    void DrawSprite(const nccu::engine::render::Texture&, nccu::engine::math::Rect,
                    nccu::engine::math::Rect, nccu::engine::math::Color) override { ++sprites; }
    void DrawText(std::string_view t, nccu::engine::math::Vec2 p, int sz,
                  nccu::engine::math::Color) override {
        texts.emplace_back(t);
        textX.push_back(p.x);
        fontSize = sz;
    }
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

// UI-B-3 — the DLC teaser renders as two CENTRED lines (DLC開發中 / 敬請期待).
// The box hugs the widest row (DLC開發中, 6 ASCII + 3 CJK cells), so the
// shorter 敬請期待 row (4 CJK = 8 cells) is drawn FURTHER RIGHT than the
// wider row → its X is strictly greater. That right-shift is the observable
// signature of per-row centring (a left-aligned box would draw both at the
// same X).
TEST_CASE("UI-B-3: the DLC teaser draws as two centred lines") {
    Spy s;
    nccu::DrawHudMessage(s, "DLC開發中\n敬請期待", 0.0f, 800.0f, 450.0f);
    REQUIRE(s.texts.size() == 2);
    CHECK(s.texts[0] == "DLC開發中");
    CHECK(s.texts[1] == "敬請期待");
    // The narrower second line (8 cells) sits inside the box hugging the
    // wider first line (6+6=12 cells), so it is pushed right of the first.
    CHECK(s.textX[1] > s.textX[0]);
    // Both rows start at or right of the box's left inner edge (inside box).
    CHECK(s.textX[0] >= s.backdrop.x);
    CHECK(s.textX[1] >= s.backdrop.x);
}

// UI-B-3 — a single centred line is centred within its (snug) box: its draw
// X equals the box's left inner edge (the box hugs the one line, so the
// centre offset is ~0). The point: a 1-line toast is unchanged in feel.
TEST_CASE("UI-B-3: a single line is drawn inside its box") {
    Spy s;
    nccu::DrawHudMessage(s, "撿到 50 元", 0.0f, 800.0f, 450.0f);
    REQUIRE(s.texts.size() == 1);
    CHECK(s.textX[0] >= s.backdrop.x);
    CHECK(s.textX[0] <= s.backdrop.x + s.backdrop.width);
}

// UI-B-3 — a long toast NEVER spills the black box: every wrapped row fits
// the panel's inner cell width. Uses the project's EAW-aware CellWidth
// (the same measure the wrap uses), so this is the box-containment gate the
// spec asks for. The box hugs the widest row, so "fits the box" ==
// "no row wider than the widest row", and additionally no row exceeds the
// cell budget derived from the 72%-screen text width.
TEST_CASE("UI-B-3: a long toast wraps within the black box cell-width") {
    Spy s;
    const float screenW = 800.0f, screenH = 450.0f;
    const std::string longLine =
        "這是一段非常長的旁白用來確認訊息框會把過長的中文字串自動換行而"
        "不會衝出黑色底框的右邊界一個字都不能溢出框外才算通過這個測試。";
    nccu::DrawHudMessage(s, longLine, 0.5f, screenW, screenH);
    REQUIRE(s.texts.size() >= 2);                  // it really wrapped

    // The font model: ~fontSize/2 px per EAW cell. The box inner width is
    // backdrop.width minus the 18px pad on each side. No wrapped row may be
    // wider than that inner width (else it spilled the box).
    REQUIRE(s.fontSize > 0);
    const float perCell  = static_cast<float>(s.fontSize) * 0.5f;
    const float innerW   = s.backdrop.width - 18.0f * 2.0f;
    for (const std::string& row : s.texts) {
        const float rowPx =
            static_cast<float>(nccu::dialog::CellWidth(row)) * perCell;
        INFO("row='" << row << "' rowPx=" << rowPx << " innerW=" << innerW);
        CHECK(rowPx <= innerW + 0.5f);             // fits inside the box
    }
    // And the box itself fits the 72%-text-budget + pad, i.e. it never grew
    // past what the screen allows.
    CHECK(s.backdrop.width <= screenW * 0.72f + 18.0f * 2.0f + 0.5f);

    // No glyph dropped/split: joining the rows reproduces the source.
    std::string joined;
    for (const std::string& ln : s.texts) joined += ln;
    CHECK(joined == longLine);
}
