#include "world/World.h"
#include "ui/View.h"
#include "controller/GameController.h"
#include "ui/TitleScreen.h"
#include "ui/CharacterSelect.h"
#include "harness/Harness.h"
#include "gfx/Window.h"
#include "gfx/DrawScope.h"
#include "gfx/Font.h"

// MVC composition root. Model = World (pure data), View = rendering,
// Controller = input + simulation + event wiring. The inner game loop is
// the only logic here: tick the controller, then draw the model through
// the view. Everything else is one-time setup / screen flow / teardown.
//
// Screen flow (human play): Title → Character-Select → Playing. The
// in-game menu's 重新開始 (Restart) routes back to the Title with a
// FULLY fresh World/View/GameController (so karma/money/flags do not
// leak across runs and no EventBus subscriber dangles — the controller
// dtor Clears the bus before the next World is built; BUGLEDGER B2/H1).
//
// HARNESS: when UMBRELLA_SCRIPT is set, BOTH the title and the
// character-select are skipped exactly as the old single character-
// select was — a deterministic sprite from UMBRELLA_SPRITE, straight
// into one Playing run — so every .claude/scripts/* timeline still
// produces a byte-identical state.jsonl. Restart is human-only (the
// scripted input never opens the pause menu).
int main() {
    constexpr int kWinW = 800;
    constexpr int kWinH = 450;

    auto win = nccu::gfx::Window::Builder()
                   .Title("Lost Umbrella - MVP")
                   .Size(kWinW, kWinH)
                   .Fps(60)
                   .Open();

    // raylib's default font is ASCII-only; load the CJK font now that the
    // GL context exists, before any text (title / select / HUD) draws.
    nccu::gfx::EnsureFont();

    // Off unless UMBRELLA_SCRIPT is set; then it drives input headlessly
    // and skips the interactive title + character-select for a
    // deterministic run.
    auto harness = nccu::MaybeAttach();

    // Outer screen-flow loop. One iteration == one prepared run. The
    // human path may revisit the title (back from select, or Restart
    // from the in-game menu); the harness path runs exactly once.
    //
    // ESC is INERT everywhere (player request): Window::Open calls
    // SetExitKey(KEY_NULL), so WindowShouldClose() only fires on the window
    // close button, never on ESC. Quitting is the 離開 menu item — on the
    // title menu and the in-game pause menu (M). The title/select/help
    // screens no longer consume ESC at all.
    bool running = true;
    while (running && !win.ShouldClose()) {
        nccu::CharacterSelectResult selection;

        if (harness.Active()) {
            // Deterministic bypass — identical to the pre-feature
            // behaviour: fixed sprite, default (white) tint, no screens.
            selection.spritePath = harness.SpritePath();
        } else {
            // Title screen first.
            if (nccu::RunTitleScreen(win) == nccu::TitleChoice::Quit)
                break;                       // 離開 / window closed
            if (win.ShouldClose()) break;

            // Character select. ESC is inert now; the player picks a
            // persona with ← → + Enter. (selection.closed stays false.)
            selection = nccu::RunCharacterSelect(win);
            if (win.ShouldClose()) break;
            if (selection.closed) continue;  // (defensive; unused now)
        }

        // Per-run scope. Declaration order matters: reverse-destruction
        // runs the controller dtor (EventBus::Clear) BEFORE the World
        // refs its subscribers captured die. Do not reorder these three.
        // On scope exit (Restart) all three are destroyed while `win` /
        // GL is still alive, then the next iteration builds a brand-new
        // set — a clean reset with no dangling EventBus handlers.
        {
            nccu::World          world{selection.spritePath};
            nccu::View           view{kWinW, kWinH};
            nccu::GameController  controller{world};

            if (Player* p = world.GetPlayer())
                p->SetTint(selection.tint);  // persona colour modulate

            harness.WireEvents();   // after controller wiring: safe teardown

            bool restart = false;
            while (!win.ShouldClose() && !harness.ShouldQuit()) {
                harness.BeginFrame();
                controller.Update();
                {
                    nccu::gfx::DrawScope frame;
                    view.Draw(world);
                }
                harness.EndFrame(world);

                // In-game menu intent (human only — the scripted input
                // never opens the pause menu, so the harness path is
                // unaffected and deterministic).
                const auto act = world.PendingAppAction();
                if (act == nccu::World::AppAction::Restart) {
                    restart = true;
                    break;                   // tear down → back to title
                }
                if (act == nccu::World::AppAction::Quit) {
                    running = false;
                    break;
                }
            }
            if (!restart) running = false;   // normal end / quit / window
        }   // world/view/controller dtors (RAII, GL still live)

        if (harness.Active()) break;         // harness runs exactly once
    }

    // Unload the font BEFORE the Window dtor runs ::CloseWindow(): a
    // static-lifetime font would otherwise destruct after the GL context
    // is gone. `win` is still alive on this line, so GL is valid.
    nccu::gfx::ShutdownFont();
    return 0;
}
