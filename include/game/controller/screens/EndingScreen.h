#ifndef CONTROLLER_SCREENS_ENDING_SCREEN_H_
#define CONTROLLER_SCREENS_ENDING_SCREEN_H_

namespace nccu {

class World;

/**
 * @file EndingScreen.h
 * @brief 結局畫面處理器：到達 Ending_* 狀態後完全凍結世界，只開放底部
 *        三選項選單（回首頁／重新開始／結束）。屬 Controller 輸入層。
 */

/**
 * @brief 處理結局畫面這一幀的底部三選項選單。
 * @param[in,out] world 當前世界；讀取選單游標、移動游標並轉送 AppAction。
 * @return 目前處於 Ending_* 狀態時回傳 true（畫面持有此幀）；否則回傳 false。
 *
 * 處於 Ending_* 狀態時整個世界被凍結（Controller 在 pipeline／互動／清除之前
 * 即回傳），故玩家唯一的操作就是此選單。←/→ 移動游標；E/Enter 確認反白選項並
 * 透過 World::RequestAppAction 轉送。回首頁與重新開始皆請求 Restart（main.cpp
 * 把整局拆回標題畫面，完整重置狀態；重新開始者再由標題重新進入）；結束請求
 * Quit（唯一關閉視窗的路徑）。此處不讀 ESC——它仍是 main.cpp 所擁有的離開鍵。
 */
[[nodiscard]] bool HandleEndingMenu(World& world);

} // namespace nccu

#endif // CONTROLLER_SCREENS_ENDING_SCREEN_H_
