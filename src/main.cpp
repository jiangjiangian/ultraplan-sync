#include "world/World.h"
#include "ui/View.h"
#include "controller/GameController.h"
#include "ui/TitleScreen.h"
#include "ui/CharacterSelect.h"
#include "ui/LoadingScreen.h"
#include "engine/platform/Harness.h"
#include "engine/render/Window.h"
#include "engine/render/DrawScope.h"
#include "engine/render/Font.h"
#include "engine/render/Texture.h"
#include <cstdlib>
#include <cstring>

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

    // Human-only 載入畫面: warm the texture cache (gfx/Texture.h) behind a
    // brief "載入中…" screen so the first World/View construct — and every
    // Restart — hits the warm cache with no first-frame disk/GPU stutter.
    // Skipped under the harness (like the title/select below) so the
    // scripted frame timeline / state.jsonl stay byte-identical; the harness
    // path lets World/View warm the cache lazily off the recorded loop.
    if (!harness.Active())
        nccu::RunLoadingScreen(win);

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
            // Harness-only debug warp: UMBRELLA_START_STATE jumps the FSM at
            // startup so a screenshot can reach a later chapter without
            // scripting the whole spine. Off by default — guarded by
            // harness.Active() AND the env var, so normal play and the
            // classic timed harness are byte-for-byte unchanged.
            if (harness.Active()) {
                if (const char* s = std::getenv("UMBRELLA_START_STATE"); s) {
                    using S = nccu::SemesterState;
                    S target = world.Semester().Current();
                    bool warp = true;
                    if      (std::strcmp(s, "Chapter2")  == 0) target = S::Chapter2_Midterms;
                    else if (std::strcmp(s, "Chapter3")  == 0) target = S::Chapter3_SportsDay;
                    else if (std::strcmp(s, "Chapter4")  == 0) target = S::Chapter4_Finals;
                    else if (std::strcmp(s, "Interlude") == 0) target = S::Interlude_Market;
                    else warp = false;
                    if (warp) {                       // FSM + roster, so the
                        world.Semester().Transition(target);   // chapter's NPCs
                        world.RespawnChapterRoster(target);     // actually spawn
                    }
                }
            }
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

    // Unload the font AND the texture cache BEFORE the Window dtor runs
    // ::CloseWindow(): both hold GPU resources, and a static-lifetime store
    // would otherwise destruct after the GL context is gone (touching dead
    // GL → UB). `win` is still alive on this line, so GL is valid. Mirror of
    // the Font discipline; the cache owns each texture once, so this is the
    // single ::UnloadTexture point for the shared (cached) textures.
    nccu::gfx::ShutdownTextureCache();
    nccu::gfx::ShutdownFont();
    return 0;
}
