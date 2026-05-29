#ifndef GAME_STATE_GAME_HELP_PAGES_H_
#define GAME_STATE_GAME_HELP_PAGES_H_

/**
 * @file GameHelpPages.h
 * @brief 遊戲說明覆蓋層的頁數常數（game 層用，與 ui 內容定義保持同步）。
 */

namespace nccu {

/**
 * @brief 遊戲說明覆蓋層的總頁數。
 *
 * 自 ui/GameHelp.h 抽出，讓需要翻頁的 game 層場景／控制器（標題場景、暫停畫面）能讀
 * 取頁數而不必引入 ui 渲染標頭——斷開 game→ui 的反向相依。ui/GameHelp.h 仍定義實際
 * 頁面內容，此值即其陣列大小，兩者由 ui 端的 static_assert 釘住一致。此處刻意重複此
 * 小常數而非間接引入，使 game 層編譯不相依於 ui；未來新增頁面時須同時更新兩處，由該
 * static_assert 把關。
 */
inline constexpr int kGameHelpPageCount = 2;

} // namespace nccu

#endif // GAME_STATE_GAME_HELP_PAGES_H_
