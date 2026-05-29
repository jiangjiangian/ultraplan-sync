#ifndef ENDING_VIEW_H_
#define ENDING_VIEW_H_
#include "game/state/EndingMenuModel.h"  // IsEndingState + EndingMenuChoice +
                                           // EndingMenuChoiceAt + EndingMenuLabel
                                           // (game-side; this header renders against them)
#include "game/state/SemesterState.h"
#include <string>
#include <string_view>
#include <vector>

namespace nccu {
namespace gfx { class IRenderer; }

// 5c — every literal string the ending screen can render, across ALL
// three endings and every deciding-condition branch, returned from the
// SAME constexpr tables DrawEndingCard draws. The glyph-coverage test
// scans this so a new reason line / condition label whose glyph is not
// baked into gfx::Font.h fails the build's atlas check automatically (no
// silent tofu drift). Pure data — no raylib, no GL.
[[nodiscard]] std::vector<std::string> EndingCardStrings();

// Item 1 — render-only DTO for the enriched ending screen. View.cpp owns
// the World/Player and extracts these primitives into this struct; the
// EndingView then renders WITHOUT touching World/Player and without any
// gameplay logic (MVC purity — the View layer reads model state, it does
// not query it). The booleans mirror the EndingGate flags so EndingView
// can show the SPECIFIC deciding conditions that fired for this run:
//   karma            final 業力 value (shown on every card).
//   hasTrueUmbrella  Flag_HasTrueUmbrella (re-claimed true umbrella, A).
//   consoledTA       Flag_ConsoledTA (體諒助教 at the finale, A).
//   tookCursed       Flag_TookCursedUmbrella (cursed-umbrella path, B).
//   boughtUgly       Flag_BoughtUglyUmbrella (the 醜綠傘 buy, C).
//   finaleChoiceMade Flag_TaFinaleChoiceMade (the 助教 (d) 結算 happened).
// `coldFinale` (B's "最後質問助教") is derived in EndingView as
// finaleChoiceMade && !consoledTA — exactly EndingGate.cpp — so this DTO
// carries only the raw flags, not the gate's verdict.
struct EndingSummary {
    SemesterState state = SemesterState::Ending_C;
    int  karma            = 0;
    bool hasTrueUmbrella  = false;
    bool consoledTA       = false;
    bool tookCursed       = false;
    bool boughtUgly       = false;
    bool finaleChoiceMade = false;
};

// EndingMenuChoice + EndingMenuChoiceAt + EndingMenuLabel live in
// game/state/EndingMenuModel.h (game-side); included above so this
// header still resolves their names for callers that include only
// ui/EndingView.h. The renderer here just reads them; the controller
// drives them.

// Full-screen ending screen: black backdrop + title + the opening 字卡,
// THEN (Item 1) an in-fiction "why you're here" reason and a 結算 stats
// card showing the final karma + the deciding conditions that fired for
// THIS ending, AND (A-T3) a bottom 3-option menu with `menuCursor` (0..2)
// highlighted. All at `alpha` (0..1, the fade-in level). Self-contained
// and spy-testable like DrawDialog — View early-returns into this when
// the semester reaches an ending, so the reason/stats/menu MUST be
// rendered here (the ending .md files are never drawn; see View.cpp). The
// summary supplies every primitive the screen needs; EndingView holds the
// title, caption, reason copy, condition labels and menu labels as
// constexpr tables. menuCursor defaults to 0 (回首頁) for callers/tests
// that don't drive the menu.
void DrawEndingCard(nccu::gfx::IRenderer& r, const EndingSummary& summary,
                    std::string_view title, float alpha,
                    float screenW, float screenH, int menuCursor = 0);
} // namespace nccu
#endif // ENDING_VIEW_H_
