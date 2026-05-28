#include "doctest/doctest.h"
#include "dialog/DialogLayout.h"
#include "dialog/DialogState.h"
#include "dialog/DialogView.h"
#include "dialog/DialogLoader.h"
#include "engine/render/IRenderer.h"
#include <string>
#include <vector>

#ifndef TEST_CONTENT_DIR
#error "TEST_CONTENT_DIR must be defined by the build system"
#endif

using nccu::dialog::CellWidth;
using nccu::dialog::WrapToCells;
using nccu::dialog::Paginate;
using nccu::dialog::LayoutPages;
using nccu::dialog::kBoxCells;
using nccu::dialog::kBoxRowsPerPage;

namespace {

// Same spy IRenderer pattern as test_dialog_box_render.cpp.
struct Spy final : nccu::gfx::IRenderer {
    int rects = 0;
    std::vector<std::string> texts;
    void DrawRect(nccu::gfx::Rect, nccu::gfx::Color) override { ++rects; }
    void DrawSprite(const nccu::gfx::Texture&, nccu::gfx::Rect,
                    nccu::gfx::Rect, nccu::gfx::Color) override {}
    void DrawText(std::string_view t, nccu::gfx::Vec2, int,
                  nccu::gfx::Color) override { texts.emplace_back(t); }
};

} // namespace

TEST_CASE("CellWidth: full-width CJK = 2, ASCII = 1 (matches dialog_lint)") {
    CHECK(CellWidth("abc") == 3);
    CHECK(CellWidth("傘") == 2);                 // CJK ideograph
    CHECK(CellWidth("你好") == 4);
    CHECK(CellWidth("傘a") == 3);                // mixed
    CHECK(CellWidth("（）") == 4);               // U+FF08/09 fullwidth
    CHECK(CellWidth("") == 0);
}

TEST_CASE("A CJK line just over the box width wraps into rows each <=kBoxCells") {
    // Scale the input to kBoxCells (not a literal 28/80) so this stays a
    // wrap test for ANY box width: kBoxCells+1 cells of CJK is the
    // smallest input that MUST spill past one row. Built from one CJK
    // glyph (2 cells) repeated, so the cell count is exact and the test
    // doesn't pin the renderer's wrap constant to a magic number.
    std::string line;
    const int targetCells = kBoxCells + 1;        // guarantees a 2nd row
    while (CellWidth(line) <= targetCells) line += "傘";  // 2 cells each
    REQUIRE(CellWidth(line) > kBoxCells);
    const auto rows = WrapToCells(line, kBoxCells);
    CHECK(rows.size() >= 2);
    for (const std::string& r : rows) {
        CHECK(CellWidth(r) <= kBoxCells);        // no overflow row, ever
        CHECK_FALSE(r.empty());
    }
    // Loss-less: concatenating the rows reproduces the source (no glyph
    // dropped or split — there are no spaces here to collapse).
    std::string joined;
    for (const std::string& r : rows) joined += r;
    CHECK(joined == line);
}

TEST_CASE("A very long CJK line wraps with no row wider than kBoxCells") {
    // Far longer than one box width (sized to kBoxCells so it stays a
    // wrap test at any box width): kBoxCells*4 cells of CJK must produce
    // ~4 rows, none wider than the box.
    std::string line;
    while (CellWidth(line) < kBoxCells * 4) line += "傘";
    REQUIRE(CellWidth(line) > kBoxCells);
    const auto rows = WrapToCells(line, kBoxCells);
    CHECK(rows.size() >= 4);
    for (const std::string& r : rows)
        CHECK(CellWidth(r) <= kBoxCells);
}

TEST_CASE("ASCII word wrap breaks on spaces, never mid-word") {
    const auto rows = WrapToCells(
        "the quick brown fox jumps over the lazy dog again", 10);
    for (const std::string& r : rows) CHECK(CellWidth(r) <= 10);
    // every token survives whole
    std::string joined;
    for (std::size_t i = 0; i < rows.size(); ++i) {
        joined += rows[i];
        if (i + 1 < rows.size()) joined += ' ';
    }
    CHECK(joined == "the quick brown fox jumps over the lazy dog again");
}

TEST_CASE("A single token wider than the budget still cannot overflow") {
    const auto rows = WrapToCells("supercalifragilistic", 8);
    for (const std::string& r : rows) CHECK(CellWidth(r) <= 8);
    std::string joined;
    for (const std::string& r : rows) joined += r;
    CHECK(joined == "supercalifragilistic");
}

TEST_CASE("Literal newline forces a hard break; leading indent preserved") {
    const auto rows = WrapToCells("  > pick me\nshort", kBoxCells);
    REQUIRE(rows.size() == 2);
    CHECK(rows[0] == "  > pick me");             // intentional indent kept
    CHECK(rows[1] == "short");
}

TEST_CASE("Paginate: rows split into pages of kBoxRowsPerPage") {
    std::vector<std::string> rows{"a", "b", "c", "d", "e", "f", "g"};
    const auto pages = Paginate(rows, 3);
    REQUIRE(pages.size() == 3);
    CHECK(pages[0].size() == 3);
    CHECK(pages[1].size() == 3);
    CHECK(pages[2].size() == 1);                 // boundary remainder
    CHECK(pages[2][0] == "g");
}

TEST_CASE("Paginate / LayoutPages always return at least one page") {
    CHECK(Paginate({}, 3).size() == 1);
    CHECK(LayoutPages("", kBoxCells, kBoxRowsPerPage).size() == 1);
}

TEST_CASE("DialogState paginates a long line; advance turns the page") {
    nccu::DialogState d;
    // Size the line to kBoxCells so this stays a TWO-PAGE pagination test
    // for ANY box width: one full page holds kBoxCells*kBoxRowsPerPage
    // cells; +1 cell more spills onto a 2nd page. Building it as a count
    // of 2-cell CJK glyphs keeps the cell total exact without pinning a
    // literal page capacity.
    const int onePageCells = kBoxCells * kBoxRowsPerPage;
    std::string longLine;
    while (CellWidth(longLine) <= onePageCells) longLine += "傘";
    REQUIRE(CellWidth(longLine) > onePageCells);  // guaranteed > 1 page
    d.Open({longLine, "短"});
    CHECK(d.Active());
    // Page 1: a full page of rows, each <=kBoxCells, more pages pending.
    auto p1 = d.CurrentPageRows();
    CHECK(p1.size() == static_cast<std::size_t>(kBoxRowsPerPage));
    for (const std::string& r : p1) CHECK(CellWidth(r) <= kBoxCells);
    CHECK(d.CurrentLineHasMorePages());
    CHECK(d.HasMore());

    CHECK(d.Advance() == nullptr);               // turn page, SAME line
    CHECK(d.CurrentLine() == longLine);          // still on line 0
    CHECK_FALSE(d.CurrentLineHasMorePages());    // spilled exactly 1 row
    auto p2 = d.CurrentPageRows();
    for (const std::string& r : p2) CHECK(CellWidth(r) <= kBoxCells);
    CHECK(d.HasMore());                          // a further line remains

    CHECK(d.Advance() == nullptr);               // now advance to line 1
    CHECK(d.CurrentLine() == "短");
    CHECK_FALSE(d.CurrentLineHasMorePages());
    CHECK_FALSE(d.HasMore());                    // last page of last line
    CHECK(d.Advance() == nullptr);               // closes
    CHECK_FALSE(d.Active());
}

TEST_CASE("Every authored line in docs/content renders with NO >28-cell "
          "row across all pages (the B4 invariant, font-independent)") {
    // Authoritative B4 verification: drive the REAL engine
    // (DialogLoader -> DialogState -> DrawDialog spy) over every
    // authored dialogue line in every shipped chapter and assert the
    // renderer's own wrap+pagination keeps every visual row within the
    // dialog box. This is what makes the 251 dialog_lint "overflow"
    // warnings a non-issue: the box can no longer overflow regardless
    // of authored length. (The CJK font asset is absent under tests, so
    // pixels are tofu — verifying via engine logic, not screenshots.)
    const char* kFiles[] = {
        "chapter1.md", "chapter2.md", "chapter3.md", "chapter4.md",
        "ending_a.md", "ending_b.md", "ending_c.md",
        "interlude_market.md", "voice_bible.md",
    };
    int linesChecked = 0;
    for (const char* fn : kFiles) {
        const auto chap = nccu::dialog::LoadChapter(
            std::string(TEST_CONTENT_DIR) + "/" + fn);
        for (const auto& [npc, subs] : chap.npcs) {
            for (const auto& sub : subs) {
                if (sub.lines.empty()) continue;
                nccu::DialogState d;
                d.Open(sub.lines);
                // Walk EVERY page of EVERY line via the real advance.
                int guard = 0;
                while (d.Active() && guard++ < 4096) {
                    Spy s;
                    nccu::DrawDialog(s, d);
                    int rowIdx = 0;
                    for (const std::string& t : s.texts) {
                        if (rowIdx++ >= kBoxRowsPerPage) break;  // ▼
                        CHECK(CellWidth(t) <= kBoxCells);
                    }
                    ++linesChecked;
                    if (d.Advance() != nullptr) break;  // choice (none here)
                    if (d.AtChoice()) break;
                }
            }
        }
    }
    CHECK(linesChecked > 200);   // sanity: the corpus really was walked
}

TEST_CASE("DrawDialog renders only one page of rows, all within the box") {
    nccu::DialogState d;
    std::string longLine;
    for (int i = 0; i < 50; ++i) longLine += "傘";   // 100 cells
    d.Open({longLine});
    Spy s;
    nccu::DrawDialog(s, d);
    CHECK(s.rects >= 2);                          // panel + border
    REQUIRE(s.texts.size() >= 1);
    // First kBoxRowsPerPage texts are wrapped rows; a trailing one may
    // be the "▼" affordance. Every wrapped row must fit the box.
    CHECK(s.texts.size() <= kBoxRowsPerPage + 1);
    for (std::size_t i = 0; i < s.texts.size() &&
                            i < static_cast<std::size_t>(kBoxRowsPerPage);
         ++i)
        CHECK(CellWidth(s.texts[i]) <= kBoxCells);
}
