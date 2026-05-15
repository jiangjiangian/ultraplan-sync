#ifndef EVENT_WIRING_H_
#define EVENT_WIRING_H_
#include "EventBus.h"
#include "SemesterStateMachine.h"
#include <iostream>
#include <string>
#include <unordered_map>

namespace nccu {

// Wires the cout-only log subscribers (ShowMessage, UmbrellaClaimed).
// Pure I/O — owns no game state.
inline void WireLoggingSubscribers(EventBus& bus) {
    bus.Subscribe(EventType::ShowMessage,
            [](const Event& e) { std::cout << "[UI] " << e.text << '\n'; })
       .Subscribe(EventType::UmbrellaClaimed,
            [](const Event& e) { std::cout << "[Game] Claimed: " << e.text << '\n'; });
}

// Wires the EnteredBuilding subscriber that drives chapter transitions.
// Captures references to caller-owned state — caller must outlive the
// subscription, OR call EventBus::Clear() before those refs go out of
// scope (main.cpp does the latter just before return 0).
inline void WireStateTransitionSubscribers(
    EventBus&                                                  bus,
    SemesterStateMachine&                                      semester,
    std::string&                                               currentBuildingName,
    const std::unordered_map<std::string, SemesterState>&      enterTrigger)
{
    bus.Subscribe(EventType::EnteredBuilding,
        [&currentBuildingName, &semester, &enterTrigger](const Event& e) {
            currentBuildingName = e.text;
            std::cout << "[Game] Entered: " << e.text << '\n';
            if (auto it = enterTrigger.find(e.text); it != enterTrigger.end()) {
                semester.Transition(it->second);
            }
        });
}

// Convenience aggregator — preserves the original single-call entry
// point so existing call sites in main.cpp do not need to change.
inline void WireDefaultSubscribers(
    EventBus&                                                  bus,
    SemesterStateMachine&                                      semester,
    std::string&                                               currentBuildingName,
    const std::unordered_map<std::string, SemesterState>&      enterTrigger)
{
    WireLoggingSubscribers(bus);
    WireStateTransitionSubscribers(bus, semester, currentBuildingName, enterTrigger);
}

} // namespace nccu

#endif // EVENT_WIRING_H_
