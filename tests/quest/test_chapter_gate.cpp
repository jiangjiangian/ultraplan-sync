/**
 * @file test_chapter_gate.cpp
 * @brief 驗證章節閘門：只有在 Ch1 領到 TrueUmbrella 才推進到幕間市集，其餘事件不誤觸。
 */
#include "doctest/doctest.h"
#include "game/controller/EventWiring.h"
#include "game/state/SemesterStateMachine.h"
#include "engine/events/EventBus.h"
#include <string>
using nccu::SemesterStateMachine;
using nccu::SemesterState;

// 在 Ch1 領到 TrueUmbrella 應觸發章節清關，推進到幕間市集。
TEST_CASE("chapter gate: TrueUmbrella claim in Ch1 -> Interlude") {
    EventBus::Instance().Clear();
    SemesterStateMachine m; std::string name;
    nccu::WireStateTransitionSubscribers(EventBus::Instance(), m, name);
    REQUIRE(m.Current() == SemesterState::Chapter1_AddDrop);
    EventBus::Instance().Publish(Event{ EventType::UmbrellaClaimed, "TrueUmbrella" });
    CHECK(m.Current() == SemesterState::Interlude_Market);
    EventBus::Instance().Clear();
}

// 領到非 TrueUmbrella 的雨傘不得推進章節，避免誤判清關條件。
TEST_CASE("chapter gate: a non-True umbrella does NOT advance Ch1") {
    EventBus::Instance().Clear();
    SemesterStateMachine m; std::string name;
    nccu::WireStateTransitionSubscribers(EventBus::Instance(), m, name);
    EventBus::Instance().Publish(Event{ EventType::UmbrellaClaimed, "FragileUmbrella" });
    CHECK(m.Current() == SemesterState::Chapter1_AddDrop);
    EventBus::Instance().Clear();
}

// 不在 Ch1 時領到 TrueUmbrella 不應改變狀態（閘門僅由 Ch1 觸發）。
TEST_CASE("chapter gate: TrueUmbrella claimed when not in Ch1 -> no bounce") {
    EventBus::Instance().Clear();
    SemesterStateMachine m; std::string name;
    nccu::WireStateTransitionSubscribers(EventBus::Instance(), m, name);
    m.Transition(SemesterState::Interlude_Market);
    EventBus::Instance().Publish(Event{ EventType::UmbrellaClaimed, "TrueUmbrella" });
    CHECK(m.Current() == SemesterState::Interlude_Market);
    EventBus::Instance().Clear();
}

// 進入建築只更新建築名稱，不應再連帶跳章（修正過去誤跳章節的行為）。
TEST_CASE("chapter gate: entering a building no longer jumps chapter") {
    EventBus::Instance().Clear();
    SemesterStateMachine m; std::string name;
    nccu::WireStateTransitionSubscribers(EventBus::Instance(), m, name);
    EventBus::Instance().Publish(Event{ EventType::EnteredBuilding, "正門" });
    CHECK(name == "正門");
    CHECK(m.Current() == SemesterState::Chapter1_AddDrop);   // 不應發生假跳章
    EventBus::Instance().Clear();
}
