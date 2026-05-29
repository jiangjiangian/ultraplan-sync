#ifndef GAME_STATE_ENDING_MENU_MODEL_H_
#define GAME_STATE_ENDING_MENU_MODEL_H_
#include "game/state/SemesterState.h"
#include <string_view>

namespace nccu {

// Blueprint Phase 4 — game-side ending-screen menu model. Hosts the
// three pieces of model-state the ending screen exposes that
// game/-layer code (scenes, controllers, the FSM observer) needs to
// read without pulling the ui render header — closes a game→ui
// back-edge.
//
// ui/EndingView.h still renders against THESE types; the EndingSummary
// DTO + DrawEndingCard signature stay on the ui side.

// True for Ending_A / Ending_B / Ending_D / Ending_C. Pure FSM-state
// predicate (no rendering); the ending scene's freeze guard reads it
// to know when the ending menu owns the frame.
[[nodiscard]] bool IsEndingState(SemesterState s) noexcept;

// A-T3 — the ending screen's bottom 3-option menu. The ending screen
// is a STABLE, INTERACTIVE screen (no longer a passive card): ←/→
// move the cursor, E/Enter confirm. The three choices, in cursor
// order:
//   0  回首頁     — back to the title screen.
//   1  重新開始   — a fresh new game from Ch1.
//   2  結束       — true quit (the ONLY path that closes the window).
// This enum is the single source of truth for the menu's meaning; the
// LABELS live in EndingMenuLabel(). Both routing semantics are
// realised through World::PendingAppAction in the controller (the
// View only renders the highlighted row).
enum class EndingMenuChoice { BackToTitle, RestartGame, Quit };

// Pure index → choice mapping (cursor 0..2). Out-of-range clamps into
// the valid set so a stray cursor can never select "nothing".
// Testable without any World / renderer.
[[nodiscard]] EndingMenuChoice EndingMenuChoiceAt(int index) noexcept;

// The on-screen label for a choice (CJK; every glyph baked into the
// atlas — these strings are also included in EndingCardStrings() in
// ui/EndingView so the glyph-scan test covers them).
[[nodiscard]] std::string_view EndingMenuLabel(EndingMenuChoice c) noexcept;

} // namespace nccu

#endif // GAME_STATE_ENDING_MENU_MODEL_H_
