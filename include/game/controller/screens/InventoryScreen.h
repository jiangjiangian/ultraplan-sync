#ifndef CONTROLLER_SCREENS_INVENTORY_SCREEN_H_
#define CONTROLLER_SCREENS_INVENTORY_SCREEN_H_

class EventBus;       // 前向宣告——ApplyConsumableEffect 需要 bus，毋須拉入完整定義

namespace nccu {

class World;

/**
 * @file InventoryScreen.h
 * @brief Tab 背包覆蓋層處理器：開啟期間如對話框般凍結模擬，提供「持有並使用」
 *        的道具清單操作。屬 Controller 輸入層。
 */

/**
 * @brief 處理 Tab 背包覆蓋層這一幀。
 * @param[in,out] bus   使用消耗品時供 ApplyConsumableEffect 發布效果訊息的 EventBus。
 * @param[in,out] world 當前世界；讀寫背包開關／游標並讀取玩家道具。
 * @return 背包開啟期間回傳 true（世界維持凍結）；背包關閉時回傳 false。
 *
 * 採邊緣觸發開關；開啟時如對話框般完全凍結模擬（orchestrator 在 pipeline／互動／
 * 清除之前即回傳）。在對話處理器之後才檢查，使對話具優先權，Tab 無法在對話中彈出
 * 面板。背包是「持有並使用」清單：↑/↓ 移動游標；在消耗品列按 E/Enter 即使用它
 * （套用與拾取相同的效果，再扣減數量）；在僅供檢視的列（金幣／雨傘／任務紙張）
 * 按 E/Enter 無作用。←/→ 整頁跳動（kInventoryRowsPerPage，夾限、不環繞）。
 */
[[nodiscard]] bool HandleInventory(EventBus& bus, World& world);

} // namespace nccu

#endif // CONTROLLER_SCREENS_INVENTORY_SCREEN_H_
