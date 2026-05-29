#include "game/quest/ChapterGate.h"
#include "game/quest/Flags.h"
#include "game/state/ChapterToast.h"
#include "engine/events/EventBus.h"
#include "game/entities/Player.h"
#include "game/state/SemesterStateMachine.h"
#include "game/state/SemesterState.h"
#include "game/dialog/DialogState.h"

namespace nccu {

void CheckChapterGates(EventBus& bus, Player& player,
                       SemesterStateMachine& semester, DialogState& dialog) {
    // 第二章通關 -> 回到市集，再由市集返回第三章。
    // Flag_Ch2Cleared 是刻意保留的主線樁：日後由真正的第二章任務設下。現在先把
    // 轉場接好，讓整段進程在該內容尚未存在前即可走訪與測試。
    if (semester.Current() == SemesterState::Chapter2_Midterms &&
        player.HasFlag(kFlagCh2Cleared)) {
        semester.SetInterludeReturnTo(SemesterState::Chapter3_SportsDay);
        semester.Transition(SemesterState::Interlude_Market);
        // 公告目的地，讓玩家看到 FSM 確實移動。在 Transition() 之「後」才發布，
        // 使會對該文字反應的訂閱者（HUD 鏡像）若查詢時讀到的是已切換後的當前狀態。
        PublishChapterTransitionToast(bus, SemesterState::Interlude_Market);
        dialog.Close();
    }

    // 第三章通關 -> 回到市集，再由市集返回第四章。
    // Flag_Ch3Cleared 是第三章任務對應的主線樁。
    if (semester.Current() == SemesterState::Chapter3_SportsDay &&
        player.HasFlag(kFlagCh3Cleared)) {
        semester.SetInterludeReturnTo(SemesterState::Chapter4_Finals);
        semester.Transition(SemesterState::Interlude_Market);
        PublishChapterTransitionToast(bus, SemesterState::Interlude_Market);
        dialog.Close();
    }

    // 離開插曲段時，返回先前設定 returnTo 的章節（第一章的 UmbrellaClaimed 閘 ->
    // 第二章；上面兩個 if -> 第三／第四章）。Flag_LeaveInterlude 由市集的「公告板」
    // NPC 設下；主線測試則直接設定它。轉場前先消費（清除）它，避免日後再次進入插曲段
    // 時又立刻被彈出。
    if (semester.Current() == SemesterState::Interlude_Market &&
        player.HasFlag(kFlagLeaveInterlude)) {
        player.ClearFlag(kFlagLeaveInterlude);
        const SemesterState target = semester.InterludeReturnTo();
        semester.Transition(target);
        PublishChapterTransitionToast(bus, target);
        dialog.Close();
    }
}

} // namespace nccu
