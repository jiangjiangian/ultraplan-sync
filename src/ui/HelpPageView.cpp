#include "ui/HelpPageView.h"
#include "ui/GameHelp.h"
#include "engine/render/TextBuilder.h"
#include "engine/math/Color.h"

#include <algorithm>
#include <cstddef>
#include <string>

/**
 * @file HelpPageView.cpp
 * @brief 遊戲說明分頁的渲染：底色面板、標題、分頁內文、頁碼指示與返回按鈕。
 */

namespace nccu::ui {

using namespace nccu::engine::render;
using namespace nccu::engine::math;

/**
 * @brief 繪製單一頁的遊戲說明內容。
 *
 * 透過注入的 fillRect 繪製矩形（而非直接依賴 IRenderer），讓疊層與標題畫面能各自
 * 帶入不同的面板透明度與底色。page 先夾在合法範圍內，避免越界存取分頁表；內文以
 * 起首位元組判斷 【…】 標題列改用金色，其餘為白色。版面數值（行距、邊距、按鈕尺寸）
 * 為手調的固定值。
 */
void DrawHelpPage(const std::function<void(Rect, Color)>& fillRect,
                  const HelpPageStyle& style) {
    const float W = style.w;
    const float H = style.h;
    const float pad = 24.0f;
    // 底色面板（疊層與標題畫面的透明度不同，故由參數帶入 panelColor）。
    fillRect(Rect{pad, pad, W - pad * 2.0f, H - pad * 2.0f}, style.panelColor);
    TextBuilder{"遊戲說明"}
        .At(Vec2{W * 0.5f - 52.0f, pad + 6.0f})
        .Size(24).Color(Colors::Gold).Draw();
    // 分頁的說明內文：行距 17（空行 8），金色的 【…】當標題列。
    const int page = std::max(0, std::min(style.page,
                                          nccu::kGameHelpPageCount - 1));
    const auto isHeader = [](std::string_view s) {
        return !s.empty() && s.front() == static_cast<char>('\xE3');
    };  // 【 為 U+3010，UTF-8 起首位元組 0xE3，藉此判斷是否為標題列
    float hy = pad + 40.0f;
    for (const std::string_view ln :
         nccu::kGameHelpPages[static_cast<std::size_t>(page)]) {
        if (!ln.empty())
            TextBuilder{std::string{ln}}
                .At(Vec2{pad + 22.0f, hy})
                .Size(15)
                .Color(isHeader(ln) ? Colors::Gold : Colors::White).Draw();
        hy += ln.empty() ? 8.0f : 17.0f;
    }
    // 「第 N／M 頁」頁碼指示，外加 ←／→ 翻頁提示。
    const std::string ind =
        "第 " + std::to_string(page + 1) + "／" +
        std::to_string(nccu::kGameHelpPageCount) + " 頁   ←／→ 翻頁";
    TextBuilder{ind}.At(Vec2{W * 0.5f - 92.0f, H - pad - 62.0f})
        .Size(15).Color(style.indicatorColor).Draw();
    // 醒目的「返回」按鈕：金色邊框、亮金色標籤。
    const float chipW = 188.0f, chipH = 26.0f;
    const float chipX = W * 0.5f - chipW * 0.5f;
    const float chipY = H - pad - chipH - 8.0f;
    fillRect(Rect{chipX, chipY, chipW, chipH}, Color{62, 52, 18, 255});
    fillRect(Rect{chipX, chipY, chipW, 2.0f}, Colors::Gold);
    fillRect(Rect{chipX, chipY + chipH - 2.0f, chipW, 2.0f}, Colors::Gold);
    TextBuilder{std::string{style.chipLabel}}
        .At(Vec2{W * 0.5f + style.chipLabelXOffset, chipY + 5.0f})
        .Size(17).Color(Colors::Gold).Draw();
}

} // namespace nccu::ui
