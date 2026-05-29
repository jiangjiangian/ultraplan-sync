#ifndef UI_PRESS_LATCH_H_
#define UI_PRESS_LATCH_H_

namespace nccu {

/**
 * @file PressLatch.h
 * @brief 跨「阻塞式畫面」的確認／關閉鍵邊緣去彈跳閘門。
 */

/**
 * @brief 抑制由前一畫面「繼承」而來的同一次按鍵邊緣，直到該鍵被放開過一次。
 *
 * 互動式標題、角色選擇、說明畫面各自跑自己的阻塞繪製迴圈，卻讀同一份全域
 * 按鍵狀態。raylib 的「pressed」邊緣只在某一次輸入輪詢為真，而唯一的輪詢
 * 點是 EndDrawing（nccu::engine::render::DrawScope 的解構子）。當某畫面在
 * Enter 邊緣回傳、下一畫面又在下一次 EndDrawing 前就開始時，新畫面會看到
 * 同一個 Enter 邊緣——於是單一次實體按鍵會驅動兩次轉場：開啟「遊戲說明」會
 * 立刻又把它關掉、確認「開始遊戲」會順帶自動選中第一個角色，諸如此類。
 *
 * PressLatch 在按鍵「自上次 arm 以來」至少被觀察到放開一次之前，會壓住按下
 * 不放行。每個畫面對每個確認鍵各持一個 latch；它從前一畫面繼承來的那次按下
 * 會被忽略，因為新畫面開始時該鍵仍處於按住狀態。純邏輯、不碰 raylib，可無頭
 * 單元測試。
 */
class PressLatch {
public:
    /**
     * @brief 每幀餵入按鍵的位準（down）與邊緣（pressed）各一次。
     * @return 僅在「自上次 arm 以來鍵已放開後」觀察到的第一次新按下回傳 true。
     */
    bool Fired(bool down, bool pressed) noexcept {
        if (!down) armed_ = true;            // 已放開 → 準備好接受下一次按下
        if (armed_ && pressed) {
            armed_ = false;                  // 消耗本次；要求再一次放開才放行
            return true;
        }
        return false;
    }

private:
    bool armed_ = false;   ///< 唯有按鍵被觀察到放開後才為 true
};

} // namespace nccu

#endif // UI_PRESS_LATCH_H_
