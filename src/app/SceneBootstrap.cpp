#include "app/SceneBootstrap.h"

#include "app/SceneManager.h"
#include "app/IScene.h"
#include "app/scenes/GameplayScene.h"
#include "app/scenes/TitleScene.h"
#include "app/scenes/CharacterSelectScene.h"
#include "app/scenes/LoadingScene.h"
#include "ui/CharacterSelect.h"          // CharacterSelectResult
#include "engine/platform/Harness.h"
#include "engine/audio/AudioDevice.h"

#include <functional>
#include <memory>

namespace nccu::app {
namespace {

// Build a fresh human screen chain: Loading -> Title -> CharacterSelect ->
// Gameplay. Each scene's ctor captures the next link by value, so the
// returned LoadingScene owns a self-contained copy of the whole chain.
//
// The in-game 重新開始 routes through GameplayScene's restartFactory, which
// here simply calls THIS function again — capturing only the borrowed
// audio/harness refs (program-lifetime, owned by main). That deliberately
// avoids the earlier forward-declared-std::function pattern: no dangling
// reference once this returns, and no shared_ptr reference cycle.
std::unique_ptr<IScene> MakeHumanInitialScene(
    nccu::audio::AudioDevice& audio, nccu::Harness& harness,
    int winW, int winH) {
    auto gameplayFactory =
        [&audio, &harness, winW, winH](nccu::CharacterSelectResult sel)
        -> std::unique_ptr<IScene> {
        return std::make_unique<GameplayScene>(
            std::move(sel), audio, harness, winW, winH,
            [&audio, &harness, winW, winH] {
                return MakeHumanInitialScene(audio, harness, winW, winH);
            });
    };
    auto charSelectFactory = [gameplayFactory]() -> std::unique_ptr<IScene> {
        return std::make_unique<CharacterSelectScene>(gameplayFactory);
    };
    auto titleFactory = [charSelectFactory]() -> std::unique_ptr<IScene> {
        return std::make_unique<TitleScene>(charSelectFactory);
    };
    return std::make_unique<LoadingScene>(titleFactory);
}

} // namespace

void PushInitialScene(SceneManager& sm, nccu::Harness& harness,
                      nccu::audio::AudioDevice& audio, int winW, int winH) {
    if (harness.Active()) {
        // Deterministic skip path: straight to one GameplayScene with an
        // empty restart factory (a stray Restart resolves to Quit).
        nccu::CharacterSelectResult harnessSel;
        harnessSel.spritePath = harness.SpritePath();
        sm.Push(std::make_unique<GameplayScene>(
            harnessSel, audio, harness, winW, winH));
        return;
    }
    sm.Push(MakeHumanInitialScene(audio, harness, winW, winH));
}

} // namespace nccu::app
