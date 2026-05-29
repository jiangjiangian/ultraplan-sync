// Phase 3 step 4: main.cpp shrinks to composition + the single initial
// push. SceneManager owns the entire screen flow (Loading -> Title ->
// CharacterSelect -> Gameplay), and Restart from gameplay routes
// through the restartFactory_ closure GameplayScene captures here —
// no outer screen-flow loop in main. The harness path pushes
// GameplayScene directly with an empty restartFactory so a stray
// Restart resolves to Quit (matches the never-restart contract).
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
#include <functional>
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

    // ESC is INERT everywhere (player request): Window::Open calls
    // SetExitKey(KEY_NULL), so WindowShouldClose() only fires on the
    // window close button, never on ESC. Quitting is the 離開 menu
    // item — on the title menu and the in-game pause menu (M).
    nccu::app::SceneManager sm;
    nccu::gfx::RaylibRenderer renderer;

    if (harness.Active()) {
        // Harness skip path: deterministic, runs ONCE. GameplayScene
        // gets an empty restartFactory so a stray Restart resolves
        // to Quit (the never-restart contract).
        nccu::CharacterSelectResult harnessSel;
        harnessSel.spritePath = harness.SpritePath();
        sm.Push(std::make_unique<nccu::app::GameplayScene>(
            harnessSel, audioDevice, harness, kWinW, kWinH));
    } else {
        // Human path: a four-link factory chain. Each scene's
        // ctor closure captures only the next link AND the
        // composition-root references it needs — audioDevice /
        // harness / window size flow through to GameplayScene
        // without polluting Title or Select. The initial scene is
        // LoadingScene; Gameplay's Restart routes back to a fresh
        // LoadingScene via initialSceneFactory (forward-declared so
        // the closure capture is valid by the time Restart fires).
        std::function<std::unique_ptr<nccu::app::IScene>()>
            initialSceneFactory;
        auto gameplayFactory =
            [&audioDevice, &harness, &initialSceneFactory, kWinW, kWinH]
            (nccu::CharacterSelectResult sel)
            -> std::unique_ptr<nccu::app::IScene> {
            return std::make_unique<nccu::app::GameplayScene>(
                std::move(sel), audioDevice, harness,
                kWinW, kWinH, initialSceneFactory);
        };
        auto charSelectFactory =
            [gameplayFactory]()
            -> std::unique_ptr<nccu::app::IScene> {
            return std::make_unique<nccu::app::CharacterSelectScene>(
                gameplayFactory);
        };
        auto titleFactory =
            [charSelectFactory]()
            -> std::unique_ptr<nccu::app::IScene> {
            return std::make_unique<nccu::app::TitleScene>(
                charSelectFactory);
        };
        initialSceneFactory =
            [titleFactory]()
            -> std::unique_ptr<nccu::app::IScene> {
            return std::make_unique<nccu::app::LoadingScene>(
                titleFactory);
        };
        sm.Push(initialSceneFactory());
    }
    // The single Run call drives the entire screen flow. Restart from
    // gameplay re-enters via the captured initialSceneFactory; Quit /
    // Window-close / harness ShouldQuit funnel into RunOutcome::Quit
    // and Run returns.
    (void)sm.Run(win, renderer, harness);
    // Plan P2 step 3: drop the entity-layer publish seam before the
    // GL teardown below. Matches the existing EventBus::Instance().
    // Clear() lifecycle the controller dtor owns.
    nccu::events::SetSink(nullptr);

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
