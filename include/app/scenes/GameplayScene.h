#ifndef APP_SCENES_GAMEPLAY_SCENE_H_
#define APP_SCENES_GAMEPLAY_SCENE_H_
#include "app/IScene.h"
#include "game/controller/GameController.h"
#include "engine/audio/AudioDevice.h"
#include "engine/audio/AudioManager.h"
#include "ui/CharacterSelect.h"          // 引入角色選擇結果型別
#include "ui/View.h"
#include "game/world/World.h"
#include <functional>
#include <memory>
#include <string>

namespace nccu { class Harness; }

namespace nccu::app {

/**
 * @brief 遊玩場景：封裝單次遊玩所需的 World／View／GameController／AudioManager 範圍
 *        及其每幀推進。
 *
 * 所有權與生命週期：
 *   - World／View／GameController／AudioManager 由場景擁有，於場景解構時依「宣告順序
 *     的逆序」拆除（成員宣告順序即為 RAII 鏈，順序具語意，見下方私有區）。
 *   - Harness 以參考方式借用——其壽命橫跨整個程式而非單一場景，故 WireEvents()
 *     （現位於 Enter()）可安全註冊訂閱者，SceneManager::Run 中的 BeginFrame／EndFrame
 *     鉤子也得以維持每幀的錄製接縫完整。
 *   - AudioDevice 同樣為借用——由 main.cpp 擁有，使 GL 拆除順序能跨越場景切換仍然
 *     正確（音訊裝置須晚於場景銷毀）。
 *
 * Restart 與 Quit 語意：
 *   World::PendingAppAction() == Restart -> Update 回傳 SceneCommand{Restart}
 *     （main.cpp 重建場景堆疊）。
 *   World::PendingAppAction() == Quit    -> Update 回傳 SceneCommand{Quit}
 *     （main.cpp 結束程式）。
 *   Harness::ShouldQuit() 或視窗關閉亦在 SceneManager::Run 層級收斂為 Quit（無須
 *     逐場景檢查——Run 迴圈本身掌管視窗／錄製器的離開）。
 */
class GameplayScene final : public IScene {
public:
    /**
     * @brief 玩家於遊戲內選單選擇「重新開始」時，用以建立接手場景的工廠。
     *
     * 由 composition root 產生，通常建立一個全新的 LoadingScene。留空（或回傳
     * nullptr）的 closure 表示「無重啟路徑」；此時 GameplayScene 會將 Restart 導向
     * Quit，恰好對應自動錄製的「永不重啟」契約。
     */
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
    // 單次遊玩狀態；宣告順序具語意——逆序解構會在「被訂閱者捕獲的 World」死亡之前，
    // 先跑 controller 的解構（EventBus::Clear）。AudioManager 宣告於 controller
    // 之後，使逆序解構最先拆除音訊（維持 EventBus 訂閱者不致懸空的紀律）。
    nccu::World          world_;
    nccu::View           view_;
    nccu::GameController controller_;
    nccu::audio::AudioManager audioManager_;
    nccu::Harness&       harness_;          ///< 借用；實體存活於 main.cpp
    RestartFactory       restartFactory_;   ///< 重啟時用以重建場景鏈的工廠
};

} // namespace nccu::app

#endif // APP_SCENES_GAMEPLAY_SCENE_H_
