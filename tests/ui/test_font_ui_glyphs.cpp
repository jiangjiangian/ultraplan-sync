#include "doctest/doctest.h"
#include "engine/render/Font.h"
#include "game/world/Buildings.h"
#include <algorithm>
#include <iomanip>
#include <string>
#include <vector>

/**
 * @file test_font_ui_glyphs.cpp
 * @brief 在字型圖集層級守住特定關鍵字形：對話框分頁的「▼」下翻提示、結局卡字卡
 *        中只在 UiLiteralChars() 才有的字（討／厭與引號），以及每個建築名稱所用
 *        的字都必須烘進圖集，否則 raylib 會渲染成無字形的「?」豆腐。
 */
//
// CollectCodepoints() 建構烘進中文字型圖集的字形集合：ASCII 32..126、
// docs/content/*.md 中的每個碼位，再加上 UiLiteralChars()。對話框以 U+25BC（▼）
// 作為「按鍵繼續以閱讀更多」的提示；它不是 ASCII，也不出現在任何內容 .md，故只能
// 透過 UiLiteralChars() 進入圖集。若該 ▼ 項被移除，0x25BC 會自圖集消失，提示就會
// 變成無意義的豆腐「?」。
//
// 結局字卡同理：結局 B 字卡中的「討／厭」不在任何內容檔，故只能透過
// UiLiteralChars() 進入圖集。
//
// 純邏輯 —— CollectCodepoints() 會讀檔並用 raylib 的碼位解碼器，但不需 GL 環境，
// 故可在無頭環境執行。

using nccu::engine::render::detail::CollectCodepoints;

namespace {

bool Contains(const std::vector<int>& v, int cp) {
    return std::find(v.begin(), v.end(), cp) != v.end();
}

} // namespace

// CollectCodepoints 會烘入 U+25BC 下翻提示（▼）。
TEST_CASE("CollectCodepoints bakes the U+25BC down-cue (V1 ▼ fix)") {
    const std::vector<int> cps = CollectCodepoints();
    // 0x25BC ▼：對話框的分頁提示。移除 UiLiteralChars() 的 ▼ 項會使它消失，
    // 提示就會變成豆腐「?」。
    CHECK(Contains(cps, 0x25BC));
    // 任何地方都沒用到 ▲ 上翻提示（對話框只畫下翻提示），故本遊戲 UI 刻意不要求
    // 0x25B2。
}

// CollectCodepoints 會烘入結局字卡的字形。
TEST_CASE("CollectCodepoints bakes the V3 ending-caption glyphs") {
    const std::vector<int> cps = CollectCodepoints();
    // 討（U+8A0E）與厭（U+53AD）只出現在結局 B 字卡「你成為了你曾經最討厭的那種人」——
    // 它們不在任何 docs/content 檔，故只能透過 UiLiteralChars() 進入圖集。
    CHECK(Contains(cps, 0x8A0E));   // 討
    CHECK(Contains(cps, 0x53AD));   // 厭
    // 每段結局字卡都用到的中文引號。
    CHECK(Contains(cps, 0x300C));   // 「
    CHECK(Contains(cps, 0x300D));   // 」
    // ASCII 一定存在（永遠加入的 32..126 區段）。
    CHECK(Contains(cps, static_cast<int>('A')));
}

// 每個建築名稱所用的字都必須在圖集中。View.cpp 會畫出「Inside: 」加上
// World::CurrentBuildingName()（取自 nccu::buildings::kAll）；名稱中若有字既不在
// docs/content 也不在 UiLiteralChars()，就會渲染成無字形的「?」（曾回報的缺字：
// 井/仁/勇/塘/夫/志/泳/雩，因為它們不在任何內容檔）。此處驅動真實的 Buildings.h
// 表，故日後改名而引入未涵蓋的字也會在此失敗。
TEST_CASE("CollectCodepoints covers every Buildings.h name glyph (#10)") {
    const std::vector<int> cps = CollectCodepoints();
    for (const auto& b : nccu::buildings::kAll) {
        // 用 raylib 自己的解碼器把 UTF-8 建築名解成碼位（與 CollectCodepoints
        // 處理字面字串所走的路徑相同）。
        const std::string name{b.name};
        int n = 0;
        int* dec = ::LoadCodepoints(name.c_str(), &n);
        for (int i = 0; i < n; ++i) {
            if (dec[i] <= 0) continue;
            INFO("building '" << name << "' glyph U+" << std::hex << dec[i]);
            CHECK(Contains(cps, dec[i]));
        }
        ::UnloadCodepoints(dec);
    }
    // 抽查當初實際回報缺字的那 8 個（不在任何 docs/content 檔 → 僅透過
    // UiLiteralChars() 區段進入圖集）。
    CHECK(Contains(cps, 0x4E95));   // 井（井塘樓）
    CHECK(Contains(cps, 0x4EC1));   // 仁（大仁樓）
    CHECK(Contains(cps, 0x52C7));   // 勇（大勇樓）
    CHECK(Contains(cps, 0x5858));   // 塘（井塘樓）
    CHECK(Contains(cps, 0x592B));   // 夫（果夫樓）
    CHECK(Contains(cps, 0x5FD7));   // 志（志希樓）
    CHECK(Contains(cps, 0x6CF3));   // 泳（游泳館）
    CHECK(Contains(cps, 0x96E9));   // 雩（風雩樓／風雩走廊）
}
