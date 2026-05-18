#include "doctest/doctest.h"
#include "DialogLayout.h"
#include "DialogState.h"
#include "DialogView.h"
#include "DialogLoader.h"
#include "gfx/IRenderer.h"
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

TEST_CASE("A 42-cell mixed CJK/ASCII line wraps into rows each <=28 cells") {
    // 21 CJK glyphs = 42 cells; > 28 so it must wrap.
    const std::string line = "順手幫我把一樓傘架的那把透明傘帶上來就好欸";
    REQUIRE(CellWidth(line) == 42);
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

TEST_CASE("The longest authored line (54 cells) wraps with no >28 row") {
    // chapter*.md peak length found by dialog_lint (54 full-width cells).
    const std::string line =
        "這是一段非常長的章節旁白用來模擬內容檔裡最長的那一句它有五十"
        "四個全形字格遠遠超過對話框的二十八格寬度必須換行";
    REQUIRE(CellWidth(line) > 28);
    for (const std::string& r : WrapToCells(line, kBoxCells))
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
    // 60 CJK glyphs = 120 cells -> >1 page at 28 cells x 3 rows (84/pg).
    std::string longLine;
    for (int i = 0; i < 60; ++i) longLine += "傘";
    d.Open({longLine, "短"});
    CHECK(d.Active());
    // Page 1: 3 rows, each <=28 cells, more pages pending.
    auto p1 = d.CurrentPageRows();
    CHECK(p1.size() == kBoxRowsPerPage);
    for (const std::string& r : p1) CHECK(CellWidth(r) <= kBoxCells);
    CHECK(d.CurrentLineHasMorePages());
    CHECK(d.HasMore());

    CHECK(d.Advance() == nullptr);               // turn page, SAME line
    CHECK(d.CurrentLine() == longLine);          // still on line 0
    CHECK_FALSE(d.CurrentLineHasMorePages());    // 120 cells -> 2 pages
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
