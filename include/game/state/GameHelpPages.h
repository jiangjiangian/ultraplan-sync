#ifndef GAME_STATE_GAME_HELP_PAGES_H_
#define GAME_STATE_GAME_HELP_PAGES_H_

namespace nccu {

// Blueprint Phase 4 — game-side page count for the 遊戲說明 overlay.
// Extracted from ui/GameHelp.h so the game-layer scenes / controllers
// (TitleScene, PauseScreen) that page through the help overlay can
// read the count without pulling the ui render header — closes a
// game→ui back-edge. ui/GameHelp.h still defines kGameHelpPages
// (the actual content string_views), and the value here is the array
// size; the two are statically consistent (the static_assert at the
// bottom of ui/GameHelp.h pins them).
//
// The constant is duplicated here intentionally (2 is a small literal)
// rather than transitively pulled in, so the game layer's compilation
// stays independent of ui. A future content change that adds a third
// page bumps this AND the ui side together — the static_assert keeps
// them honest.
inline constexpr int kGameHelpPageCount = 2;

} // namespace nccu

#endif // GAME_STATE_GAME_HELP_PAGES_H_
