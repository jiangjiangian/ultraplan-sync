#ifndef HELP_PAGE_VIEW_H_
#define HELP_PAGE_VIEW_H_
#include "gfx/Rect.h"
#include "gfx/Vec2.h"
#include "gfx/Color.h"
#include <functional>
#include <string_view>

namespace nccu::ui {

// Shared 遊戲說明 page renderer (review MINOR: de-duplicate the near-
// identical help-page DRAW that lived in BOTH View.cpp's pause-menu
// overlay and TitleScreen.cpp::RunHelpPage). Both call sites drew the same
// panel + 遊戲說明 title + paged GameHelp body (pitch 17 / blank 8, gold
// 【…】 section headers) + 「第 N／M 頁」 indicator + a gold-bordered 返回
// chip. Only a few values differed; this helper takes those as parameters
// so the two stay PIXEL-IDENTICAL to before while sharing one body of code.
//
// Renderer-agnostic by design: the caller passes a `fillRect` callable so
// the in-game overlay (an IRenderer&) and the title screen (the concrete
// gfx::Renderer) both reuse this without coupling to one drawing class.
// Text is drawn through gfx::TextBuilder directly (exactly as both call
// sites already did). Pure presentation — no World/Player, no input (MVC).

// Parameters that differed between the two original call sites; everything
// else (geometry derived from w/h, pitch, header detection) is shared.
struct HelpPageStyle {
    float                  w;             // viewport / window width
    float                  h;             // viewport / window height
    int                    page;          // 0-based page to draw (clamped)
    nccu::gfx::Color       panelColor;    // overlay 245α vs title 200α
    nccu::gfx::Color       indicatorColor;// overlay light vs title dim grey
    std::string_view       chipLabel;     // "M / E 返回選單" vs "Enter / E 返回"
    float                  chipLabelXOffset; // -58 (overlay) vs -56 (title)
};

// Draw the panel, title, current help page body, page indicator and the
// 返回 chip via `fillRect` (for the rects) + gfx::TextBuilder (for text).
// The caller is responsible for any backdrop drawn BEFORE this (the
// overlay's full-screen scrim / the title screen's Clear) — those differ
// and stay at the call site.
void DrawHelpPage(const std::function<void(nccu::gfx::Rect,
                                           nccu::gfx::Color)>& fillRect,
                  const HelpPageStyle& style);

} // namespace nccu::ui

#endif // HELP_PAGE_VIEW_H_
