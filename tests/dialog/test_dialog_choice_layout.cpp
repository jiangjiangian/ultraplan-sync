#include "doctest/doctest.h"
#include "game/quest/Flags.h"
#include "game/dialog/DialogState.h"
#include "game/dialog/DialogView.h"
#include "game/dialog/DialogLayout.h"
#include "engine/render/IRenderer.h"
#include <string>
#include <vector>

/**
 * @file test_dialog_choice_layout.cpp
 * @brief 驗證寬對話框（kBoxCells）下的選項排版：每個邏輯選項佔一列、被選中者帶
 *        "> " 游標標記且隨游標移動、選項間有額外間隔（非等距），以及三選項的 Ch4
 *        終局選單能容納於 110px 面板內。
 */

// 位置感知的間諜 IRenderer（對話框算繪那支間諜只擷取字串）：記下每次 DrawText
// 的 y，讓我們能在無 GL 環境下驗證選項間隔與每個選項的列數（測試環境下 CJK 本
// 來就是豆腐字，這裡測的是排版邏輯而非字形算繪）。

using nccu::dialog::kBoxCells;
using nccu::dialog::kBoxTextY;
using nccu::dialog::kBoxLineH;
using nccu::dialog::kBoxH;
using nccu::dialog::kBoxY;

namespace {

struct PosSpy final : nccu::engine::render::IRenderer {
    struct T { std::string text; float y; };
    std::vector<T> texts;
    void DrawRect(nccu::engine::math::Rect, nccu::engine::math::Color) override {}
    void DrawSprite(const nccu::engine::render::Texture&, nccu::engine::math::Rect,
                    nccu::engine::math::Rect, nccu::engine::math::Color) override {}
    void DrawText(std::string_view t, nccu::engine::math::Vec2 p, int,
                  nccu::engine::math::Color) override {
        texts.push_back({std::string(t), p.y});
    }
};

// 建出真正的 Ch4 終局選單形狀（標籤對應 DialogOpener.cpp：體諒／質問，
// 接著是 kDialogExitLabel「我再想想…」這個婉拒的收尾）。
nccu::DialogState OpenFinaleMenu() {
    nccu::DialogState d;
    d.Open({"（他把傘遞過來）"},
           {{"體諒助教的辛勞", 15, nccu::kFlagConsoledTA, true, {"x"}},
            {"質問／強硬索回", -5, "", false, {"y"}},
            {std::string(nccu::kDialogExitLabel), 0, "", false, {}}});
    d.Advance();                         // 越過開場白 -> 選項模式
    return d;
}

} // namespace

// Ch4 終局選單在寬對話框下每個選項算繪成一列。
TEST_CASE("5b: Ch4 finale menu renders one row per option at the wide box") {
    nccu::DialogState d = OpenFinaleMenu();
    REQUIRE(d.AtChoice());
    REQUIRE(d.Choices().size() == 3);
    // 每個標籤在 kBoxCells 下都短到只佔一列，因此寬框不得換行任何一個
    //（這正是垂直換行修正的重點）。
    for (const auto& c : d.Choices())
        REQUIRE(nccu::dialog::CellWidth("  " + c.label) <= kBoxCells);

    PosSpy s;
    nccu::DrawDialog(s, d);
    // 每個選項一段文字（沒有額外換行列）——一個選項一列。
    REQUIRE(s.texts.size() == 3);
    CHECK(s.texts[0].text.find("體諒") != std::string::npos);
    CHECK(s.texts[1].text.find("質問") != std::string::npos);
    CHECK(s.texts[2].text.find("我再想想") != std::string::npos);
}

// 恰好只有一個選項帶 "> " 被選中標記。
TEST_CASE("5b: exactly one option carries the '> ' selected-row marker") {
    nccu::DialogState d = OpenFinaleMenu();
    PosSpy s;
    nccu::DrawDialog(s, d);
    int marked = 0, blank = 0;
    for (const auto& t : s.texts) {
        if (t.text.rfind("> ", 0) == 0) ++marked;
        else if (t.text.rfind("  ", 0) == 0) ++blank;
    }
    CHECK(marked == 1);                  // 恰好一個被選中的游標
    CHECK(blank == 2);                   // 另外兩個未標記
    CHECK(s.texts[0].text.rfind("> ", 0) == 0);   // 游標從 0 開始
}

// 被選中標記會跟著選項游標移動。
TEST_CASE("5b: selected marker follows the choice cursor") {
    nccu::DialogState d = OpenFinaleMenu();
    d.MoveChoice(1);                     // 游標 -> 選項 1
    PosSpy s;
    nccu::DrawDialog(s, d);
    REQUIRE(s.texts.size() == 3);
    CHECK(s.texts[0].text.rfind("  ", 0) == 0);
    CHECK(s.texts[1].text.rfind("> ", 0) == 0);   // 現在標記在選項 1
    CHECK(s.texts[2].text.rfind("  ", 0) == 0);
}

// 選項之間有額外間隔（非等距）。
TEST_CASE("5b: an inter-choice gap separates options (not equal pitch)") {
    nccu::DialogState d = OpenFinaleMenu();
    PosSpy s;
    nccu::DrawDialog(s, d);
    REQUIRE(s.texts.size() == 3);
    // 每個單列選項相隔一個 kBoxLineH 再加上一段間隔，因此選項對選項的間距
    // 嚴格大於 kBoxLineH（這證明選單不是等距的平列清單）。
    const float pitch01 = s.texts[1].y - s.texts[0].y;
    const float pitch12 = s.texts[2].y - s.texts[1].y;
    CHECK(pitch01 > kBoxLineH);
    CHECK(pitch12 > kBoxLineH);
    CHECK(pitch01 == doctest::Approx(pitch12));   // 選項間距一致
}

// 三個終局選項能容納於 110px 對話面板內。
TEST_CASE("5b: three finale options fit inside the 110px dialog panel") {
    nccu::DialogState d = OpenFinaleMenu();
    PosSpy s;
    nccu::DrawDialog(s, d);
    REQUIRE(!s.texts.empty());
    // 最後一列的底部必須留在面板下緣（kBoxY + kBoxH）之內，並保留一個字形
    // 高度的餘裕。
    float maxY = 0.0f;
    for (const auto& t : s.texts) maxY = t.y > maxY ? t.y : maxY;
    CHECK(maxY + kBoxLineH <= kBoxY + kBoxH);
}
