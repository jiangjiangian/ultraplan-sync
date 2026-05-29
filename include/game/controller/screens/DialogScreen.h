#ifndef CONTROLLER_SCREENS_DIALOG_SCREEN_H_
#define CONTROLLER_SCREENS_DIALOG_SCREEN_H_

class Vendor;
class EventBus;     // 前向宣告——bus 轉發給下方各 gate 呼叫，毋須拉入完整定義

namespace nccu {

class World;
class InputHandler;
class SceneRouter;

/**
 * @file DialogScreen.h
 * @brief 對話畫面處理器：對話開啟期間凍結世界，推進對話、套用選項副作用，
 *        並把確認的攤主庫存選項導向 Vendor::TryBuy。屬 Controller 輸入層。
 */

/**
 * @brief 處理對話畫面這一幀：推進對話／套用選項／攤主購買確認。
 * @param[in,out] bus           轉發給結局／章節 gate 的 EventBus。
 * @param[in,out] world         當前世界；持有對話狀態與玩家。
 * @param[in,out] pendingVendor 待確認的攤主；放棄／購買／被非攤主對話取代時於此清空。
 * @param[in,out] input         擁有「按住 E 自動推進」邊緣計時的 InputHandler。
 * @param[in,out] sceneRouter   同幀轉場後負責結算 NPC 名冊的 SceneRouter。
 * @return 對話開啟期間回傳 true（世界維持凍結）；無對話進行時回傳 false。
 *
 * 對話凍結：對話開啟時整個世界暫停——orchestrator 只跑這個處理器，略過物件
 * tick／移動／碰撞／建築進入／清除。輸入採邊緣觸發，故上一幀於 DispatchInteract
 * 開啟對話框的那次 E 不會在本幀又把它推進。pendingVendor 以參考傳入是因為它會在
 * 本函式內被改寫。
 */
[[nodiscard]] bool HandleDialog(EventBus& bus, World& world,
                                Vendor*& pendingVendor,
                                InputHandler& input,
                                SceneRouter& sceneRouter);

} // namespace nccu

#endif // CONTROLLER_SCREENS_DIALOG_SCREEN_H_
