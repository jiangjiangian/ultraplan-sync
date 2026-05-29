#include "game/controller/screens/PauseScreen.h"
#include "game/world/World.h"
#include "game/state/GameHelpPages.h"  // kGameHelpPageCount
#include "engine/input/Input.h"
#include "engine/input/Key.h"

namespace nccu {

bool HandlePauseMenu(World& world) {
    using nccu::gfx::Input;
    using nccu::gfx::Key;
    const bool toggle = Input::IsPressed(Key::M);
    if (world.MenuOpen()) {
        // REQUIREMENT #9: the 說明 help overlay sits ON TOP of the
        // paused menu. While it is up, M / E / Enter dismisses it
        // back to the menu and the menu cursor / sim stay frozen.
        // (ESC is not a dismiss key — it quits the program.) Handled
        // FIRST so a key meant for "close help" never also moves the
        // menu cursor or triggers an AppAction.
        if (world.HelpOpen()) {
            // U2-T4: the 說明 overlay is paged — ←/→ flip between the
            // 操作+目標 page and the 雨傘外觀+道具須知+結局 page (the
            // page index wraps; the View draws a 「第 N／M 頁」 indicator).
            // Pure UI state (World::HelpPage, NOT serialized — see
            // Harness.cpp), so a paged help leaves state.jsonl byte-
            // identical. M / E / Enter still dismisses back to the menu.
            constexpr int n = nccu::kGameHelpPageCount;
            if (Input::IsPressed(Key::Right))
                world.SetHelpPage((world.HelpPage() + 1) % n);
            if (Input::IsPressed(Key::Left))
                world.SetHelpPage((world.HelpPage() - 1 + n) % n);
            if (Input::IsPressed(Key::M) ||
                Input::IsPressed(Key::Enter) ||
                Input::IsPressed(Key::E))
                world.SetHelpOpen(false);
            return true;                    // frozen behind help
        }
        if (Input::IsPressed(Key::Up))   world.MoveMenuCursor(-1);
        if (Input::IsPressed(Key::Down)) world.MoveMenuCursor(1);
        if (toggle) {                       // M = quick Resume
            world.SetMenuOpen(false);
            return true;
        }
        if (Input::IsPressed(Key::Enter)) {
            switch (world.MenuCursor()) {
                case 0:                     // 繼續 (Resume)
                    world.SetMenuOpen(false);
                    break;
                case 1:                     // 說明 (Help) — overlay
                    world.SetHelpOpen(true);
                    break;
                case 2:                     // 減少動畫 (toggle)
                    // Cycle 9.E.3: pause-menu UI for the
                    // ReducedMotion accessibility flag added in
                    // 9.E.1. Flip in place; the menu stays open so
                    // the player can see the [開]/[關] state update
                    // on the same row their cursor is on. Pure
                    // World mutation — no AppAction, no menu close.
                    world.SetReducedMotion(!world.ReducedMotion());
                    break;
                case 3:                     // 擴大目標 (toggle)
                    // Cycle 9.E.3: pause-menu UI for the
                    // LargeTargets accessibility flag added in
                    // 9.E.2. Same in-place toggle shape as row 2 —
                    // the next gameplay frame's E-probe reach picks
                    // up the new value via World::LargeTargets().
                    world.SetLargeTargets(!world.LargeTargets());
                    break;
                case 4:                     // 重新開始 (Restart)
                    world.RequestAppAction(World::AppAction::Restart);
                    break;
                default:                    // 離開 (Quit)
                    world.RequestAppAction(World::AppAction::Quit);
                    break;
            }
        }
        return true;   // frozen while the menu is up
    }
    if (toggle) {                           // open from gameplay
        world.SetMenuOpen(true);
        return true;
    }
    return false;   // no menu involved this frame — fall through
}

} // namespace nccu
