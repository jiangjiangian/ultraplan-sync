#include "doctest/doctest.h"
#include "engine/render/Font.h"
#include "ui/EndingView.h"
#include "ui/ChapterCard.h"
#include "ui/GameHelp.h"
#include "game/quest/ItemCatalog.h"
#include "game/quest/QuestObjective.h"
#include "game/vendor/VendorMessages.h"
#include <algorithm>
#include <iomanip>
#include <string>
#include <vector>

/**
 * @file test_font_ui_glyph_scan.cpp
 * @brief 全面的字形涵蓋閘：掃描各種由程式組出的 UI 字串（結局卡、章節卡、遊戲說明、
 *        View 的 HUD/選單/物品欄字面、任務目標、道具表名稱與說明、商人提示），
 *        若其中任一字未烘進中文字型圖集（CollectCodepoints()）即失敗，以免日後新增
 *        未涵蓋的字而在畫面上變成「?」豆腐。
 */
//
// 凡是程式組出的 UI 字串所用的每個字，都必須烘進中文字型圖集（CollectCodepoints()），
// 否則 raylib 會渲染成無字形的「?」豆腐。test_font_ui_glyphs.cpp 固定特定的歷史字形；
// 本檔則「掃描」實際的 UI 字面字串，只要有任一字缺漏就失敗，使日後新增未涵蓋的字
// （新的結局緣由台詞、說明文案調整、選單標籤）不會悄悄退回豆腐。這些字串來自渲染器
// 實際繪製的同一來源。掃描範圍涵蓋玩家曾看到「?」的每一處由程式組出的 UI 字串：
// HUD 目標文字、道具表名稱與說明、商人提示片段、雨傘外觀說明行，以及結局字串。
//
// 無頭安全：CollectCodepoints() 與 raylib 的碼位解碼器都不需 GL 環境。

using nccu::engine::render::detail::CollectCodepoints;

namespace {

// 用 raylib 自己的解碼器把 UTF-8 字串解成碼位（與 CollectCodepoints 所走路徑相同），
// 並斷言每個碼位都在圖集集合內。
void RequireAllCovered(const std::vector<int>& atlas, const std::string& s,
                       const char* whatFor) {
    int n = 0;
    int* dec = ::LoadCodepoints(s.c_str(), &n);
    for (int i = 0; i < n; ++i) {
        const int cp = dec[i];
        if (cp <= 0) continue;
        // ASCII 控制字元／空白與可列印字一定在圖集中（CollectCodepoints 無條件加入
        // 32..126）；略過解碼器在格式不正確的尾端所產生的無字形 0。
        INFO(whatFor << " glyph U+" << std::hex << std::setw(4)
                     << std::setfill('0') << cp << " in: " << s);
        CHECK(std::find(atlas.begin(), atlas.end(), cp) != atlas.end());
    }
    ::UnloadCodepoints(dec);
}

// View.cpp 中由程式組出的 HUD／選單／物品欄／操作提示字面（僅中文者 —— 純 ASCII
// 字串不需檢查圖集）。此處作為自動推導的結局／說明集合之手動補充；若新增的 View
// 字面引入了新字形，請加到這裡讓閘涵蓋它。
const std::vector<std::string>& ViewLiterals() {
    static const std::vector<std::string> kV = {
        "金幣: %d 元",
        "M 選單",
        "Tab: 物品欄   M: 選單",   // 左上角操作提示
        "遊戲選單",
        "繼續", "說明", "減少動畫", "擴大目標", "重新開始", "離開",
        "  [開]", "  [關]",
        "↑ ↓ 選擇   Enter 確認   M 繼續",
        "遊戲說明",
        "M / E 返回選單",
        "物品欄",
        "（空）",
        "\xE2\x96\xBC more",   // ▼ more（對話框的提示）
    };
    return kV;
}

} // namespace

// 結局畫面的每個字形都已烘進字型圖集。
TEST_CASE("結局畫面的每個字形都烘進字型圖集") {
    const std::vector<int> atlas = CollectCodepoints();
    const std::vector<std::string> strs = nccu::EndingCardStrings();
    CHECK(strs.size() > 10);          // 確認：表確實有列舉內容
    for (const std::string& s : strs)
        RequireAllCovered(atlas, s, "ending-screen");
}

// 章節書擋大卡的每個字形都已烘進圖集。
TEST_CASE("章節書擋大卡的每個字形都烘進圖集") {
    const std::vector<int> atlas = CollectCodepoints();
    const std::vector<std::string> strs = nccu::ChapterCardStrings();
    CHECK(strs.size() >= 8);          // 4 對章節 Lost + 1 對 Found
    for (const std::string& s : strs)
        RequireAllCovered(atlas, s, "chapter-card");
}

// 遊戲說明（GameHelp）的每個字形都已烘進圖集。
TEST_CASE("遊戲說明（GameHelp）的每個字形都烘進圖集") {
    const std::vector<int> atlas = CollectCodepoints();
    for (const std::string_view ln : nccu::kGameHelpLines)
        RequireAllCovered(atlas, std::string{ln}, "help-line");
    RequireAllCovered(atlas, std::string{nccu::kGameHelpClosing},
                      "help-closing");
}

// 說明現已分頁（kGameHelpPages）。連分頁視圖也一併掃描，使某行加到分頁中卻未同步到
// 扁平 kGameHelpLines 的字形仍不致在畫面上變豆腐 —— 渲染器畫的是分頁。
TEST_CASE("分頁後的遊戲說明每個字形都烘進圖集") {
    const std::vector<int> atlas = CollectCodepoints();
    CHECK(nccu::kGameHelpPageCount == 2);
    int pageNo = 0;
    for (const auto page : nccu::kGameHelpPages) {
        ++pageNo;
        for (const std::string_view ln : page)
            RequireAllCovered(atlas, std::string{ln}, "help-page");
    }
    CHECK(pageNo == 2);
}

// View.cpp 的每個 HUD/選單/物品欄字面字形都已烘入。
TEST_CASE("View.cpp 的每個 HUD／選單／物品欄字面字形都烘入") {
    const std::vector<int> atlas = CollectCodepoints();
    for (const std::string& s : ViewLiterals())
        RequireAllCovered(atlas, s, "view-literal");
}

// 每個 HUD 目標文字的字形都已烘入。
TEST_CASE("每個 HUD 目標文字的字形都烘入") {
    const std::vector<int> atlas = CollectCodepoints();
    const std::vector<std::string> objs = nccu::QuestObjectiveStrings();
    CHECK(objs.size() >= 9);          // 確認：每個章節／狀態都有列舉
    for (const std::string& s : objs)
        RequireAllCovered(atlas, s, "objective");
}

// 每個道具表名稱與說明的字形都已烘入。
TEST_CASE("每個 ItemCatalog 名稱與說明的字形都烘入") {
    const std::vector<int> atlas = CollectCodepoints();
    const std::vector<std::string> cat = nccu::CatalogStrings();
    CHECK(cat.size() > 10);           // 確認：道具表已攤平
    for (const std::string& s : cat)
        RequireAllCovered(atlas, s, "catalog");
}

// 每個商人提示／訊息的字形都已烘入。
TEST_CASE("每個 Vendor 提示／訊息的字形都烘入") {
    const std::vector<int> atlas = CollectCodepoints();
    namespace m = nccu::vendor::msg;
    const std::vector<std::string> v = {
        std::string{m::kInsufficientFunds}, std::string{m::kPurchasedPrefix},
        std::string{m::kSpentMid},          std::string{m::kSpentUnitOpen},
        std::string{m::kSpentUnitClose},    std::string{m::kSoldOut},
        std::string{m::kStockLineSep},      std::string{m::kStockLineUnit},
    };
    for (const std::string& s : v)
        RequireAllCovered(atlas, s, "vendor-msg");
}
