#include "ui/MessageView.h"
#include "ui/ReducedMotion.h"
#include "dialog/DialogLayout.h"   // WrapToCells / CellWidth — the project's
                                   // EAW-aware wrap + measure (CJK = 2 cells)
#include "gfx/IRenderer.h"
#include "engine/math/Rect.h"
#include "engine/math/Vec2.h"
#include "engine/math/Color.h"
#include <algorithm>
#include <cstddef>
#include <string>
#include <vector>

namespace nccu {

using namespace nccu::gfx;

namespace {

// Banner text size and the layout pad around it.
constexpr int   kFontSize = 18;
constexpr float kPadX     = 18.0f;
constexpr float kPadY     = 12.0f;
constexpr float kLineH    = 24.0f;   // > kFontSize: a little leading
constexpr float kMarginB  = 28.0f;   // gap from the screen bottom

// Pixels per EAW cell at kFontSize. The project's font-independent text
// model (EndingView::CenteredX, ChapterCard, DialogLayout.h) advances a
// full-width glyph ~`sz` px = 2 cells, so ~`sz/2` px per cell. Sharing
// this with dialog::CellWidth keeps the toast measure identical to the
// dialog box / ending card — one source of truth, not a private estimate.
constexpr float kPxPerCell = static_cast<float>(kFontSize) * 0.5f;

// Pixel width of `s` at kFontSize via the cell model.
float TextWidthPx(const std::string& s) {
    return static_cast<float>(nccu::dialog::CellWidth(s)) * kPxPerCell;
}

// UI-B-3: wrap the toast to a CELL budget (not a private pixel estimate)
// using the project's EAW-aware nccu::dialog::WrapToCells, so a long
// ShowMessage never spills the panel and a literal '\n' still forces a
// break (WrapToCells honours '\n' exactly like the old WrapCjk did, and
// like DialogView). maxWidth px → cells via the shared px/cell model.
std::vector<std::string> WrapToBox(const std::string& s, float maxWidth) {
    const int maxCells =
        std::max(1, static_cast<int>(maxWidth / kPxPerCell));
    return nccu::dialog::WrapToCells(s, maxCells);
}

}  // namespace

void DrawHudMessage(IRenderer& r, const std::string& message,
                    float age, float screenW, float screenH,
                    bool reducedMotion, HudSlot slot) {
    if (message.empty() || age >= kHudTtl) return;  // nothing to show

    // Lifetime → alpha. Hold opaque, then ramp 1→0 across the final
    // kHudFade seconds — the raylib-extras Timer idiom (a countdown
    // remaining = lifetime - elapsed, gone at <= 0) recast as a fade.
    // Audit D8 / SC 2.3.3: HudToastFadeT collapses the fade to a hard
    // cut when reducedMotion is true (holds opaque until TTL boundary,
    // then DrawHudMessage's early-return above hides the toast in
    // one frame).
    const float remaining = kHudTtl - age;
    const float t = HudToastFadeT(remaining, kHudFade, reducedMotion);
    const auto  a = static_cast<unsigned char>(t * 255.0f);

    const float maxTextW = screenW * 0.72f;
    const std::vector<std::string> lines = WrapToBox(message, maxTextW);

    // Banner box sized to the widest wrapped line, centred horizontally.
    // Cycle 9.G — vertical anchor depends on slot:
    //   Bottom -> screen-bottom - kMarginB - boxH (pre-9.G position).
    //   Top    -> bottomBaseline - kSlotGap - boxH (a fixed visual band
    //             above the bottom slot, sized for a 1-line bottom toast
    //             so the Top band is stable even when only Top is live).
    // The gap is a layout constant rather than reading the live Bottom
    // height: the View calls Top BEFORE Bottom and the two are pure
    // functions of their own state, so coupling them would force a
    // measurement pass. ~25-30 px between bands keeps both lines
    // legible without overlap; the GUI manual-verify confirms it.
    float widest = 0.0f;
    for (const std::string& ln : lines)
        widest = std::max(widest, TextWidthPx(ln));

    const float boxW = widest + kPadX * 2.0f;
    const float boxH = static_cast<float>(lines.size()) * kLineH +
                       kPadY * 2.0f;
    const float boxX = (screenW - boxW) * 0.5f;
    // Bottom-slot baseline: pre-9.G position. Top-slot floats above it
    // by the fixed bottom-slot single-line height plus a 12 px gap.
    const float bottomBaseline = screenH - kMarginB;
    constexpr float kSlotGap   = 12.0f;
    const float kSingleLineH   = kLineH + kPadY * 2.0f;  // 1-line tall
    const float boxY =
        slot == HudSlot::Top
            ? bottomBaseline - kSingleLineH - kSlotGap - boxH
            : bottomBaseline - boxH;

    // Backdrop carries the same alpha as the text so the whole toast
    // fades as one (mirrors DrawEndingCard's self-contained fade).
    r.DrawRect(Rect{boxX, boxY, boxW, boxH}, Color{20, 20, 20, a});
    r.DrawRect(Rect{boxX, boxY, boxW, 2.0f},
               Color{245, 245, 245, a});

    // UI-B-3: CENTRE each wrapped row inside the box (was left-aligned at
    // boxX+kPadX). The box hugs the widest row, so a multi-line toast like
    // the DLC teaser (「DLC開發中」 over 「敬請期待」) reads as two balanced
    // centred lines instead of a ragged left edge. Centre offset is per-row
    // via the shared cell model, so it tracks each line's true width.
    float y = boxY + kPadY;
    const float innerW = boxW - kPadX * 2.0f;
    for (const std::string& ln : lines) {
        const float lineW = TextWidthPx(ln);
        const float cx = boxX + kPadX + std::max(0.0f, (innerW - lineW) * 0.5f);
        r.DrawText(ln, Vec2{cx, y}, kFontSize, Color{245, 245, 245, a});
        y += kLineH;
    }
}

} // namespace nccu
