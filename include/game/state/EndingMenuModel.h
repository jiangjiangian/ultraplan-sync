#ifndef GAME_STATE_ENDING_MENU_MODEL_H_
#define GAME_STATE_ENDING_MENU_MODEL_H_
#include "game/state/SemesterState.h"
#include <string_view>

/**
 * @file EndingMenuModel.h
 * @brief 結局畫面選單的 game 層模型：結局判定述詞、選項列舉與索引／標籤對映。
 */

namespace nccu {

// 結局畫面選單的 game 層模型。承載結局畫面所暴露、且 game 層程式（場景、控制器、
// 狀態機觀察者）需讀取的模型狀態，使其不必引入 ui 渲染標頭——斷開 game→ui 的反向
// 相依。ui/EndingView.h 仍針對下列型別進行渲染；EndingSummary DTO 與 DrawEndingCard
// 簽章留在 ui 側。

/**
 * @brief 判斷給定狀態是否為四種結局之一。
 * @param s 待判定的學期狀態。
 * @return s 為 Ending_A／B／D／C 時回傳 true。
 *
 * 純狀態述詞（不涉渲染）；結局場景的凍結守衛以此判斷結局選單是否掌控當前幀。
 */
[[nodiscard]] bool IsEndingState(SemesterState s) noexcept;

/**
 * @brief 結局畫面底部三選項選單的選擇列舉。
 *
 * 結局畫面是穩定且可互動的畫面（非被動卡片）：以 ←／→ 移動游標、E／Enter 確認。
 * 依游標順序為 0 回首頁、1 重新開始、2 結束（唯一會關閉視窗的路徑）。本列舉是選單
 * 語意的唯一真實來源；顯示標籤見 EndingMenuLabel()。兩種路由語意皆透過控制器的
 * World::PendingAppAction 實現（View 只負責繪製被選取的列）。
 */
enum class EndingMenuChoice { BackToTitle, RestartGame, Quit };

/**
 * @brief 將游標索引對映為選單選擇。
 * @param index 游標索引（0..2）。
 * @return 對應的 EndingMenuChoice；超出範圍時夾鉗回有效集合，使游標永不會選到「無」。
 */
[[nodiscard]] EndingMenuChoice EndingMenuChoiceAt(int index) noexcept;

/**
 * @brief 取得選擇的螢幕顯示標籤。
 * @param c 選單選擇。
 * @return 對應的繁中標籤字串。
 */
[[nodiscard]] std::string_view EndingMenuLabel(EndingMenuChoice c) noexcept;

} // namespace nccu

#endif // GAME_STATE_ENDING_MENU_MODEL_H_
