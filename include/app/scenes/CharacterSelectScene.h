#ifndef APP_SCENES_CHARACTER_SELECT_SCENE_H_
#define APP_SCENES_CHARACTER_SELECT_SCENE_H_
#include "app/IScene.h"
#include "engine/render/Texture.h"
#include "ui/CharacterSelect.h"          // CharacterSelectResult + kPersonas
#include "ui/PressLatch.h"
#include <functional>
#include <memory>
#include <vector>

namespace nccu::app {

// Blueprint Phase 3 step 2 — the persona picker, ported from the
// blocking RunCharacterSelect free function (src/ui/CharacterSelect.cpp)
// into the IScene contract. Per-frame Update reads ← →/A D + Enter
// to advance the cursor / confirm; Draw paints the five tiles + the
// selection panel. On confirmed selection, the scene emits
// SceneCommand{Replace, gameplayFactory_(selection)} so the next
// scene (today: GameplayScene) materialises with the resolved
// CharacterSelectResult.
//
// Like TitleScene's startGame factory, the gameplay factory is a
// closure produced by the composition root: it captures the
// audioDevice/harness/window refs GameplayScene needs without
// flowing them through this scene. Tests can hand a stub.
class CharacterSelectScene final : public IScene {
public:
    using GameplayFactory = std::function<
        std::unique_ptr<IScene>(nccu::CharacterSelectResult)>;

    explicit CharacterSelectScene(GameplayFactory gameplay);

    void Enter() override;
    [[nodiscard]] SceneCommand Update(float dt) override;
    void Draw(nccu::gfx::IRenderer& renderer) override;

private:
    GameplayFactory             gameplay_;
    int                         cursor_ = 0;
    std::vector<nccu::gfx::Texture> previews_;
    nccu::PressLatch            confirm_;          // gate held-Enter from TitleScene
};

} // namespace nccu::app

#endif // APP_SCENES_CHARACTER_SELECT_SCENE_H_
