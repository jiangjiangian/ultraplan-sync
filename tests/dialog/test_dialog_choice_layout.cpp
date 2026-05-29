#include "doctest/doctest.h"
#include "game/quest/Flags.h"
#include "game/dialog/DialogState.h"
#include "game/dialog/DialogView.h"
#include "game/dialog/DialogLayout.h"
#include "engine/render/IRenderer.h"
#include <string>
#include <vector>

// Item 5b regression: at the WIDE dialog box (kBoxCells) a choice menu
// must read as ONE option per logical entry (not a wall of equal-pitch
// rows), the selected option carries the "> " caret marker, and the
// three-option Ch4 finale (體諒/質問/我再想想…) fits the 110px panel.
//
// Position-aware spy (the box-render spy captures only strings): records
// each DrawText's y so the inter-choice gap and the per-option row count
// can be asserted without a GL context (CJK is tofu under tests anyway —
// this is layout logic, not glyph rendering).

using nccu::dialog::kBoxCells;
using nccu::dialog::kBoxTextY;
using nccu::dialog::kBoxLineH;
using nccu::dialog::kBoxH;
using nccu::dialog::kBoxY;

namespace {

struct PosSpy final : nccu::gfx::IRenderer {
    struct T { std::string text; float y; };
    std::vector<T> texts;
    void DrawRect(nccu::engine::math::Rect, nccu::engine::math::Color) override {}
    void DrawSprite(const nccu::gfx::Texture&, nccu::engine::math::Rect,
                    nccu::engine::math::Rect, nccu::engine::math::Color) override {}
    void DrawText(std::string_view t, nccu::engine::math::Vec2 p, int,
                  nccu::engine::math::Color) override {
        texts.push_back({std::string(t), p.y});
    }
};

// Build the real Ch4 finale menu shape (labels mirror DialogOpener.cpp:
// 體諒/質問 then the kDialogExitLabel "我再想想…" decline tail).
nccu::DialogState OpenFinaleMenu() {
    nccu::DialogState d;
    d.Open({"（他把傘遞過來）"},
           {{"體諒助教的辛勞", 15, nccu::kFlagConsoledTA, true, {"x"}},
            {"質問／強硬索回", -5, "", false, {"y"}},
            {std::string(nccu::kDialogExitLabel), 0, "", false, {}}});
    d.Advance();                         // step past opener -> choice mode
    return d;
}

} // namespace

TEST_CASE("5b: Ch4 finale menu renders one row per option at the wide box") {
    nccu::DialogState d = OpenFinaleMenu();
    REQUIRE(d.AtChoice());
    REQUIRE(d.Choices().size() == 3);
    // Every label is short enough to be a single row at kBoxCells, so the
    // wide box must NOT wrap any of them (the point of the V-wrap fix).
    for (const auto& c : d.Choices())
        REQUIRE(nccu::dialog::CellWidth("  " + c.label) <= kBoxCells);

    PosSpy s;
    nccu::DrawDialog(s, d);
    // One text per option (no wrapped extra rows) — one option per line.
    REQUIRE(s.texts.size() == 3);
    CHECK(s.texts[0].text.find("體諒") != std::string::npos);
    CHECK(s.texts[1].text.find("質問") != std::string::npos);
    CHECK(s.texts[2].text.find("我再想想") != std::string::npos);
}

TEST_CASE("5b: exactly one option carries the '> ' selected-row marker") {
    nccu::DialogState d = OpenFinaleMenu();
    PosSpy s;
    nccu::DrawDialog(s, d);
    int marked = 0, blank = 0;
    for (const auto& t : s.texts) {
        if (t.text.rfind("> ", 0) == 0) ++marked;
        else if (t.text.rfind("  ", 0) == 0) ++blank;
    }
    CHECK(marked == 1);                  // exactly one selected caret
    CHECK(blank == 2);                   // the other two are unmarked
    CHECK(s.texts[0].text.rfind("> ", 0) == 0);   // cursor starts at 0
}

TEST_CASE("5b: selected marker follows the choice cursor") {
    nccu::DialogState d = OpenFinaleMenu();
    d.MoveChoice(1);                     // cursor -> option 1
    PosSpy s;
    nccu::DrawDialog(s, d);
    REQUIRE(s.texts.size() == 3);
    CHECK(s.texts[0].text.rfind("  ", 0) == 0);
    CHECK(s.texts[1].text.rfind("> ", 0) == 0);   // now option 1 marked
    CHECK(s.texts[2].text.rfind("  ", 0) == 0);
}

TEST_CASE("5b: an inter-choice gap separates options (not equal pitch)") {
    nccu::DialogState d = OpenFinaleMenu();
    PosSpy s;
    nccu::DrawDialog(s, d);
    REQUIRE(s.texts.size() == 3);
    // Each single-row option is one kBoxLineH apart PLUS a gap, so the
    // option-to-option pitch strictly exceeds kBoxLineH (what proves the
    // menu isn't a flat equal-pitch list).
    const float pitch01 = s.texts[1].y - s.texts[0].y;
    const float pitch12 = s.texts[2].y - s.texts[1].y;
    CHECK(pitch01 > kBoxLineH);
    CHECK(pitch12 > kBoxLineH);
    CHECK(pitch01 == doctest::Approx(pitch12));   // uniform option spacing
}

TEST_CASE("5b: three finale options fit inside the 110px dialog panel") {
    nccu::DialogState d = OpenFinaleMenu();
    PosSpy s;
    nccu::DrawDialog(s, d);
    REQUIRE(!s.texts.empty());
    // Bottom of the last drawn row must stay within the panel bottom edge
    // (kBoxY + kBoxH) with a glyph's height of clearance.
    float maxY = 0.0f;
    for (const auto& t : s.texts) maxY = t.y > maxY ? t.y : maxY;
    CHECK(maxY + kBoxLineH <= kBoxY + kBoxH);
}
