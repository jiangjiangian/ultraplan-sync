#ifndef GAME_CONTROLLER_H_
#define GAME_CONTROLLER_H_
#include "game/controller/InputHandler.h"
#include "game/controller/SceneRouter.h"
#include "game/controller/SimSystem.h"
#include "game/state/SemesterState.h"
#include "engine/math/Rect.h"
#include "engine/math/Vec2.h"
#include <memory>
#include <string>
#include <vector>

class Player;     // 全域命名空間的模型物件——前向宣告
class Vendor;     // 商店 NPC；跨對話幀的待購買目標——前向宣告
class EventBus;   // 由建構子注入並存為成員的事件匯流排——前向宣告

namespace nccu {

class World;
struct DialogChoice;

/**
 * @file GameController.h
 * @brief MVC 中的「輸入＋模擬」協調者（orchestrator）。
 */

/**
 * @brief 串起輸入層、ISystem 模擬管線與各觀察者，每幀推進一次世界的協調者。
 *
 * 一次 Update() 把世界推進一幀：tick 每個物件、解算玩家碰撞、派發 E 鍵互動、偵測
 * 建築進入、清除死亡物件。它擁有 EventBus 的接線——於建構子安裝、於解構子拆除，使
 * 任何綁定到 bus 的 lambda 都不會比它所捕捉的 World 參考更長壽。它只變動 World；
 * 絕不渲染、也不為 View 讀取輸入裝置。
 *
 * 設計意圖：輸入的邊緣計時已移入 InputHandler，FSM 轉場觀察者已移入 SceneRouter，
 * 模擬階段已拆成 ISystem。Controller 維持為把它們接到 World 的協調者——一個輔助
 * 類別負責一項具體職責。
 */
class GameController {
public:
    /**
     * @brief 以世界與事件匯流排建構協調者，並安裝預設訂閱者。
     * @param[in,out] world 要被本協調者推進的世界。
     * @param[in,out] bus   注入的事件匯流排（依賴注入）。
     *
     * bus 以注入方式傳入，使 Controller 可達的每個發布點（HandleDialog 的各 gate、
     * SceneRouter、預設訂閱者接線）都走「同一個」實例，測試也能替換成自己的匯流排。
     * 正式版 main.cpp 傳入單例；測試可傳入區域匯流排。
     */
    GameController(World& world, EventBus& bus);
    ~GameController();

    GameController(const GameController&)            = delete;
    GameController& operator=(const GameController&) = delete;

    /** @brief 推進世界一幀：先處理畫面輸入，再跑模擬管線與各 gate。 */
    void Update();

private:
    /// @name 各畫面的輸入子處理器
    /// 每個都讀取 Input，故留在 Controller 層（非 system）。各自在「吃掉本幀且世界
    /// 須維持凍結」時回傳 true（協調者隨即在模擬管線之前回傳），否則回傳 false 以
    /// 落到下一個畫面／模擬。
    ///@{
    bool HandleEndingMenu();   ///< 結局畫面的底部選單
    bool HandlePauseMenu();    ///< M 暫停選單＋說明覆蓋層
    bool HandleDialog();       ///< 對話推進＋攤主購買導向
    bool HandleInventory();    ///< Tab 背包：↑/↓ 選取、←/→ 翻頁、E/Enter 使用
    ///@}
    /**
     * @brief 每幀的 E 鍵對話／拾取探測派發。
     *
     * 讀取輸入（E 鍵邊緣＋觸及盒），故留在 Controller；它把任務副作用委派給已註冊
     * 的 QuestHook 表（RunInteractHooks），而非內嵌一長串 TryXxx 呼叫。
     */
    void DispatchInteract();

    World&                                               world_;          ///< 被推進的世界
    EventBus&                                            bus_;            ///< 注入的事件匯流排
    std::vector<nccu::engine::math::Rect>                         frameColliders_; ///< 重用的暫存碰撞盒清單（僅動態演員；地形走遮罩）
    nccu::engine::math::Vec2                                       worldSize_;      ///< 世界邊界尺寸（像素）
    nccu::engine::math::Vec2                                       playerSize_;     ///< 玩家碰撞盒尺寸（像素）
    SceneRouter                                          sceneRouter_;    ///< FSM 轉場觀察者（名冊／側效游標、離開閂鎖）
    InputHandler                                         input_;          ///< 輸入計時（按住 E 自動推進對話）
    /// 目前開啟購買選單的攤主。商店互動於本幀開啟選項對話，購買則在「較晚一幀」的
    /// 對話分支確認，故此目標須跨凍結存活。它是對 World 所擁有物件的非擁有觀察指標；
    /// 選單關閉（確認／無動作）或開啟非攤主對話時即清空，故絕不可能在名冊清除之後
    /// 懸空（攤主對話會凍結模擬，故其設定期間名冊清除路徑不會執行）。nullptr 表示
    /// 沒有開啟任何商店選單。
    Vendor*                                              pendingVendor_ = nullptr;

    /// 有序的模型推進管線。每個非凍結幀，於 E 鍵互動／建築進入／gate 輪詢「之前」，
    /// 以此精確順序執行：Survival（降雨）-> Movement（物件 tick）-> Collision
    /// （玩家 AABB＋地形）-> Spawn（繞圈＋延遲生成）。MovementSystem 透過 SimContext
    /// 把 tick 前的玩家座標交給 CollisionSystem。SweepSystem 是「終端」階段（幀末
    /// 延遲刪除），於互動／gate 邏輯「之後」單獨執行，故每幀整體順序維持逐位元保存。
    /// 純模型：沒有 system 讀輸入或渲染。
    std::vector<std::unique_ptr<ISystem>>                advanceSystems_;
    SweepSystem                                          sweep_;          ///< 終端清除階段（幀末延遲刪除）
};

} // namespace nccu

#endif // GAME_CONTROLLER_H_
