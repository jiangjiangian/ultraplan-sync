#ifndef SCENE_ROUTER_H_
#define SCENE_ROUTER_H_
#include "game/state/SemesterState.h"

namespace nccu {

class World;

/**
 * @file SceneRouter.h
 * @brief 把「章節／市集／結局」轉場觀察者從 GameController 抽出的類別。
 */

/**
 * @brief 觀察 SemesterState 機台、讓 World 跟隨 FSM 轉場的觀察者（Observer）。
 *
 * 三項職責、單一焦點——「讓 World 跟隨 FSM」：
 *   1. 偵測 SemesterState 相對上次觀察值是否改變。
 *   2. 套用各目的地的副作用（NPC 名冊重生、進入市集的重新定位＋消耗品清空、
 *      Ch4 雨傘重置、離開閂鎖重置、抵達市集的 ShowMessage）。
 *   3. 標記游標，使同一幀後續的讀取（View／序列化快照）看到一致的
 *      {semester, npcs[], player}。
 *
 * 一個一幀延遲的視覺問題：早期 Controller 在「下一幀」Update() 開頭才觀察到狀態
 * 改變——比轉場觸發晚一幀，導致 View 畫出 semester=新但 npcs[]=舊 的一幀。為此把
 * 結算「故意」拆成兩個入口點，使修正在視覺上精準：
 *
 *   SettleRoster(World)
 *     僅做名冊抽換（章節 NPC 跟隨 FSM）。於 Update() 結尾呼叫，使轉場觸發的同一幀
 *     就畫出（並記錄）新名冊。不寫玩家座標、不清消耗品、不發事件、不改旗標：那些是
 *     唯讀世界快照用來解析「下一幀」腳本步驟的可觀察副作用，於本幀中途更動會破壞腳本
 *     （已驗證：在轉場幀直接傳送玩家的單發版本會卡住某結局流程）。
 *
 *   SettleSideEffects(World)
 *     另一半（玩家重新定位、ClearConsumables、Ch4 雨傘重置、離開閂鎖重置、抵達市集
 *     的 ShowMessage）。於 Update() 開頭呼叫——正是早期內嵌區塊觸發之處——使可觀察的
 *     {player.pos, consumables, flags, events} 時間軸維持不變。
 *
 * 兩個入口各自內部檢查游標（cur == 對應游標）故單獨呼叫任一個都安全；合起來的效果
 * 是「晚一拍、但可觀察結果與先前相同」。
 */
class SceneRouter {
public:
    /**
     * @brief 以 World 建構時的 SemesterState 作為初始游標。
     * @param[in] initial 初始學期狀態；呼叫端於 GameController 建構時傳入
     *                    World::Semester().Current()。
     */
    explicit SceneRouter(SemesterState initial) noexcept
        : lastRosterState_(initial),
          lastRosterRespawnState_(initial) {}

    SceneRouter(const SceneRouter&)            = delete;
    SceneRouter& operator=(const SceneRouter&) = delete;

    /**
     * @brief 僅做 NPC 名冊抽換，使 FSM 轉場的同一幀畫出一致的 npcs[]。
     * @param[in,out] world 要重生名冊的世界。
     *
     * 於 Update() 結尾呼叫。未發生轉場時呼叫亦安全（冪等：追蹤獨立游標
     * lastRosterRespawnState_，故能在 SettleSideEffects 之前推進而不互相餓死）。
     */
    void SettleRoster(World& world);

    /**
     * @brief 副作用半部：玩家座標＋消耗品＋旗標＋抵達提示＋閂鎖重置。
     * @param[in,out] world 要套用副作用的世界。
     *
     * 於 Update() 開頭呼叫，使可觀察時間軸與早期一致；並標記 lastRosterState_ 讓
     * 下次轉場的 SettleRoster 能偵測到新的改變。若 SettleRoster 因故被略過，此處
     * 會防禦性地一併重生名冊——兩半部每次轉場各恰好執行一次。
     */
    void SettleSideEffects(World& world);

    /**
     * @brief 取得市集離開區提示的閂鎖（可變參考）。
     * @return 對 interludeExitZoneLatched_ 的可變參考。
     *
     * GameController 在玩家越過南側帶狀區時，把它轉給 MaybeAnnounceInterludeExit。
     * 放在此處讓所有與轉場相關的狀態集中於一處。
     */
    [[nodiscard]] bool& InterludeExitLatchMut() noexcept {
        return interludeExitZoneLatched_;
    }

    /** @brief 測試檢視：上次套用副作用所對應的 FSM 狀態。 */
    [[nodiscard]] SemesterState LastRosterState() const noexcept {
        return lastRosterState_;
    }
    /** @brief 測試檢視：上次名冊重生所對應的 FSM 狀態。 */
    [[nodiscard]] SemesterState LastRosterRespawnState() const noexcept {
        return lastRosterRespawnState_;
    }

private:
    /// SettleSideEffects 的游標：上次副作用套用所對應的 FSM 狀態；只在
    /// SettleSideEffects 中推進。
    SemesterState lastRosterState_;
    /// SettleRoster 的游標：上次名冊重生所對應的 FSM 狀態。獨立游標讓 SettleRoster
    /// 能在 SettleSideEffects 之前執行而不互相餓死（各自在自己的游標下冪等）。
    SemesterState lastRosterRespawnState_;
    /// 市集離開區提示的「每次到訪一次」閂鎖。在 SettleSideEffects 的市集抵達分支
    /// 重置為 false，使重訪時恰好重發提示一次。
    bool          interludeExitZoneLatched_ = false;
};

} // namespace nccu

#endif // SCENE_ROUTER_H_
