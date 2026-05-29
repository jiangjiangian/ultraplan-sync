#ifndef CHAPTER_TOAST_H_
#define CHAPTER_TOAST_H_
#include "engine/events/EventBus.h"
#include "game/state/SemesterState.h"
#include <string>

/**
 * @file ChapterToast.h
 * @brief 章節／幕間／結局轉移的過場提示文字與發布輔助，外加幕間出口提示。
 */

namespace nccu {

/**
 * @brief 將轉移目標狀態對映為其過場提示顯示文字。
 * @param target 轉移後的目標 SemesterState。
 * @return 對應的繁中提示文字；列舉未涵蓋時回傳空字串。
 *
 * 純資料對映：每次章節／幕間／結局轉移都發布一則短訊息，讓玩家在學期狀態機推進的
 * 當下即看到回饋（否則轉移是無聲的，只有目的地的任務目標暗示進度）。以值回傳而非
 * const 參考，方便未來呼叫端裝飾文字而不必為每個變體強加靜態生命週期。所有字串都
 * 控制在對話框單行字數預算內。
 */
[[nodiscard]] inline std::string ChapterTransitionToast(
    SemesterState target) {
    switch (target) {
        case SemesterState::Chapter1_AddDrop:
            return "✓ 進入第一章 加退選";
        case SemesterState::Interlude_Market:
            return "✓ 章節清關 — 進入幕間市集";
        case SemesterState::Chapter2_Midterms:
            return "✓ 進入第二章 期中考";
        case SemesterState::Chapter3_SportsDay:
            return "✓ 進入第三章 運動會";
        case SemesterState::Chapter4_Finals:
            return "✓ 進入第四章 期末考";
        case SemesterState::Ending_A:
        case SemesterState::Ending_B:
        case SemesterState::Ending_D:
        case SemesterState::Ending_C:
            return "✓ 抵達結局";
    }
    return std::string{};
}

/**
 * @brief 在事件匯流排上發布章節／結局過場提示。
 * @param bus    要發布訊息的事件匯流排（由呼叫端注入）。
 * @param target 轉移後的目標狀態，用以查得提示文字。
 *
 * 每個轉移點呼叫一次。提示文字為空（switch 覆蓋完整時不應發生）即視為無操作，避免
 * 未來新增列舉值時意外送出空白橫幅。章節／結局提示固定走 HudSlot::Top，使同一幀在
 * Bottom 槽發布的其他訊息（真傘拾取、幕間抵達提示、業力提示）能與章節橫幅並存而不
 * 互相覆蓋。
 */
inline void PublishChapterTransitionToast(EventBus& bus, SemesterState target) {
    const std::string msg = ChapterTransitionToast(target);
    if (msg.empty()) return;
    bus.Publish(Event{EventType::ShowMessage, msg, nccu::HudSlot::Top});
}

/// @brief 抵達幕間市集中央時的引導提示（提醒玩家逛完往南離開）。
inline constexpr const char* kInterludeArrivalHint =
    "市集中央。逛完後往南離開";
/// @brief 走進南側出口觸發區時的離場提示。
inline constexpr const char* kInterludeExitPrep =
    "準備離開市集";

/**
 * @brief 每次造訪只播一次的幕間出口提示閂鎖輔助。
 * @param bus     要發布提示的事件匯流排（由呼叫端注入）。
 * @param latched 進／出狀態閂鎖；首次呼叫（為 false 時）發布並設為 true。
 * @return 本次實際發布回傳 true；latched 已為 true 時為無操作並回傳 false。
 *
 * 幕間南側出口區本身是無聲位置觸發；此閂鎖讓玩家在出口邊界來回抖動時不會洗版 HUD。
 * 純邏輯，讓 GameController 整合與回歸測試走同一條程式路徑；閂鎖的重置（幕間進入時）
 * 由呼叫端負責。
 */
inline bool MaybeAnnounceInterludeExit(EventBus& bus, bool& latched) {
    if (latched) return false;
    latched = true;
    bus.Publish(Event{EventType::ShowMessage, kInterludeExitPrep});
    return true;
}

} // namespace nccu

#endif // CHAPTER_TOAST_H_
