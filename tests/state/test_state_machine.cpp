/**
 * @file test_state_machine.cpp
 * @brief 驗證 SemesterStateMachine 的初始狀態、狀態轉移、各狀態的顯示名稱，
 *        以及 Update 在任何狀態（含結局狀態）下都安全。
 */
#include "doctest/doctest.h"
#include "game/state/SemesterStateMachine.h"

using nccu::SemesterState;
using nccu::SemesterStateMachine;

// 狀態機建立後預設停在第一章。
TEST_CASE("SemesterStateMachine 建立後預設停在 Chapter1_AddDrop") {
    SemesterStateMachine m;
    CHECK(m.Current() == SemesterState::Chapter1_AddDrop);
    CHECK(m.CurrentName() == "第一章 加退選");
}

// Transition 會切換到指定列舉狀態，並更新對應的顯示名稱。
TEST_CASE("SemesterStateMachine：Transition 會切換到指定列舉狀態並更新名稱") {
    SemesterStateMachine m;
    m.Transition(SemesterState::Interlude_Market);
    CHECK(m.Current() == SemesterState::Interlude_Market);
    CHECK(m.CurrentName() == "幕間 市集");   // 幕間本身就是市集場景

    m.Transition(SemesterState::Chapter4_Finals);
    CHECK(m.Current() == SemesterState::Chapter4_Finals);
    CHECK(m.CurrentName() == "第四章 期末");
}

// 轉移到任一結局狀態後，CurrentName 會回報對應的結局名稱。
TEST_CASE("SemesterStateMachine：轉移到結局狀態後 CurrentName 回報結局名稱") {
    SemesterStateMachine m;
    m.Transition(SemesterState::Ending_A);
    CHECK(m.Current() == SemesterState::Ending_A);
    CHECK(m.CurrentName() == "結局 A");

    m.Transition(SemesterState::Ending_B);
    CHECK(m.CurrentName() == "結局 B");

    // 第四種結局 D 同樣是合法的終點狀態，且有自己的名稱。
    m.Transition(SemesterState::Ending_D);
    CHECK(m.Current() == SemesterState::Ending_D);
    CHECK(m.CurrentName() == "結局 D");

    m.Transition(SemesterState::Ending_C);
    CHECK(m.CurrentName() == "結局 C");
}

// Update 在任何狀態下呼叫都安全，即使結局狀態下內部 state_ 為空。
TEST_CASE("SemesterStateMachine::Update 在任何狀態下呼叫都安全") {
    SemesterStateMachine m;
    m.Update(0.016f);
    m.Transition(SemesterState::Ending_A);
    m.Update(0.016f); // 即使 state_ 為 null 也安全
}
