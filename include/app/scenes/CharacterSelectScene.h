#ifndef APP_SCENES_CHARACTER_SELECT_SCENE_H_
#define APP_SCENES_CHARACTER_SELECT_SCENE_H_
#include "app/IScene.h"
#include "engine/render/Texture.h"
#include "ui/CharacterSelect.h"          // 引入 CharacterSelectResult 與 kPersonas
#include "ui/PressLatch.h"
#include <functional>
#include <memory>
#include <vector>

namespace nccu::app {

/**
 * @brief 角色選擇畫面：在五位角色間挑選。
 *
 * 每幀 Update 讀 ← →／A D ＋ Enter 以移動游標／確認；Draw 畫出五格角色與選取面板。
 * 確認選擇後，場景發出 SceneCommand{Replace, gameplayFactory_(selection)}，使下一個
 * 場景（目前為 GameplayScene）帶著解析後的 CharacterSelectResult 實體化。
 *
 * 與 TitleScene 的 startGame 工廠一樣，gameplay 工廠也是 composition root 產生的
 * closure：它捕獲 GameplayScene 所需的 audioDevice／harness／window 參考，而不必讓
 * 這些參考流經本場景。測試可換入 stub。
 */
class CharacterSelectScene final : public IScene {
public:
    using GameplayFactory = std::function<
        std::unique_ptr<IScene>(nccu::CharacterSelectResult)>;

    explicit CharacterSelectScene(GameplayFactory gameplay);

    void Enter() override;
    [[nodiscard]] SceneCommand Update(float dt) override;
    void Draw(nccu::engine::render::IRenderer& renderer) override;

private:
    GameplayFactory             gameplay_;
    int                         cursor_ = 0;        ///< 目前選取的角色索引
    std::vector<nccu::engine::render::Texture> previews_;  ///< 各角色預覽紋理（隨場景 RAII 釋放）
    nccu::PressLatch            confirm_;          ///< 攔下從 TitleScene 延續的 Enter 長按
};

} // namespace nccu::app

#endif // APP_SCENES_CHARACTER_SELECT_SCENE_H_
