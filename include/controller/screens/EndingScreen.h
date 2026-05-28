#ifndef CONTROLLER_SCREENS_ENDING_SCREEN_H_
#define CONTROLLER_SCREENS_ENDING_SCREEN_H_

namespace nccu {

class World;

// A-T3: the ENDING SCREEN is a stable, interactive screen with a bottom
// 3-option menu (回首頁 / 重新開始 / 結束). When the game has reached an
// Ending_* state this fully FREEZES the world (the controller returns
// before the pipeline / interact / sweep), so the player's ONLY agency
// is this menu. ←/→ move the cursor; E/Enter confirm the highlighted
// choice and route it through World::RequestAppAction. 回首頁 and
// 重新開始 both request Restart (main.cpp tears the run down to the
// title — a full state reset; for 重新開始 the player then re-enters
// from the title, the accepted "Restart→title→select" flow); 結束
// requests Quit (the only path that closes the window). ESC is NOT
// read here — it stays the program's quit key owned by main.cpp.
//
// Returns true while an Ending_* state is current (the screen is up and
// owns the frame); false otherwise (the controller falls through).
[[nodiscard]] bool HandleEndingMenu(World& world);

} // namespace nccu

#endif // CONTROLLER_SCREENS_ENDING_SCREEN_H_
