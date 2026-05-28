#ifndef TITLE_SCREEN_H_
#define TITLE_SCREEN_H_
#include "engine/render/Window.h"

namespace nccu {

// What the player chose on the title / home screen.
enum class TitleChoice {
    StartGame,   // → proceed to character select
    Quit,        // → exit the program cleanly
};

// The home screen shown BEFORE gameplay: game title + a keyboard-
// navigable menu (Up/Down to move the cursor, Enter to confirm).
// Three items: 「開始遊戲」 (StartGame), 「遊戲說明」 (an in-place help
// page handled internally — REQUIREMENT #9) and 「離開」 (Quit). Closing
// the window is treated as Quit. Runs its own draw loop on `win` and
// blocks until the player confirms Start or Quit (選 遊戲說明 just shows
// the help page and returns to this menu; the public result stays the
// 2-valued StartGame/Quit so main.cpp's screen flow is unchanged).
//
// HARNESS: never called when the autoplay harness is active — main.cpp
// bypasses the title (and character-select) exactly as the old
// character-select was bypassed, so every .claude/scripts/* playtest
// still drops straight into gameplay with a byte-identical state.jsonl.
TitleChoice RunTitleScreen(gfx::Window& win);

} // namespace nccu

#endif // TITLE_SCREEN_H_
