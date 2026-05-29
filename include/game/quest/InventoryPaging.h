#ifndef GAME_QUEST_INVENTORY_PAGING_H_
#define GAME_QUEST_INVENTORY_PAGING_H_

namespace nccu {

/**
 * @file InventoryPaging.h
 * @brief Tab 背包面板的純遊戲領域分頁常數。
 *
 * 自 ui/InventoryView.h 抽出，使遊戲層控制器（InventoryScreen.cpp）能讀取它而不必
 * 拉進 ui/ 渲染標頭——封住一條 game→ui 的反向相依，同時讓此值維持 ui/InventoryView
 * 仍消費的單一事實來源（其分頁計算與翻頁動作都取用同一個 kInventoryRowsPerPage）。
 */

/// Tab 背包每頁固定列數。列數超過此值時面板改採「分頁」，而非把每列擠進不到
/// 28px 的窄帶（那也會使描述溢位）。顯示的頁面永遠是包含游標的那一頁（於
/// ui/InventoryView 由 InventoryPageOf 推導），故選取列絕不在畫面外；背包場景中明
/// 確的左右翻頁動作會把游標移動 ±此值。
inline constexpr int kInventoryRowsPerPage = 6;

} // namespace nccu

#endif // GAME_QUEST_INVENTORY_PAGING_H_
