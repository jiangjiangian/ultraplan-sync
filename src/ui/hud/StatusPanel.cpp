#include "ui/hud/StatusPanel.h"

#include "game/world/World.h"
#include "game/entities/Player.h"
#include "ui/RainHud.h"                   // RainTierPrefix
#include "engine/render/IRenderer.h"
#include "engine/render/TextBuilder.h"
#include "engine/math/Color.h"
#include "engine/math/Rect.h"
#include "engine/math/Vec2.h"

#include <algorithm>
#include <cstdio>
#include <string>
#include <string_view>

/**
 * @file StatusPanel.cpp
 * @brief 左上角 HUD 狀態面板：操作提示、業力／雨傘、金錢、所在建築、章節與降雨讀數。
 */

namespace nccu {

/**
 * @brief 繪製左上角的狀態面板。
 *
 * 只讀取 World／Player 資料、絕不改寫狀態（MVC 純度），每幀重畫故所有讀數即時更新。
 * 各行畫在半透明深色底矩形上，使數值在任何明亮地圖背景上都清楚可讀；面板寬度依各行
 * 字數估計（CJK 全形字計兩倍寬），高度依實際行數而定（Inside 行為條件性）。
 */
void DrawStatusPanel(nccu::engine::render::IRenderer& r, const World& world) {
    using namespace nccu::engine::render;
    using namespace nccu::engine::math;

    // 左上角狀態：WASD 提示、業力／雨傘、選用的建築、章節名稱、降雨計量。先前是直接畫在
    // 明亮世界地圖上的素色深灰／藍色文字——幾乎不可讀。現在重用已驗證的目標面板慣用法
    //（半透明的 Color{20,22,30,185} 底色矩形 + 明亮文字），使每個數值在任何背景上都突出。
    constexpr float kHudX    = 10.0f;
    constexpr float kHudY    = 10.0f;
    constexpr float kLineH   = 20.0f;
    constexpr int   kHudSize = 16;
    constexpr float kPad     = 6.0f;

    const Player* p = world.GetPlayer();
    char kbuf[64] = {0};
    if (p)
        std::snprintf(kbuf, sizeof(kbuf), "karma: %d   umbrella: %s",
            p->GetKarma(), p->HasUmbrella() ? "yes" : "no");
    // 金錢讀數——玩家在遊玩中必須隨時看到自己的錢包（金幣 HUD 功能）。只讀取 World／
    // Player 資料；View 絕不改寫狀態（MVC 純度）。即時更新，因為 HUD 每幀都由 GetMoney()
    // 重畫。
    char mbuf[48] = {0};
    if (p)
        std::snprintf(mbuf, sizeof(mbuf), "金幣: %d 元", p->GetMoney());
    const bool inside = !world.CurrentBuildingName().empty();
    const std::string insideLine =
        inside ? ("Inside: " + world.CurrentBuildingName())
               : std::string{};
    const std::string chapter{world.Semester().CurrentName()};
    // 在前面加上一個冗餘的等級字形，使降雨讀數的三個壓力等級無需僅靠白→金→紅的漸層也能
    // 辨識（紅綠色覺者會把金色與紅色看成幾乎相同的橄欖／褐色）。下方的色彩漸層保留作為
    // 加強。
    char rbuf[40] = {0};
    if (p) {
        const std::string_view tag =
            RainTierPrefix(p->GetRainMeter());
        std::snprintf(rbuf, sizeof(rbuf), "%.*s rain: %d%%",
            static_cast<int>(tag.size()), tag.data(),
            static_cast<int>(p->GetRainMeter() + 0.5f));
    }

    // 操作提示。移動／拾取那一行，外加第二行，把兩個疊層按鍵（Tab → 物品欄、M → 選單）
    // 呈現在玩家最先看的左上角 HUD。物品欄／選單已烘焙進字形圖集（UiLiteralChars）；使用
    // ASCII ':'（與 WASD 行一致），故不需新字形。以具名區域變數，使寬度估計與繪製使用
    //「相同」的文字。
    const std::string ctrlLine1 = "WASD: move    E: pick up";
    const std::string ctrlLine2 = "Tab: 物品欄   M: 選單";

    // 實際存在的行數（Inside 是條件性的）。寬度像下方目標面板一樣由 UTF-8 起首位元組數
    // 估計——章節名稱是 CJK，故最壞情況每字形約 size px。
    int rows = 1;                         // WASD 提示
    rows += 1;                            // Tab/M 操作提示
    if (p)    rows += 1;                  // 業力／雨傘
    if (p)    rows += 1;                  // 金幣
    if (inside) rows += 1;                // Inside
    rows += 1;                            // 章節
    if (p)    rows += 1;                  // 降雨
    auto glyphsOf = [](const std::string& s) {
        int g = 0;
        for (unsigned char c : s) if ((c & 0xC0) != 0x80) ++g;
        return static_cast<std::size_t>(g);
    };
    std::size_t maxGlyphs = ctrlLine1.size();
    // Tab/M 提示行混合 ASCII 與 4 個全形 CJK（物品欄／選單）。把它的 CJK 字數計兩倍
    // （全形）再加上 ASCII 部分，讓黑色底色加寬到足以容納較長的提示。glyphsOf 只數
    // 碼點；額外再加一次 CJK 字數，藉以近似全形格佔用的多餘寬度（與章節／金幣行同理）。
    auto cjkGlyphsOf = [](const std::string& s) {
        int g = 0;  // 起首位元組 0xE0.. = 本遊戲使用的 3 位元組 CJK BMP
        for (unsigned char c : s) if ((c & 0xF0) == 0xE0) ++g;
        return static_cast<std::size_t>(g);
    };
    maxGlyphs = std::max(maxGlyphs,
                         glyphsOf(ctrlLine2) + cjkGlyphsOf(ctrlLine2));
    maxGlyphs = std::max(maxGlyphs, glyphsOf(kbuf));
    // 金幣行為 CJK（3 個全形漢字＋數字）——比照章節行把起首位元組字數計兩倍，
    // 使面板在任何金錢數值下都夠寬。
    maxGlyphs = std::max(maxGlyphs, glyphsOf(mbuf) * 2);
    if (inside) maxGlyphs = std::max(maxGlyphs, glyphsOf(insideLine));
    maxGlyphs = std::max(maxGlyphs, glyphsOf(chapter) * 2);  // CJK 全形
    maxGlyphs = std::max(maxGlyphs, glyphsOf(rbuf));
    const float panelW =
        static_cast<float>(maxGlyphs) * (kHudSize * 0.55f) + kPad * 2.0f;
    const float panelH =
        static_cast<float>(rows) * kLineH + kPad * 2.0f - 4.0f;
    r.DrawRect(Rect{kHudX - kPad, kHudY - kPad, panelW, panelH},
               Color{20, 22, 30, 185});

    float y = kHudY;
    TextBuilder{ctrlLine1}
        .At(Vec2{kHudX, y}).Size(kHudSize).Color(Colors::White).Draw();
    y += kLineH;
    // Tab/M 操作提示，用柔和的灰色，使它讀起來像主要 WASD/E 行之下的次要提示。
    TextBuilder{ctrlLine2}
        .At(Vec2{kHudX, y}).Size(kHudSize).Color(Color{200, 205, 215, 255}).Draw();
    y += kLineH;
    if (p) {
        TextBuilder{kbuf}
            .At(Vec2{kHudX, y}).Size(kHudSize).Color(Colors::White).Draw();
        y += kLineH;
    }
    if (p) {
        // Gold so the purse reads at a glance and matches the coin
        // motif; stays on the dark panel so it pops on any map tile.
        TextBuilder{mbuf}
            .At(Vec2{kHudX, y}).Size(kHudSize).Color(Colors::Gold).Draw();
        y += kLineH;
    }
    if (inside) {
        TextBuilder{insideLine}
            .At(Vec2{kHudX, y}).Size(kHudSize).Color(Colors::Gold).Draw();
        y += kLineH;
    }
    TextBuilder{chapter}
        .At(Vec2{kHudX, y}).Size(kHudSize).Color(Colors::Gold).Draw();
    y += kLineH;
    if (p) {
        // Rain readout colour ramps with the meter so the rising risk
        // is legible at a glance (tiers mirror the vignette below).
        const float rm = p->GetRainMeter();
        const Color rc = rm >= 85.0f ? Colors::Red
                       : rm >= 60.0f ? Colors::Gold
                                     : Colors::White;
        TextBuilder{rbuf}
            .At(Vec2{kHudX, y}).Size(kHudSize).Color(rc).Draw();
        y += kLineH;
    }
}

}  // namespace nccu
