#ifndef APP_SCENES_LOADING_SCENE_H_
#define APP_SCENES_LOADING_SCENE_H_
#include "app/IScene.h"
#include <functional>
#include <memory>

namespace nccu::app {

/**
 * @brief 「載入中…」畫面，位於人類遊玩場景鏈的最前端。
 *
 * SceneManager 最先推入此場景；它在 Enter() 暖機紋理快取，並把標題停留可被察覺的
 * 數幀後，回傳 SceneCommand{Replace, nextFactory_()} 將控制權交給下一個場景
 * （TitleScene）。
 *
 * next 是 composition root 交付的工廠 closure（即 TitleScene 的工廠，內部又捕獲
 * gameplay／CharacterSelect 鏈）。如此 LoadingScene 無須得知 TitleScene 的對外
 * 形貌，測試也能換入任意 IScene。
 */
class LoadingScene final : public IScene {
public:
    using NextSceneFactory = std::function<std::unique_ptr<IScene>()>;

    explicit LoadingScene(NextSceneFactory next);

    /**
     * @brief 掛載時同步暖機紋理快取，使首個遊玩幀不致卡頓。
     */
    void Enter() override;

    [[nodiscard]] SceneCommand Update(float dt) override;
    void Draw(nccu::engine::render::IRenderer& renderer) override;

private:
    NextSceneFactory next_;
    int              frameCount_ = 0;
};

} // namespace nccu::app

#endif // APP_SCENES_LOADING_SCENE_H_
