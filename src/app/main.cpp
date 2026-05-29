// Phase 3 step 2: main.cpp pushes TitleScene (human) or GameplayScene
// (harness) via SceneManager. The blocking RunTitleScreen /
// RunCharacterSelect free functions are no longer called — replaced
// by TitleScene + CharacterSelectScene chained through
// SceneCommand::Replace.
#include "app/SceneManager.h"
#include "app/scenes/GameplayScene.h"
#include "app/scenes/TitleScene.h"
#include "app/scenes/CharacterSelectScene.h"
#include "app/scenes/LoadingScene.h"
#include "ui/CharacterSelect.h"
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

    // Phase 3 step 3: LoadingScene replaces the pre-Phase-3
    // RunLoadingScreen blocking call. It is now the FIRST scene the
    // SceneManager pushes on the human path (its Enter() runs the
    // PreloadGameTextures warm, then a kHoldFrames hold makes the
    // 載入中… affordance perceptible) before chaining to TitleScene
    // via SceneCommand::Replace. The harness path skips it exactly as
    // the pre-Phase-3 main.cpp did — only GameplayScene gets pushed.

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
        // Phase 3 step 2: SceneManager now owns BOTH the human path
        // (Title -> CharacterSelect -> Gameplay) AND the harness skip
        // path (push GameplayScene directly). The blocking
        // RunTitleScreen / RunCharacterSelect free functions are
        // retired in favour of TitleScene + CharacterSelectScene; the
        // chain between them runs on SceneCommand::Replace with a
        // factory closure produced here in the composition root, so
        // the next-scene ctor refs (audioDevice / harness / window
        // size) flow through the closure capture and don't pollute
        // earlier scenes' surfaces.
        nccu::app::SceneManager sm;
        nccu::gfx::RaylibRenderer renderer;
        if (harness.Active()) {
            // Deterministic bypass — identical to the pre-Phase-3
            // path: fixed sprite, default (white) tint, no screens.
            // GameplayScene constructed with the harness-resolved
            // selection.
            nccu::CharacterSelectResult harnessSel;
            harnessSel.spritePath = harness.SpritePath();
            sm.Push(std::make_unique<nccu::app::GameplayScene>(
                harnessSel, audioDevice, harness, kWinW, kWinH));
        } else {
            // Human path: TitleScene -> CharacterSelectScene ->
            // GameplayScene, every transition deferred via the
            // factory closures captured here.
            auto gameplayFactory =
                [&](nccu::CharacterSelectResult sel)
                -> std::unique_ptr<nccu::app::IScene> {
                return std::make_unique<nccu::app::GameplayScene>(
                    std::move(sel), audioDevice, harness,
                    kWinW, kWinH);
            };
            auto charSelectFactory =
                [gameplayFactory]()
                -> std::unique_ptr<nccu::app::IScene> {
                return std::make_unique<
                    nccu::app::CharacterSelectScene>(
                    gameplayFactory);
            };
            auto titleFactory =
                [charSelectFactory]()
                -> std::unique_ptr<nccu::app::IScene> {
                return std::make_unique<nccu::app::TitleScene>(
                    charSelectFactory);
            };
            // Phase 3 step 3: LoadingScene -> TitleScene chain.
            // Loading runs ONCE per Restart cycle (warm is idempotent
            // — the texture cache no-ops on a second call), so the
            // Restart-from-gameplay path gets the same "載入中…"
            // affordance the first run got. Pre-Phase-3 main.cpp ran
            // RunLoadingScreen exactly once before the outer loop;
            // running it per cycle is a minor UX change that the
            // blueprint phase-3 spec accepts (every Replace chain
            // is logically a fresh load — the visible affordance is
            // a feature, not a regression).
            sm.Push(std::make_unique<nccu::app::LoadingScene>(
                titleFactory));
        }
        const auto outcome = sm.Run(win, renderer, harness);
        if (outcome == nccu::app::SceneManager::RunOutcome::Quit)
            running = false;
        // outcome == Restart: outer loop iterates → new SceneManager
        // + initial scene next round. The harness path never
        // restarts; the human Restart loops back to TitleScene.
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
