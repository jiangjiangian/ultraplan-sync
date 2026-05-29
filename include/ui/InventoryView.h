#ifndef INVENTORY_VIEW_H_
#define INVENTORY_VIEW_H_
#include "game/quest/InventoryPaging.h"  // kInventoryRowsPerPage (game-side)
#include "game/quest/ItemCatalog.h"      // nccu::InventoryRow DTO
#include <vector>

namespace nccu {
namespace engine::render { class IRenderer; }

/**
 * @file InventoryView.h
 * @brief 物品欄疊層的渲染與分頁視窗運算。
 */

// kInventoryRowsPerPage 定義於 game/quest/InventoryPaging.h（單一真實來源，上方
// 已 include），使遊戲層的物品欄場景不必拉進此 ui 標頭即可讀取。View 的分頁運算
// （下方 InventoryPageCount／InventoryPageOf）對齊同一常數。

/**
 * @brief 計算 `rowCount` 列的背包共有幾頁（每頁 kInventoryRowsPerPage 列）。
 * @return 頁數，恆 >=1（空背包為「1／1」）。
 *
 * 純整數運算，公開以便分頁視窗不需 renderer 即可單元測試。
 */
[[nodiscard]] int InventoryPageCount(int rowCount) noexcept;

/**
 * @brief 計算游標所在的頁（0 起算）。
 * @return 游標頁碼。
 *
 * 會先把游標夾限進 [0,rowCount)，使越界游標仍得出有效頁（與 View 自身的夾限一
 * 致）。
 */
[[nodiscard]] int InventoryPageOf(int cursor, int rowCount) noexcept;

/**
 * @brief 繪製 Tab 物品欄疊層：一個置中面板，列出每一筆背包列——金幣、每樣持有
 *        消耗品（含數量與描述）、攜帶的雨傘，以及當前回合的任務紙張——並帶有可
 *        移動游標與選取列的描述面板。空的 `rows` 會畫一行「（空）」。
 *
 * 「純渲染」：只畫傳入的 InventoryRow DTO 與游標索引，別無其他（MVC 純度，與
 * DrawEndingCard 接收 EndingSummary 同理）。DTO 由 quest／controller 層
 * （BuildInventoryRows）從 World／Player 建出；本函式絕不碰 World／Player、不含
 * 遊戲邏輯。此處不碰 raylib——所有繪製皆經注入的 IRenderer，故具決定性且可無頭
 * spy 測試。當列數超過 kInventoryRowsPerPage 時，面板只顯示游標所在頁（選取列恆
 * 可見）並附「第 N／M 頁」指示；放大後的框與各列描述區會把（較長的）效果文字以
 * nccu::dialog::WrapToCells 折在邊框內，絕不溢出。
 */
void DrawInventory(nccu::engine::render::IRenderer& r,
                   const std::vector<InventoryRow>& rows,
                   int cursor,
                   float screenW, float screenH);

} // namespace nccu

#endif // INVENTORY_VIEW_H_
