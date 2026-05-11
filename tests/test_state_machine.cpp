#include "doctest/doctest.h"
#include "SemesterStateMachine.h"

using nccu::SemesterState;
using nccu::SemesterStateMachine;

TEST_CASE("SemesterStateMachine starts in Chapter1_AddDrop") {
    SemesterStateMachine m;
    CHECK(m.Current() == SemesterState::Chapter1_AddDrop);
    CHECK(m.CurrentName() == "第一章 加退選");
}

TEST_CASE("SemesterStateMachine: Transition follows enum") {
    SemesterStateMachine m;
    m.Transition(SemesterState::Interlude_Market);
    CHECK(m.Current() == SemesterState::Interlude_Market);
    CHECK(m.CurrentName() == "幕間 福利社");

    m.Transition(SemesterState::Chapter4_Finals);
    CHECK(m.Current() == SemesterState::Chapter4_Finals);
    CHECK(m.CurrentName() == "第四章 期末");
}

TEST_CASE("SemesterStateMachine: endings drop state_ and report ending name") {
    SemesterStateMachine m;
    m.Transition(SemesterState::Ending_A);
    CHECK(m.Current() == SemesterState::Ending_A);
    CHECK(m.CurrentName() == "結局 A");

    m.Transition(SemesterState::Ending_B);
    CHECK(m.CurrentName() == "結局 B");

    m.Transition(SemesterState::Ending_C);
    CHECK(m.CurrentName() == "結局 C");
}

TEST_CASE("SemesterStateMachine::Update is safe at every state") {
    SemesterStateMachine m;
    m.Update(0.016f);
    m.Transition(SemesterState::Ending_A);
    m.Update(0.016f); // safe even when state_ is null
}
