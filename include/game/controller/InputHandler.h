#ifndef INPUT_HANDLER_H_
#define INPUT_HANDLER_H_
#include "engine/input/Input.h"
#include "engine/input/Key.h"

namespace nccu {

/**
 * @file InputHandler.h
 * @brief 從 GameController 抽出的每幀輸入層，讓 Controller 維持 orchestrator
 *        角色，而由一個聚焦的類別專責輸入的邊緣／按住計時。
 */

/**
 * @brief 專責跨幀輸入計時的小類別（目前僅「按住 E 自動推進對話」）。
 *
 * 目前只有「按住 E 自動推進對話」帶有跨幀狀態；其餘對 Input 的讀取都走無狀態的
 * 邊緣判斷式（IsPressed／IsDown／IsReleased），留在各自呼叫點即可。把此自動推進
 * 計時器收進專屬類別的好處：
 *   - 把輸入契約（E 邊緣，或按住 E ≥ 300 ms 且兩次自動觸發間有 4 幀冷卻）釘在
 *     單一處，回歸測試無須啟動整個 Controller／World 即可驅動它。
 *   - 讓 Update() 迴圈不再散落可變的按住狀態。
 *   - 未來的輸入重構（重新綁定、多來源合併）可落在此處，而非繼續膨脹 Controller。
 *
 * 純輸入計時：不變動 World、不發布事件——由呼叫端決定如何處理布林回傳值。相對於
 * 其餘模擬為無狀態，故 doctest 可餵入合成的 InputSource 並斷言精確的邊緣／冷卻語意。
 */
class InputHandler {
public:
    InputHandler() = default;

    /**
     * @brief 詢問玩家本幀是否發出了「推進對話」的請求。
     * @param[in] dt 幀間隔（秒），來源為 nccu::engine::platform::Time::DeltaSeconds()。
     * @return 本幀應推進對話時回傳 true。
     *
     * 下列任一情況為 true：邊緣 IsPressed(E)（與舊有單幀輕點語意相同）；或連續
     * 按住 E 達 kHoldAdvanceMs 且內部冷卻已歸零（每次自動觸發都重新計冷卻，讓玩家
     * 能逐頁閱讀而非以每秒 60 頁的速度閃過）。單幀內冪等：同一 tick 第二次呼叫回傳
     * 相同布林（除按住計時器隨 dt 遞增外無內部變動，而 Controller 每幀至多呼叫一次）。
     */
    bool TickDialogAdvance(float dt) noexcept;

    /**
     * @brief 對話剛關閉（或本幀從未開啟）時呼叫，丟棄按住計時。
     *
     * 使下次對話開啟時玩家須重新按住 E——對話框出現前累積的舊 300 ms 讀數不可在新
     * 對話的第一幀就自動觸發。低成本且冪等。
     */
    void ResetDialogAdvance() noexcept {
        eHoldMs_ = 0.0f;
        eAutoAdvanceCooldown_ = 0;
    }

    /// 測試接縫：玩家須連續按住 E 多久（毫秒）自動推進才會啟動；由回歸測試釘住。
    static constexpr float kHoldAdvanceMs = 300.0f;
    /// 測試接縫：每次觸發後自動推進靜默幾幀，以維持可閱讀的節奏。
    static constexpr int   kAutoCooldownFrames = 4;

    /** @brief 測試專用檢視：目前 E 已按住的毫秒數（正式邏輯不依賴）。 */
    [[nodiscard]] float HoldMs() const noexcept { return eHoldMs_; }
    /** @brief 測試專用檢視：自動推進剩餘的冷卻幀數（正式邏輯不依賴）。 */
    [[nodiscard]] int   Cooldown() const noexcept {
        return eAutoAdvanceCooldown_;
    }

private:
    float eHoldMs_ = 0.0f;             ///< E 已按住的毫秒數；放開時歸零
    int   eAutoAdvanceCooldown_ = 0;   ///< 每次自動觸發後要略過的幀數
};

} // namespace nccu

#endif // INPUT_HANDLER_H_
