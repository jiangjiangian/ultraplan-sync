#include "game/dialog/DialogView.h"
#include "game/dialog/DialogLayout.h"
#include "engine/render/IRenderer.h"
#include "engine/math/Rect.h"
#include "engine/math/Vec2.h"
#include "engine/math/Color.h"
#include <string>
#include <vector>

namespace nccu {

void DrawDialog(nccu::engine::render::IRenderer& r, const DialogState& d) {
    using namespace nccu::engine::render;
    using namespace nccu::engine::math;
    using namespace nccu::dialog;
    if (!d.Active()) return;

    r.DrawRect(Rect{kBoxX, kBoxY, kBoxW, kBoxH}, Colors::RayWhite);
    r.DrawRect(Rect{kBoxX, kBoxY, kBoxW, 2.0f},  Colors::DarkGray);

    if (d.AtChoice()) {
        // 選項標籤同樣斷行至框內，長選項絕不溢出；每個選項的列先堆疊，再加一小段選項間距，
        // 再接下一個選項——使選單讀作「一個邏輯項一個選項」（而非一片等距列），這在框變寬
        // （kBoxCells）且 Ch4 終局提供三個選項（體諒／質問／我再想想…）時尤其重要。被選中
        // 的選項前綴 "> " 游標標記，使鍵盤選取明確無歧義。kInterChoiceGap 刻意小於整行高，
        // 使三個單列終局選項加間距仍容於 110px 面板（kBoxH）：3 列*22 + 2 間距*8 = 82px < 110。
        constexpr float kInterChoiceGap = 8.0f;
        float y = kBoxTextY;
        const auto& choices = d.Choices();
        for (int i = 0; i < static_cast<int>(choices.size()); ++i) {
            const bool sel = (i == d.ChoiceCursor());
            const std::string mark = sel ? "> " : "  ";
            const auto rows = WrapToCells(
                mark + choices[static_cast<std::size_t>(i)].label, kBoxCells);
            for (const std::string& row : rows) {
                r.DrawText(row, Vec2{kBoxTextX, y}, kBoxFontSize,
                           Colors::Black);
                y += kBoxLineH;
            }
            if (i + 1 < static_cast<int>(choices.size())) y += kInterChoiceGap;
        }
    } else {
        // 當前行斷行至框內，每次顯示一頁（至多 kBoxRowsPerPage 列）——分頁由 DialogState
        // 負責，前進鍵（GameController 處理）翻頁，故不論行多長，文字「絕不」溢出或被裁切。
        float y = kBoxTextY;
        for (const std::string& row : d.CurrentPageRows()) {
            r.DrawText(row, Vec2{kBoxTextX, y}, kBoxFontSize, Colors::Black);
            y += kBoxLineH;
        }
        // 「▼ 更多」提示：本行還有頁、還有後續行、或後面還有選單。畫在面板右下角，
        // 讓玩家知道要再按一次前進
        if (d.HasMore()) {
            r.DrawText("\xE2\x96\xBC",  // U+25BC ▼
                       Vec2{kBoxX + kBoxW - 24.0f, kBoxY + kBoxH - 22.0f},
                       kBoxFontSize, Colors::DarkGray);
        }
    }
}

} // namespace nccu
