/**
 * @file test_press_latch.cpp
 * @brief 驗證 PressLatch 防止「單次按鍵跨越多個阻塞畫面而重複觸發」：
 *        繼承自前一畫面的按鍵必須先被釋放、重新武裝後，下一次按下才會觸發。
 */
//
// 標題／選角／說明這些互動畫面共用全域的 raylib 按鍵狀態，而「剛按下」這個邊緣
// 訊號會持續到下一次輪詢。當某畫面在 Enter 上返回、下一個畫面卻在輪詢前就開始，
// 同一個 Enter 會觸發兩次 —— 說明畫面開了又瞬間關、開始遊戲直接略過選角。
//
// PressLatch 的修正：在按鍵自武裝後被觀察到釋放之前，忽略任何按下。下列為純邏輯
// 測試（無視窗／GL），可在無頭環境執行。

#include "doctest/doctest.h"
#include "ui/PressLatch.h"

using nccu::PressLatch;

// 從前一畫面延續按住的按鍵會被抑制；釋放後重新武裝，下一次新按下才觸發一次。
TEST_CASE("PressLatch 抑制從前一畫面延續按住的按鍵") {
    PressLatch l;
    // 建立鎖存時按鍵已按下（Enter 由前一畫面延續）。無論按住多久都不該觸發。
    CHECK_FALSE(l.Fired(/*down=*/true, /*pressed=*/true));   // 進入畫面那一幀
    CHECK_FALSE(l.Fired(true, false));                       // 仍按住
    CHECK_FALSE(l.Fired(true, false));

    // 釋放後鎖存武裝；下一次新按下恰好觸發一次，且按住期間不會自動重複。
    CHECK_FALSE(l.Fired(false, false));                      // 釋放 → 武裝
    CHECK(l.Fired(true, true));                              // 新按下，觸發
    CHECK_FALSE(l.Fired(true, false));                       // 按住，不重複
    CHECK_FALSE(l.Fired(true, true));                        // 不會重複觸發
}

// 當按鍵一開始就是釋放狀態時，第一次按下即觸發。
TEST_CASE("按鍵一開始是釋放狀態時 PressLatch 在第一次按下即觸發") {
    PressLatch l;
    CHECK_FALSE(l.Fired(false, false));   // 第 1 幀：按鍵抬起 → 武裝
    CHECK(l.Fired(true, true));           // 第一次真正按下，觸發
}

// 模擬「標題 -> 遊戲說明 -> 標題」這趟來回。
TEST_CASE("PressLatch 模擬「標題 → 遊戲說明 → 標題」這趟來回") {
    // 一次實體 Enter 必須只開啟說明、不會同時關閉它；而按住不放的關閉用 Enter
    // 回到標題後也不可再次開啟說明。
    PressLatch titleConfirm;
    CHECK_FALSE(titleConfirm.Fired(false, false));  // 抵達時 Enter 抬起 → 武裝
    CHECK(titleConfirm.Fired(true, true));          // 在遊戲說明上按 Enter → 開啟

    // 說明畫面的鎖存：進入時開啟用的 Enter 仍按著。
    PressLatch helpDismiss;
    CHECK_FALSE(helpDismiss.Fired(true, true));     // 進入畫面那一幀：被抑制
    CHECK_FALSE(helpDismiss.Fired(true, false));    // 仍按著（由標題延續）
    CHECK_FALSE(helpDismiss.Fired(false, false));   // 釋放 → 武裝
    CHECK(helpDismiss.Fired(true, true));           // 新按下的 Enter → 關閉

    // 回到標題、關閉用的 Enter 仍按著：不可觸發（titleConfirm 已觸發過，
    // 在釋放前維持未武裝）。
    CHECK_FALSE(titleConfirm.Fired(true, true));    // 殘留的關閉邊緣訊號
    CHECK_FALSE(titleConfirm.Fired(true, false));
    CHECK_FALSE(titleConfirm.Fired(false, false));  // 釋放 → 重新武裝
    CHECK(titleConfirm.Fired(true, true));          // 新的按下又能運作
}

// 模擬「標題開始遊戲 -> 選角」的交接。
TEST_CASE("PressLatch 模擬「標題開始遊戲 → 選角」的交接") {
    // 確認開始遊戲的 Enter 在選角畫面開始時仍按著；選角自己的鎖存必須吞掉這次
    // 按下，以免第 1 幀就自動確認第 0 個角色。
    PressLatch selectConfirm;
    CHECK_FALSE(selectConfirm.Fired(true, true));   // 繼承的 Enter：忽略
    CHECK_FALSE(selectConfirm.Fired(true, false));  // 移動中，仍按著
    CHECK_FALSE(selectConfirm.Fired(false, false)); // 釋放 → 武裝
    CHECK(selectConfirm.Fired(true, true));         // 玩家確認某個角色
}
