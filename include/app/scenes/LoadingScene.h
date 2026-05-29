#ifndef APP_SCENES_LOADING_SCENE_H_
#define APP_SCENES_LOADING_SCENE_H_
#include "app/IScene.h"
#include <functional>
#include <memory>

namespace nccu::app {

// Blueprint Phase 3 step 3 — UI-C-2 載入中… screen, ported from the
// blocking RunLoadingScreen free function (see src/ui/LoadingScreen.cpp)
// into the IScene contract. Sits at the front of the human path's
// scene chain: SceneManager pushes this first, it warms the texture
// cache in Enter() and holds the label for a perceptible number of
// frames, then returns SceneCommand{Replace, nextFactory_()} so the
// next scene (TitleScene) takes over.
//
// `nextFactory` is the factory closure the composition root hands
// over — TitleScene's factory, capturing the gameplay/CharacterSelect
// chain. Keeps LoadingScene unaware of TitleScene's surface, so a
// test can swap any IScene in.
class LoadingScene final : public IScene {
public:
    using NextSceneFactory = std::function<std::unique_ptr<IScene>()>;

    explicit LoadingScene(NextSceneFactory next);

    // Enter() runs the synchronous PreloadGameTextures warm so the
    // first gameplay frame doesn't stutter. The pre-Phase-3 free
    // function did the same warm, just inside its own loop.
    void Enter() override;

    [[nodiscard]] SceneCommand Update(float dt) override;
    void Draw(nccu::gfx::IRenderer& renderer) override;

private:
    NextSceneFactory next_;
    int              frameCount_ = 0;
};

} // namespace nccu::app

#endif // APP_SCENES_LOADING_SCENE_H_
