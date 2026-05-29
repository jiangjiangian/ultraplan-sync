#include "doctest/doctest.h"
#include "game/controller/EventWiring.h"
#include "game/state/SemesterStateMachine.h"
#include "engine/events/EventBus.h"
#include <string>
using nccu::SemesterStateMachine;
using nccu::SemesterState;

TEST_CASE("chapter gate: TrueUmbrella claim in Ch1 -> Interlude") {
    EventBus::Instance().Clear();
    SemesterStateMachine m; std::string name;
    nccu::WireStateTransitionSubscribers(EventBus::Instance(), m, name);
    REQUIRE(m.Current() == SemesterState::Chapter1_AddDrop);
    EventBus::Instance().Publish(Event{ EventType::UmbrellaClaimed, "TrueUmbrella" });
    CHECK(m.Current() == SemesterState::Interlude_Market);
    EventBus::Instance().Clear();
}

TEST_CASE("chapter gate: a non-True umbrella does NOT advance Ch1") {
    EventBus::Instance().Clear();
    SemesterStateMachine m; std::string name;
    nccu::WireStateTransitionSubscribers(EventBus::Instance(), m, name);
    EventBus::Instance().Publish(Event{ EventType::UmbrellaClaimed, "FragileUmbrella" });
    CHECK(m.Current() == SemesterState::Chapter1_AddDrop);
    EventBus::Instance().Clear();
}

TEST_CASE("chapter gate: TrueUmbrella claimed when not in Ch1 -> no bounce") {
    EventBus::Instance().Clear();
    SemesterStateMachine m; std::string name;
    nccu::WireStateTransitionSubscribers(EventBus::Instance(), m, name);
    m.Transition(SemesterState::Interlude_Market);
    EventBus::Instance().Publish(Event{ EventType::UmbrellaClaimed, "TrueUmbrella" });
    CHECK(m.Current() == SemesterState::Interlude_Market);
    EventBus::Instance().Clear();
}

TEST_CASE("chapter gate: entering a building no longer jumps chapter") {
    EventBus::Instance().Clear();
    SemesterStateMachine m; std::string name;
    nccu::WireStateTransitionSubscribers(EventBus::Instance(), m, name);
    EventBus::Instance().Publish(Event{ EventType::EnteredBuilding, "正門" });
    CHECK(name == "正門");
    CHECK(m.Current() == SemesterState::Chapter1_AddDrop);   // no fake jump
    EventBus::Instance().Clear();
}
