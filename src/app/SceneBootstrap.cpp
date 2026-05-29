#include "app/SceneBootstrap.h"

#include "app/SceneManager.h"
#include "app/IScene.h"
#include "app/scenes/GameplayScene.h"
#include "app/scenes/TitleScene.h"
#include "app/scenes/CharacterSelectScene.h"
#include "app/scenes/LoadingScene.h"
#include "ui/CharacterSelect.h"          // 引入 CharacterSelectResult
#include "engine/platform/Harness.h"
#include "engine/audio/AudioDevice.h"

#include <functional>
#include <memory>

namespace nccu::app {
namespace {

// 建立一條全新的人類畫面鏈：Loading -> Title -> CharacterSelect -> Gameplay。
// 每個場景的建構子以值捕獲下一段，故回傳的 LoadingScene 自帶整條鏈的獨立副本。
//
// 遊戲內的「重新開始」會走 GameplayScene 的 restartFactory，而此處的 restartFactory
// 只是再次呼叫「本函式」——僅捕獲借用的 audio／harness 參考（程式級壽命、由 main
// 擁有）。如此刻意避開先前「前向宣告的 std::function」寫法：函式回傳後不留懸空參考，
// 也沒有 shared_ptr 的循環參考。
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
        // 具決定性的略過路徑：直接進入單一 GameplayScene，restart 工廠留空（誤觸的
        // Restart 會收斂為 Quit）。
        nccu::CharacterSelectResult harnessSel;
        harnessSel.spritePath = harness.SpritePath();
        sm.Push(std::make_unique<GameplayScene>(
            harnessSel, audio, harness, winW, winH));
        return;
    }
    sm.Push(MakeHumanInitialScene(audio, harness, winW, winH));
}

} // namespace nccu::app
