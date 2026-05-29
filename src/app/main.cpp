// Phase 3 step 1: main.cpp shrinks to compose + push the initial
// scene. The per-run gameplay loop body moved into GameplayScene;
// SceneManager owns the one app loop.
#include "app/SceneManager.h"
#include "app/scenes/GameplayScene.h"
#include "ui/TitleScreen.h"
#include "ui/CharacterSelect.h"
#include "ui/LoadingScreen.h"
#include "engine/platform/Harness.h"
#include "engine/render/Window.h"
#include "engine/render/Font.h"
#include "engine/render/RaylibRenderer.h"
#include "engine/render/Texture.h"
#include "engine/audio/AudioDevice.h"
#include "engine/events/EventBus.h"
#include "engine/events/EventSink.h"  // Plan P2 step 3: entity publish seam
#include <memory>

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

    // Audio device — one-per-process RAII handle. Today no-op (project
    // ships no audio assets yet), but the scaffold lives in main()'s
    // teardown order alongside Window/Font so that when the first sfx
    // arrives, InitAudioDevice/CloseAudioDevice slot into a slot the
    // composition root already accommodates. See include/engine/audio/.
    nccu::audio::AudioDevice audioDevice;

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

        // Phase 3 step 1: the per-run scope (World/View/GameController/
        // AudioManager + the gameplay while-loop) now lives entirely
        // inside GameplayScene. SceneManager owns the scene stack +
        // runs the one app loop (BeginFrame → Update → Draw →
        // EndFrame → deferred-command apply), mirroring main.cpp's
        // pre-Phase-3 shape one-for-one so the harness-observable
        // {state.jsonl, screenshots, event stream} stays byte-identical.
        //
        // RAII order: SceneManager dtor runs the scene's dtor (which
        // runs the controller → AudioManager → View → World chain in
        // reverse-declaration order) BEFORE the surrounding renderer /
        // window / GL resources die. The unique_ptr-managed scene gets
        // teardown for free, exactly as the inline block did.
        {
            nccu::app::SceneManager sm;
            nccu::gfx::RaylibRenderer renderer;
            sm.Push(std::make_unique<nccu::app::GameplayScene>(
                selection, audioDevice, harness, kWinW, kWinH));
            const auto outcome = sm.Run(win, renderer, harness);
            if (outcome == nccu::app::SceneManager::RunOutcome::Quit)
                running = false;
            // outcome == Restart: outer loop iterates → new SceneManager
            // + GameplayScene next round (after Title/Select rerun for
            // the human path; harness never restarts so the harness
            // branch never returns Restart).
        }
        // Plan P2 step 3: drop the entity-layer publish seam BEFORE the
        // next iteration rebuilds World/Controller (which would re-SetSink
        // anyway). Resetting to nullptr makes the seam fall through to
        // EventBus::Instance() for any code that runs between iterations
        // (asset cleanup, the title screen on the next loop) — byte-
        // identical to the pre-P2 path. Matches the existing
        // EventBus::Instance().Clear() lifecycle the controller dtor owns.
        nccu::events::SetSink(nullptr);

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
