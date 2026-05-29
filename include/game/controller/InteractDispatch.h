#ifndef CONTROLLER_INTERACT_DISPATCH_H_
#define CONTROLLER_INTERACT_DISPATCH_H_

class Vendor;
class EventBus;        // 前向宣告——bus 一路傳遞到 RunInteractHooks，毋須拉入完整定義

namespace nccu {

class World;

/**
 * @file InteractDispatch.h
 * @brief 每幀 E 鍵互動的派發（對話／拾取／開啟商店），屬 Controller 輸入層。
 */

/**
 * @brief 處理一次 E 鍵互動：對話、拾取，或開啟商店選單。
 * @param[in,out] bus           發布互動衍生事件（對話、拾取、購買）的 EventBus。
 * @param[in,out] world         當前世界；讀取玩家觸及範圍並可開啟對話／選單。
 * @param[out]    pendingVendor 以參考傳入；當 E 開啟商店時被設為該攤主，否則清空。
 *
 * 讀取 E 鍵邊緣與玩家的觸及盒，故留在 Controller 層（非 ISystem）。對非閒談型
 * NPC 會先依序跑過已註冊的 QuestHook 表（RunInteractHooks，原本約 14 個內嵌
 * TryXxx 呼叫現已成為 quest/QuestHookTable.cpp 的資料，順序與自我守門語意不變），
 * 再開啟對應 (npcId, state) 的對話。閒談型 NPC 短路到自身的逐行循環 Interact；
 * 非 NPC（拾取物／Vendor）則走其 IInteractable 角色。
 *
 * pendingVendor 是對 World 所擁有之 Vendor 的非擁有觀察指標：本幀開啟其購買選單後，
 * HandleDialog 於確認時讀它以導向 Vendor::TryBuy。以參考傳入，讓本函式能在 E 開啟
 * 商店時設定它；一旦開啟非攤主對話、或攤主放棄／購買時即清空。
 */
void DispatchInteract(EventBus& bus, World& world, Vendor*& pendingVendor);

} // namespace nccu

#endif // CONTROLLER_INTERACT_DISPATCH_H_
