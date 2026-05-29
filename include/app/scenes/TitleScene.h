#ifndef APP_SCENES_TITLE_SCENE_H_
#define APP_SCENES_TITLE_SCENE_H_
#include "app/IScene.h"
#include "ui/PressLatch.h"
#include <functional>
#include <memory>

namespace nccu::app {

/**
 * @brief 標題畫面：開始遊戲／遊戲說明／離開 三項主選單。
 *
 * 每幀 Update 讀輸入並移動游標；Draw 視 showingHelp_ 旗標畫出選單，或畫出標題內部
 * 的「遊戲說明」頁。「遊戲說明」刻意維持為標題內部的子頁面（不跨出場景邊界），唯有
 * 開始遊戲／離開 兩個選項才會抵達場景切換。
 *
 * startGame 是 composition root（main.cpp）產生的工廠 closure：確認「開始遊戲」時
 * 場景發出 SceneCommand{Replace, startGame_}，使下一個場景在「Draw 之後」的延後
 * 套用點才實體化。目前鏈接到產生 CharacterSelectScene；測試可換入回傳任意 IScene
 * 的 stub。
 */
class TitleScene final : public IScene {
public:
    /**
     * @brief 確認「開始遊戲」後產生下一個場景的工廠 thunk。
     *
     * 由 closure 負責建構下一個場景，使 TitleScene 無須得知 CharacterSelectScene
     * 的建構介面（GameplayScene 所需的 audioDevice／harness／window 參考也就不必
     * 流經 Title），讓 Title 保持可測試。
     */
    using NextSceneFactory = std::function<std::unique_ptr<IScene>()>;

    explicit TitleScene(NextSceneFactory startGame);

    [[nodiscard]] SceneCommand Update(float dt) override;
    void Draw(nccu::engine::render::IRenderer& renderer) override;

private:
    NextSceneFactory startGame_;
    int              cursor_ = 0;       ///< 目前游標（0..kCount-1）
    bool             showingHelp_ = false;  ///< 是否正顯示「遊戲說明」內部頁
    int              helpPage_ = 0;     ///< 分頁式「遊戲說明」的目前頁碼
    nccu::PressLatch confirm_;          ///< 攔下從前一畫面延續的 Enter 長按
    nccu::PressLatch helpBack_;         ///< 攔下從選單進入說明頁時延續的 Enter 長按
};

} // namespace nccu::app

#endif // APP_SCENES_TITLE_SCENE_H_
