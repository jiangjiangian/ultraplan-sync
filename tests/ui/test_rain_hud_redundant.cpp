/**
 * @file test_rain_hud_redundant.cpp
 * @brief 驗證雨量 HUD 讀數的色盲冗餘設計：RainTierPrefix 依雨量門檻回傳
 *        固定 2 字寬的標記（calm/warn/crit），讓等級不必依賴顏色也能辨識。
 */
//
// 雨量 HUD 文字原本只用白→金→紅的色階傳達等級；紅綠色盲玩家看金色與紅色幾乎
// 都是相近的橄欖／棕色。現於文字前加上 2 字寬前綴，不靠色相也能讀出等級：
//   雨量 <  60       → "  "（平靜）
//   60 ≤ 雨量 < 85   → " !"（警告）
//   雨量 >= 85       → "!!"（危急）
// 色階本身仍保留，作為輔助強化。
//
// 此處測試 View 用來組字串的純輔助函式 RainTierPrefix()。View::Draw 本身走 raylib
// （無 GL 環境），但等級前綴正是依雨量推導的關鍵邏輯。

#include "doctest/doctest.h"
#include "ui/RainHud.h"
#include <cstdio>
#include <string>
#include <string_view>

using nccu::RainTierPrefix;

// RainTierPrefix 依門檻回傳平靜／警告／危急三級前綴。
TEST_CASE("RainTierPrefix 依門檻回傳平靜／警告／危急三級前綴") {
    // 低於 60：平靜 — 兩個空格。空格保留欄位，讓 "rain: NN%" 的數字仍與其他
    // HUD 行對齊。
    CHECK(RainTierPrefix(0.0f)   == "  ");
    CHECK(RainTierPrefix(30.0f)  == "  ");
    CHECK(RainTierPrefix(59.99f) == "  ");

    // [60, 85)：警告 — 單一 '!'。
    CHECK(RainTierPrefix(60.0f)  == " !");
    CHECK(RainTierPrefix(72.5f)  == " !");
    CHECK(RainTierPrefix(84.99f) == " !");

    // >= 85：危急 — 雙 '!!'。
    CHECK(RainTierPrefix(85.0f)  == "!!");
    CHECK(RainTierPrefix(90.0f)  == "!!");
    CHECK(RainTierPrefix(100.0f) == "!!");
}

// 不論哪一級，前綴都恰好佔 2 個可見字寬。
TEST_CASE("不論哪一級前綴都恰好佔 2 個可見字寬") {
    // 寬度不變量：雨量 HUD 行依固定欄距排版，故前綴不論等級都必須是固定的
    // 2 字寬（不縮成 "!"、也不拉成 "!!!"），以維持與上下方 karma／傘／金幣行的
    // 欄位對齊。
    CHECK(RainTierPrefix(0.0f).size()   == 2u);
    CHECK(RainTierPrefix(70.0f).size()  == 2u);
    CHECK(RainTierPrefix(95.0f).size()  == 2u);
}

// 中等級的 HUD 行會帶有警告標記。
TEST_CASE("中等級的 HUD 行帶有警告標記") {
    // 端到端形態：複製 View::Draw 組 rbuf 的 snprintf 呼叫，再檢查結果文字，
    // 確認此輔助函式能正確組進 View 使用的格式字串。
    constexpr float midTier = 70.0f;
    const std::string_view tag = RainTierPrefix(midTier);
    char rbuf[40] = {0};
    std::snprintf(rbuf, sizeof(rbuf), "%.*s rain: %d%%",
        static_cast<int>(tag.size()), tag.data(),
        static_cast<int>(midTier + 0.5f));
    const std::string s(rbuf);
    CHECK(s.find(" !") != std::string::npos);
    CHECK(s.find("rain: 70%") != std::string::npos);
}

// 危急等級的 HUD 行會帶有 !! 標記。
TEST_CASE("危急等級的 HUD 行帶有 !! 標記") {
    constexpr float critTier = 90.0f;
    const std::string_view tag = RainTierPrefix(critTier);
    char rbuf[40] = {0};
    std::snprintf(rbuf, sizeof(rbuf), "%.*s rain: %d%%",
        static_cast<int>(tag.size()), tag.data(),
        static_cast<int>(critTier + 0.5f));
    const std::string s(rbuf);
    CHECK(s.find("!!") != std::string::npos);
    CHECK(s.find("rain: 90%") != std::string::npos);
}
