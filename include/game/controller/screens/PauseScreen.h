#ifndef CONTROLLER_SCREENS_PAUSE_SCREEN_H_
#define CONTROLLER_SCREENS_PAUSE_SCREEN_H_

namespace nccu {

class World;

/**
 * @file PauseScreen.h
 * @brief 遊戲內暫停選單處理器：M 開啟，開啟期間完全凍結世界；其上可再疊一層
 *        說明覆蓋層。屬 Controller 輸入層。
 */

/**
 * @brief 處理暫停選單（及其說明覆蓋層）這一幀。
 * @param[in,out] world 當前世界；讀寫選單／說明開關與游標並轉送 AppAction。
 * @return 選單（或其說明覆蓋層）開啟期間、或 M 開啟它的那一幀回傳 true；
 *         本幀與選單無關時回傳 false。
 *
 * 以 M 開啟（位於右上）；開啟時整個世界完全凍結（orchestrator 在物件 tick／
 * 移動／降雨／清除之前即回傳，與對話／背包凍結相同）。在結局畫面之後才檢查，
 * 使暫停優先於對話或 Tab 背包；M 同時用來關閉（切換／繼續）。
 *
 * 說明覆蓋層疊在暫停選單之上：其開啟時 M/E/Enter 將它收回選單，而選單游標與
 * 模擬維持凍結。覆蓋層分頁（←/→ 在「操作+目標」與「雨傘外觀+道具須知+結局」
 * 兩頁間翻動）。此處不讀 ESC——它仍是 main.cpp 所擁有的直接離開鍵。Restart／
 * Quit 僅在 World 上「記錄」意圖，真正的拆解發生在 main.cpp 的外層迴圈。
 */
[[nodiscard]] bool HandlePauseMenu(World& world);

} // namespace nccu

#endif // CONTROLLER_SCREENS_PAUSE_SCREEN_H_
