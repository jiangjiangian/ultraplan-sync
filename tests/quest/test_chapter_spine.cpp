/**
 * @file test_chapter_spine.cpp
 * @brief 驗證章節主幹的可重入走訪：Ch1→市→Ch2→市→Ch3→市→Ch4，含離開旗標消耗與轉場關閉對話。
 */
#include "doctest/doctest.h"
#include "game/quest/Flags.h"
#include "game/quest/ChapterGate.h"
#include "game/state/EndingGate.h"
#include "game/controller/EventWiring.h"
#include "game/state/SemesterStateMachine.h"
#include "engine/events/EventBus.h"
#include "game/dialog/DialogState.h"
#include "game/entities/Player.h"
#include "engine/math/Vec2.h"
#include <string>
using nccu::SemesterStateMachine;
using nccu::SemesterState;

// 完整走一遍可重入的章節主幹：Ch1 → 市 → Ch2 → 市 → Ch3 → 市 → Ch4。
// 幕間市集會進入 3 次；returnTo 存在狀態機上（而非每次重建的
// InterludeMarket 物件），負責把每次離開市集導向下一章。
TEST_CASE("chapter spine: Ch1 -> 市 -> Ch2 -> 市 -> Ch3 -> 市 -> Ch4") {
    EventBus::Instance().Clear();
    SemesterStateMachine m;
    Player p{nccu::engine::math::Vec2{0, 0}};
    nccu::DialogState d;
    std::string name;
    nccu::WireStateTransitionSubscribers(EventBus::Instance(), m, name);

    REQUIRE(m.Current() == SemesterState::Chapter1_AddDrop);

    // --- Ch1 → 幕間市集，走既有的 UmbrellaClaimed 路徑；訂閱者同時把
    //     returnTo 設為 Ch2（第一座市集接在 Ch1 之後）。---
    EventBus::Instance().Publish(Event{ EventType::UmbrellaClaimed, "TrueUmbrella" });
    CHECK(m.Current() == SemesterState::Interlude_Market);
    CHECK(m.InterludeReturnTo() == SemesterState::Chapter2_Midterms);

    // --- 市集 1 → Ch2；離開旗標在轉場時被消耗。---
    p.SetFlag(nccu::kFlagLeaveInterlude);
    nccu::CheckChapterGates(EventBus::Instance(), p, m, d);
    CHECK(m.Current() == SemesterState::Chapter2_Midterms);
    CHECK_FALSE(p.HasFlag(nccu::kFlagLeaveInterlude));   // 已被消耗

    // --- Ch2 清關 → 幕間市集，這次 returnTo 改為 Ch3。---
    p.SetFlag(nccu::kFlagCh2Cleared);
    nccu::CheckChapterGates(EventBus::Instance(), p, m, d);
    CHECK(m.Current() == SemesterState::Interlude_Market);
    CHECK(m.InterludeReturnTo() == SemesterState::Chapter3_SportsDay);

    // --- 市集 2 → Ch3。---
    p.SetFlag(nccu::kFlagLeaveInterlude);
    nccu::CheckChapterGates(EventBus::Instance(), p, m, d);
    CHECK(m.Current() == SemesterState::Chapter3_SportsDay);
    CHECK_FALSE(p.HasFlag(nccu::kFlagLeaveInterlude));

    // --- Ch3 清關 → 幕間市集，這次 returnTo 改為 Ch4。---
    p.SetFlag(nccu::kFlagCh3Cleared);
    nccu::CheckChapterGates(EventBus::Instance(), p, m, d);
    CHECK(m.Current() == SemesterState::Interlude_Market);
    CHECK(m.InterludeReturnTo() == SemesterState::Chapter4_Finals);

    // --- 市集 3 → Ch4（沒有第 4 座市集；Ch4 → 結局是後續階段）。---
    p.SetFlag(nccu::kFlagLeaveInterlude);
    nccu::CheckChapterGates(EventBus::Instance(), p, m, d);
    CHECK(m.Current() == SemesterState::Chapter4_Finals);
    CHECK_FALSE(p.HasFlag(nccu::kFlagLeaveInterlude));

    // --- 健全性檢查：Ch4 此處沒有同層 if 轉場，主幹到此結束。---
    nccu::CheckChapterGates(EventBus::Instance(), p, m, d);
    CHECK(m.Current() == SemesterState::Chapter4_Finals);

    EventBus::Instance().Clear();
}

// 重新進入市集而沒有新的離開旗標時，不得立刻退出（旗標只在離開時消耗一次）。
TEST_CASE("chapter spine: re-entry does not instantly exit (flag consumed)") {
    SemesterStateMachine m;
    Player p{nccu::engine::math::Vec2{0, 0}};
    nccu::DialogState d;

    m.SetInterludeReturnTo(SemesterState::Chapter3_SportsDay);
    m.Transition(SemesterState::Interlude_Market);

    // 第一次離開會消耗 Flag_LeaveInterlude。
    p.SetFlag(nccu::kFlagLeaveInterlude);
    nccu::CheckChapterGates(EventBus::Instance(), p, m, d);
    CHECK(m.Current() == SemesterState::Chapter3_SportsDay);

    // 重新進入市集；沒有新旗標就不應退出。
    m.Transition(SemesterState::Interlude_Market);
    nccu::CheckChapterGates(EventBus::Instance(), p, m, d);
    CHECK(m.Current() == SemesterState::Interlude_Market);
}

// 轉場時若仍有開啟中的對話，閘門應將其關閉，避免殘留在新章節。
TEST_CASE("chapter spine: gate closes a still-active dialog on transition") {
    SemesterStateMachine m;
    Player p{nccu::engine::math::Vec2{0, 0}};
    nccu::DialogState d;

    m.Transition(SemesterState::Chapter2_Midterms);
    d.Open({"queued line"});
    REQUIRE(d.Active());
    p.SetFlag(nccu::kFlagCh2Cleared);
    nccu::CheckChapterGates(EventBus::Instance(), p, m, d);
    CHECK(m.Current() == SemesterState::Interlude_Market);
    CHECK_FALSE(d.Active());
}

// 沒有設任何旗標時，任一章節都不應發生轉場（守住誤觸條件）。
TEST_CASE("chapter spine: no flags -> no transition at any chapter") {
    SemesterStateMachine m;
    Player p{nccu::engine::math::Vec2{0, 0}};
    nccu::DialogState d;

    nccu::CheckChapterGates(EventBus::Instance(), p, m, d);                 // Ch1：沒有同層 if 轉場
    CHECK(m.Current() == SemesterState::Chapter1_AddDrop);

    m.Transition(SemesterState::Chapter2_Midterms);
    nccu::CheckChapterGates(EventBus::Instance(), p, m, d);
    CHECK(m.Current() == SemesterState::Chapter2_Midterms);

    m.Transition(SemesterState::Interlude_Market);
    nccu::CheckChapterGates(EventBus::Instance(), p, m, d);
    CHECK(m.Current() == SemesterState::Interlude_Market);
}
