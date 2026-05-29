// MVC composition root. Model = World (pure data), View = rendering,
// Controller = input + simulation + event wiring. main() stays thin: open
// the window, load shared resources, then hand the entire screen flow to
// SceneManager. The harness-vs-human branching and the Loading → Title →
// CharacterSelect → Gameplay scene chain live in SceneBootstrap, so this
// file is just composition + the run call + ordered teardown.
//
// Screen flow (human play): Loading → Title → Character-Select → Playing.
// The in-game menu's 重新開始 (Restart) rebuilds a fully fresh
// World/View/GameController scene so no state leaks across runs and no
// EventBus subscriber dangles (the controller dtor Clears the bus before
// the next World is built).
//
// HARNESS: when UMBRELLA_SCRIPT is set, the title + character-select are
// skipped — a deterministic sprite straight into one Playing run — so a
// scripted timeline produces a byte-identical state.jsonl. Restart is
// human-only (the scripted input never opens the pause menu).
#include "app/SceneManager.h"
#include "app/SceneBootstrap.h"
#include "engine/platform/Harness.h"
#include "engine/platform/WorkingDir.h"
#include "engine/render/Window.h"
#include "engine/render/Font.h"
#include "engine/render/RaylibRenderer.h"
#include "engine/render/Texture.h"
#include "engine/audio/AudioDevice.h"
#include "engine/events/EventSink.h"  // entity publish seam: SetSink(nullptr) on teardown

int main() {
    constexpr int kWinW = 800;
    constexpr int kWinH = 450;

    auto win = nccu::engine::render::Window::Builder()
                   .Title("Lost Umbrella - MVP")
                   .Size(kWinW, kWinH)
                   .Fps(60)
                   .Open();

    // Normalise the working directory FIRST (before any asset load) so the
    // game's relative resources/ + docs/content/ paths resolve no matter how
    // it was launched (Finder / IDE / run-from-build). This keeps EnsureFont
    // on its safe content-present atlas path instead of the oversized
    // fallback that crashed on some GPUs. No-op when already run from a
    // directory that has the assets (project root / ctest / harness).
    nccu::engine::platform::EnsureAssetWorkingDir();

    // raylib's default font is ASCII-only; load the CJK font now that the
    // GL context exists, before any text (title / select / HUD) draws.
    nccu::engine::render::EnsureFont();

    // Audio device — one-per-process RAII handle, declared here so its
    // teardown order sits alongside Window/Font in the composition root.
    nccu::audio::AudioDevice audioDevice;

    // Off unless UMBRELLA_SCRIPT is set; then it drives input headlessly and
    // skips the interactive title + character-select for a deterministic run.
    auto harness = nccu::MaybeAttach();

    // ESC is INERT everywhere (Window::Open calls SetExitKey(KEY_NULL)), so
    // WindowShouldClose() only fires on the window close button. Quitting is
    // the 離開 menu item (title menu / in-game pause menu).
    nccu::app::SceneManager sm;
    nccu::engine::render::RaylibRenderer renderer;

    // Assemble + push the initial scene (harness skip path or the human
    // Loading→Title→Select→Gameplay chain). All screen-flow wiring is in
    // SceneBootstrap so main() stays a thin composition root.
    nccu::app::PushInitialScene(sm, harness, audioDevice, kWinW, kWinH);

    // The single Run call drives the entire screen flow. Restart re-enters a
    // fresh chain; Quit / window-close / harness ShouldQuit end the program.
    (void)sm.Run(win, renderer, harness);

    // Drop the entity-layer publish seam before the GL teardown below.
    nccu::events::SetSink(nullptr);

    // Unload the font AND the texture cache BEFORE the Window dtor runs
    // ::CloseWindow(): both hold GPU resources, and a static-lifetime store
    // would otherwise destruct after the GL context is gone (touching dead
    // GL → UB). `win` is still alive on this line, so GL is valid. The cache
    // owns each texture once, so this is the single ::UnloadTexture point for
    // the shared (cached) textures.
    nccu::engine::render::ShutdownTextureCache();
    nccu::engine::render::ShutdownFont();
    return 0;
}
