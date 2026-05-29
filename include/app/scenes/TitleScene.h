#ifndef APP_SCENES_TITLE_SCENE_H_
#define APP_SCENES_TITLE_SCENE_H_
#include "app/IScene.h"
#include "ui/PressLatch.h"
#include <functional>
#include <memory>

namespace nccu::app {

// Blueprint Phase 3 step 2 — the home screen 開始遊戲 / 遊戲說明 / 離開
// menu, ported from the blocking RunTitleScreen free function (see
// src/ui/TitleScreen.cpp) into the IScene contract. Per-frame Update
// reads input + advances cursor; Draw paints either the menu or the
// internal 遊戲說明 help page (showingHelp_ flag — Help stays a
// title-internal page like the pre-Phase-3 free function did,
// per blueprint "TitleChoice still only sees Start/Quit").
//
// `startGame` is a factory closure produced by the composition root
// (main.cpp): on confirmed 開始遊戲 the scene emits
// SceneCommand{Replace, startGame_} so the next scene materialises
// at the AFTER-Draw deferred-apply point. Today's chain wires it to
// produce a CharacterSelectScene; tests can hand a stub that returns
// any IScene.
class TitleScene final : public IScene {
public:
    // Factory thunk that produces the next scene once 開始遊戲 is
    // confirmed. Letting the closure construct the next scene keeps
    // TitleScene unaware of CharacterSelectScene's ctor surface
    // (the audioDevice/harness/window refs GameplayScene needs
    // never have to flow through Title), so Title stays testable.
    using NextSceneFactory = std::function<std::unique_ptr<IScene>()>;

    explicit TitleScene(NextSceneFactory startGame);

    [[nodiscard]] SceneCommand Update(float dt) override;
    void Draw(nccu::engine::render::IRenderer& renderer) override;

private:
    NextSceneFactory startGame_;
    int              cursor_ = 0;       // 0..kItemCount-1
    bool             showingHelp_ = false;
    int              helpPage_ = 0;     // U2-T4 paged 遊戲說明
    nccu::PressLatch confirm_;          // gate held-Enter from prior screen
    nccu::PressLatch helpBack_;         // gate held-Enter from menu->help
};

} // namespace nccu::app

#endif // APP_SCENES_TITLE_SCENE_H_
