#include "doctest/doctest.h"
#include "game/dialog/DialogLayout.h"
#include "game/dialog/DialogState.h"
#include "game/dialog/DialogView.h"
#include "game/dialog/DialogLoader.h"
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

/**
 * @file test_dialog_layout.cpp
 * @brief 驗證對話排版邏輯：CellWidth 的全形／半形寬度、CJK 與 ASCII 的自動換行、
 *        硬換行與縮排保留、Paginate 分頁、DialogState 的長行翻頁，以及所有實際
 *        對話內容算繪後每列都不超過對話框寬度。
 */

namespace {

// 與 test_dialog_box_render.cpp 相同的間諜 IRenderer 模式。
struct Spy final : nccu::engine::render::IRenderer {
    int rects = 0;
    std::vector<std::string> texts;
    void DrawRect(nccu::engine::math::Rect, nccu::engine::math::Color) override { ++rects; }
    void DrawSprite(const nccu::engine::render::Texture&, nccu::engine::math::Rect,
                    nccu::engine::math::Rect, nccu::engine::math::Color) override {}
    void DrawText(std::string_view t, nccu::engine::math::Vec2, int,
                  nccu::engine::math::Color) override { texts.emplace_back(t); }
};

} // namespace

// CellWidth：全形 CJK 算 2、ASCII 算 1。
TEST_CASE("CellWidth: full-width CJK = 2, ASCII = 1 (matches dialog_lint)") {
    CHECK(CellWidth("abc") == 3);
    CHECK(CellWidth("傘") == 2);                 // CJK 表意文字
    CHECK(CellWidth("你好") == 4);
    CHECK(CellWidth("傘a") == 3);                // 混合
    CHECK(CellWidth("（）") == 4);               // U+FF08/09 全形
    CHECK(CellWidth("") == 0);
}

// 略寬於對話框的 CJK 單行會換成多列，每列寬度都 <= kBoxCells。
TEST_CASE("A CJK line just over the box width wraps into rows each <=kBoxCells") {
    // 以 kBoxCells 為基準（而非寫死 28/80）來構造輸入，使這個測試對任何框寬都
    // 成立：kBoxCells+1 個 cell 的 CJK 是必定溢出一列的最小輸入。用單一 CJK
    // 字（2 cell）重複組成，cell 數精確，且不把算繪器的換行常數寫成魔術數字。
    std::string line;
    const int targetCells = kBoxCells + 1;        // 保證有第二列
    while (CellWidth(line) <= targetCells) line += "傘";  // 每個 2 cell
    REQUIRE(CellWidth(line) > kBoxCells);
    const auto rows = WrapToCells(line, kBoxCells);
    CHECK(rows.size() >= 2);
    for (const std::string& r : rows) {
        CHECK(CellWidth(r) <= kBoxCells);        // 永遠沒有溢出的列
        CHECK_FALSE(r.empty());
    }
    // 無損：把各列接回去能還原來源（沒有字被丟掉或切開——這裡沒有空白可摺疊）。
    std::string joined;
    for (const std::string& r : rows) joined += r;
    CHECK(joined == line);
}

// 很長的 CJK 單行換行後，沒有任何一列寬於 kBoxCells。
TEST_CASE("A very long CJK line wraps with no row wider than kBoxCells") {
    // 遠長於一個框寬（以 kBoxCells 為基準，使其對任何框寬都成立）：kBoxCells*4
    // 個 cell 的 CJK 必須產生約 4 列，且沒有一列寬於框。
    std::string line;
    while (CellWidth(line) < kBoxCells * 4) line += "傘";
    REQUIRE(CellWidth(line) > kBoxCells);
    const auto rows = WrapToCells(line, kBoxCells);
    CHECK(rows.size() >= 4);
    for (const std::string& r : rows)
        CHECK(CellWidth(r) <= kBoxCells);
}

// ASCII 文字換行只在空白處斷，絕不從字中間斷。
TEST_CASE("ASCII word wrap breaks on spaces, never mid-word") {
    const auto rows = WrapToCells(
        "the quick brown fox jumps over the lazy dog again", 10);
    for (const std::string& r : rows) CHECK(CellWidth(r) <= 10);
    // 每個詞都完整保留
    std::string joined;
    for (std::size_t i = 0; i < rows.size(); ++i) {
        joined += rows[i];
        if (i + 1 < rows.size()) joined += ' ';
    }
    CHECK(joined == "the quick brown fox jumps over the lazy dog again");
}

// 單一詞就比預算還寬時，仍不會溢出。
TEST_CASE("A single token wider than the budget still cannot overflow") {
    const auto rows = WrapToCells("supercalifragilistic", 8);
    for (const std::string& r : rows) CHECK(CellWidth(r) <= 8);
    std::string joined;
    for (const std::string& r : rows) joined += r;
    CHECK(joined == "supercalifragilistic");
}

// 字面換行強制硬斷行，且前導縮排會被保留。
TEST_CASE("Literal newline forces a hard break; leading indent preserved") {
    const auto rows = WrapToCells("  > pick me\nshort", kBoxCells);
    REQUIRE(rows.size() == 2);
    CHECK(rows[0] == "  > pick me");             // 刻意的縮排保留
    CHECK(rows[1] == "short");
}

// Paginate：列依每頁 kBoxRowsPerPage 切成多頁。
TEST_CASE("Paginate: rows split into pages of kBoxRowsPerPage") {
    std::vector<std::string> rows{"a", "b", "c", "d", "e", "f", "g"};
    const auto pages = Paginate(rows, 3);
    REQUIRE(pages.size() == 3);
    CHECK(pages[0].size() == 3);
    CHECK(pages[1].size() == 3);
    CHECK(pages[2].size() == 1);                 // 邊界餘數
    CHECK(pages[2][0] == "g");
}

// Paginate／LayoutPages 永遠至少回傳一頁。
TEST_CASE("Paginate / LayoutPages always return at least one page") {
    CHECK(Paginate({}, 3).size() == 1);
    CHECK(LayoutPages("", kBoxCells, kBoxRowsPerPage).size() == 1);
}

// DialogState 會替長行分頁；Advance 會翻頁。
TEST_CASE("DialogState paginates a long line; advance turns the page") {
    nccu::DialogState d;
    // 以 kBoxCells 為基準構造此行，使其對任何框寬都維持「兩頁」分頁測試：
    // 一整頁容納 kBoxCells*kBoxRowsPerPage 個 cell；再多 1 cell 就溢到第二頁。
    // 以 2-cell 的 CJK 字計數組成，cell 總數精確且不寫死頁容量。
    const int onePageCells = kBoxCells * kBoxRowsPerPage;
    std::string longLine;
    while (CellWidth(longLine) <= onePageCells) longLine += "傘";
    REQUIRE(CellWidth(longLine) > onePageCells);  // 保證 > 1 頁
    d.Open({longLine, "短"});
    CHECK(d.Active());
    // 第 1 頁：滿一頁的列、每列 <=kBoxCells、仍有後續頁。
    auto p1 = d.CurrentPageRows();
    CHECK(p1.size() == static_cast<std::size_t>(kBoxRowsPerPage));
    for (const std::string& r : p1) CHECK(CellWidth(r) <= kBoxCells);
    CHECK(d.CurrentLineHasMorePages());
    CHECK(d.HasMore());

    CHECK(d.Advance() == nullptr);               // 翻頁，同一行
    CHECK(d.CurrentLine() == longLine);          // 仍在第 0 行
    CHECK_FALSE(d.CurrentLineHasMorePages());    // 剛好只溢出 1 列
    auto p2 = d.CurrentPageRows();
    for (const std::string& r : p2) CHECK(CellWidth(r) <= kBoxCells);
    CHECK(d.HasMore());                          // 還有下一行

    CHECK(d.Advance() == nullptr);               // 現在前進到第 1 行
    CHECK(d.CurrentLine() == "短");
    CHECK_FALSE(d.CurrentLineHasMorePages());
    CHECK_FALSE(d.HasMore());                    // 最後一行的最後一頁
    CHECK(d.Advance() == nullptr);               // 關閉
    CHECK_FALSE(d.Active());
}

TEST_CASE("Every authored line in docs/content renders with NO >28-cell "
          "row across all pages (the B4 invariant, font-independent)") {
    // 端到端驗證：以真正的引擎（DialogLoader -> DialogState -> DrawDialog 間諜）
    // 走過每個出貨章節的每一句作者撰寫對話，斷言算繪器自身的換行＋分頁讓每個
    // 視覺列都留在對話框內。如此一來，不論作者寫多長，對話框都不會溢出。
    //（測試環境下沒有 CJK 字型素材，像素是豆腐字——這裡靠引擎邏輯驗證而非截圖。）
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
                // 以真正的 advance 走過每一行的每一頁。
                int guard = 0;
                while (d.Active() && guard++ < 4096) {
                    Spy s;
                    nccu::DrawDialog(s, d);
                    int rowIdx = 0;
                    for (const std::string& t : s.texts) {
                        if (rowIdx++ >= kBoxRowsPerPage) break;  // 跳過 ▼ 提示列
                        CHECK(CellWidth(t) <= kBoxCells);
                    }
                    ++linesChecked;
                    if (d.Advance() != nullptr) break;  // 選項（此處皆無）
                    if (d.AtChoice()) break;
                }
            }
        }
    }
    CHECK(linesChecked > 200);   // 合理性檢查：確實走過整個語料
}

// DrawDialog 一次只算繪一頁的列，且每列都在框內。
TEST_CASE("DrawDialog renders only one page of rows, all within the box") {
    nccu::DialogState d;
    std::string longLine;
    for (int i = 0; i < 50; ++i) longLine += "傘";   // 100 cell
    d.Open({longLine});
    Spy s;
    nccu::DrawDialog(s, d);
    CHECK(s.rects >= 2);                          // 面板 + 外框
    REQUIRE(s.texts.size() >= 1);
    // 前 kBoxRowsPerPage 段文字是換行後的列；末尾可能多一個 ▼ 提示符。
    // 每個換行列都必須能容納於框內。
    CHECK(s.texts.size() <= kBoxRowsPerPage + 1);
    for (std::size_t i = 0; i < s.texts.size() &&
                            i < static_cast<std::size_t>(kBoxRowsPerPage);
         ++i)
        CHECK(CellWidth(s.texts[i]) <= kBoxCells);
}
