#include "ui/HelpPageView.h"
#include "ui/GameHelp.h"
#include "gfx/TextBuilder.h"
#include "engine/math/Color.h"

#include <algorithm>
#include <cstddef>
#include <string>

namespace nccu::ui {

using namespace nccu::gfx;   // Color / Colors / Rect / Vec2 / TextBuilder

void DrawHelpPage(const std::function<void(Rect, Color)>& fillRect,
                  const HelpPageStyle& style) {
    const float W = style.w;
    const float H = style.h;
    const float pad = 24.0f;
    // Panel (alpha differs between the overlay and the title screen → param).
    fillRect(Rect{pad, pad, W - pad * 2.0f, H - pad * 2.0f}, style.panelColor);
    TextBuilder{"遊戲說明"}
        .At(Vec2{W * 0.5f - 52.0f, pad + 6.0f})
        .Size(24).Color(Colors::Gold).Draw();
    // U2-T4: paged help body — pitch 17 (blank 8), gold 【…】 headers.
    const int page = std::max(0, std::min(style.page,
                                          nccu::kGameHelpPageCount - 1));
    const auto isHeader = [](std::string_view s) {
        return !s.empty() && s.front() == static_cast<char>('\xE3');
    };  // 【 is U+3010 → lead byte 0xE3
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
    // 「第 N／M 頁」 indicator + the ←／→ flip hint.
    const std::string ind =
        "第 " + std::to_string(page + 1) + "／" +
        std::to_string(nccu::kGameHelpPageCount) + " 頁   ←／→ 翻頁";
    TextBuilder{ind}.At(Vec2{W * 0.5f - 92.0f, H - pad - 62.0f})
        .Size(15).Color(style.indicatorColor).Draw();
    // Prominent 返回 chip: gold-bordered, bright gold label.
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
