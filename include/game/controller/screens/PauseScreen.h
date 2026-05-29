#ifndef CONTROLLER_SCREENS_PAUSE_SCREEN_H_
#define CONTROLLER_SCREENS_PAUSE_SCREEN_H_

namespace nccu {

class World;

// In-game pause menu (top-right). Opens with M; while open the world is
// fully frozen (the orchestrator returns before the object tick /
// movement / rain / sweep, exactly like the dialog/inventory freezes).
// Checked AFTER the ending screen so the pause takes precedence over a
// dialog or the Tab inventory. M also closes it (toggle / Resume).
//
// REQUIREMENT #9: the 說明 help overlay sits ON TOP of the paused menu;
// while it is up, M/E/Enter dismisses it back to the menu and the menu
// cursor / sim stay frozen. The overlay is paged (←/→ flip between the
// 操作+目標 and 雨傘外觀+道具須知+結局 pages — U2-T4).
//
// ESC is NOT read here — it stays the program's direct-quit key owned by
// main.cpp. Restart/Quit only RECORD an intent on the World — the actual
// teardown happens in main.cpp's outer loop (BUGLEDGER B2/H1).
//
// Returns true while the menu (or its help overlay) is up, OR on the
// frame M opens it; false when no menu is involved.
[[nodiscard]] bool HandlePauseMenu(World& world);

} // namespace nccu

#endif // CONTROLLER_SCREENS_PAUSE_SCREEN_H_
