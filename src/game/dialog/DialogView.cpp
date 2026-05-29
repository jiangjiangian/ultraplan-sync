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
        // Choice labels also wrap to the box so a long option can never
        // overflow; each option's rows stack, then a small inter-choice
        // gap, then the next option — so the menu reads as ONE option per
        // logical entry (not a wall of equal-pitch rows), which matters
        // now the box is wide (kBoxCells) and the Ch4 finale offers three
        // options (體諒/質問/我再想想…). The selected option is prefixed
        // with the "> " caret marker so keyboard selection is unambiguous.
        // kInterChoiceGap is intentionally smaller than a full line so the
        // three single-row finale options + their gaps still clear the
        // 110px panel (kBoxH): 3 rows*22 + 2 gaps*8 = 82px < 110.
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
        // The current line, wrapped to the box and shown one page (up to
        // kBoxRowsPerPage rows) at a time — DialogState paginates and
        // the advance key (handled by GameController) turns the page, so
        // text NEVER overflows or clips no matter how long the line is.
        float y = kBoxTextY;
        for (const std::string& row : d.CurrentPageRows()) {
            r.DrawText(row, Vec2{kBoxTextX, y}, kBoxFontSize, Colors::Black);
            y += kBoxLineH;
        }
        // "▼ more" affordance: another page of THIS line, a further
        // line, or a choice menu still to come. Drawn bottom-right
        // inside the panel so the player knows to press advance again.
        if (d.HasMore()) {
            r.DrawText("\xE2\x96\xBC",  // U+25BC ▼
                       Vec2{kBoxX + kBoxW - 24.0f, kBoxY + kBoxH - 22.0f},
                       kBoxFontSize, Colors::DarkGray);
        }
    }
}

} // namespace nccu
