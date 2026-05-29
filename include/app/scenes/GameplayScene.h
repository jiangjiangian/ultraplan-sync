#ifndef APP_SCENES_GAMEPLAY_SCENE_H_
#define APP_SCENES_GAMEPLAY_SCENE_H_
#include "app/IScene.h"
#include "game/controller/GameController.h"
#include "engine/audio/AudioDevice.h"
#include "engine/audio/AudioManager.h"
#include "ui/CharacterSelect.h"          // PersonaSelection
#include "ui/View.h"
#include "game/world/World.h"
#include <functional>
#include <memory>
#include <string>

namespace nccu { class Harness; }

namespace nccu::app {

// Blueprint Phase 3 step 1 — encapsulates the per-run gameplay scope
// that used to live inline in main.cpp (the World/View/GameController/
// AudioManager block + the while(!ShouldClose) loop body). Relocated
// verbatim so the harness-observable {state.jsonl, screenshots, event
// stream} is byte-identical to the pre-relocation path.
//
// Ownership / lifetime:
//   - World/View/GameController/AudioManager are scene-owned and torn
//     down in reverse-declaration order at scene destruction (same
//     RAII chain the inline block had).
//   - Harness is BORROWED via reference — its lifetime spans the
//     program, not the scene, so WireEvents() (now inside Enter())
//     can safely register subscribers and the BeginFrame/EndFrame
//     hooks in SceneManager::Run keep the per-frame harness seam
//     intact.
//   - AudioDevice is also BORROWED — owned by main.cpp so the GL
//     teardown order survives a scene transition (R3 invariant).
//
// Restart vs Quit semantics (matches today's main.cpp):
//   World::PendingAppAction() == Restart -> Update returns
//     SceneCommand{Restart} (main.cpp rebuilds the scene stack).
//   World::PendingAppAction() == Quit    -> Update returns
//     SceneCommand{Quit}    (main.cpp ends the program).
//   Harness::ShouldQuit() or Window-close also resolve to Quit at
//     the SceneManager::Run level (no per-scene check needed —
//     Run loop owns the Window/Harness exits).
class GameplayScene final : public IScene {
public:
    // restartFactory: produced by the composition root to build the
    // scene that should take over when the player picks 重新開始 from
    // the in-game menu — typically a fresh LoadingScene. Empty (or
    // nullptr-returning) closure means "no restart path available";
    // GameplayScene then routes Restart to Quit, which matches the
    // harness's never-restart contract today.
    using RestartFactory = std::function<std::unique_ptr<IScene>()>;

    GameplayScene(nccu::CharacterSelectResult selection,
                  nccu::audio::AudioDevice& audioDevice,
                  nccu::Harness& harness,
                  int windowWidth,
                  int windowHeight,
                  RestartFactory restartFactory = {});

    void Enter() override;
    [[nodiscard]] SceneCommand Update(float dt) override;
    void Draw(nccu::engine::render::IRenderer& renderer) override;
    void Exit() override;
    [[nodiscard]] const nccu::World*
    WorldForHarnessOrNull() const noexcept override { return &world_; }

private:
    // Per-run state, declaration order MATTERS — reverse-destruction
    // runs the controller dtor (EventBus::Clear) BEFORE the World it
    // captured into subscribers dies. AudioManager declared AFTER the
    // controller so reverse-destruction tears down audio FIRST (R3 +
    // the H1/B2 EventBus discipline).
    nccu::World          world_;
    nccu::View           view_;
    nccu::GameController controller_;
    nccu::audio::AudioManager audioManager_;
    nccu::Harness&       harness_;          // borrowed; lives in main.cpp
    RestartFactory       restartFactory_;    // Phase 3 step 4: rebuild path
};

} // namespace nccu::app

#endif // APP_SCENES_GAMEPLAY_SCENE_H_
