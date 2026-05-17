#ifndef EVENT_WIRING_H_
#define EVENT_WIRING_H_
#include "EventBus.h"
#include "SemesterStateMachine.h"
#include <iostream>
#include <string>

namespace nccu {

// Wires the cout-only log subscribers (ShowMessage, UmbrellaClaimed).
// Pure I/O — owns no game state.
inline void WireLoggingSubscribers(EventBus& bus) {
    bus.Subscribe(EventType::ShowMessage,
            [](const Event& e) { std::cout << "[UI] " << e.text << '\n'; })
       .Subscribe(EventType::UmbrellaClaimed,
            [](const Event& e) { std::cout << "[Game] Claimed: " << e.text << '\n'; });
}

// Wires the state subscribers: the EnteredBuilding tracker that feeds
// the current-building HUD label, and the real Chapter 1 gate that
// advances the semester when the TrueUmbrella is claimed. Captures
// references to caller-owned state — caller must outlive the
// subscription, OR call EventBus::Clear() before those refs go out of
// scope (main.cpp does the latter just before return 0).
inline void WireStateTransitionSubscribers(
    EventBus&                                                  bus,
    SemesterStateMachine&                                      semester,
    std::string&                                               currentBuildingName)
{
    bus.Subscribe(EventType::EnteredBuilding,
        [&currentBuildingName](const Event& e) {
            currentBuildingName = e.text;
            std::cout << "[Game] Entered: " << e.text << '\n';
        });

    // Real Ch1 chapter gate: claiming the TrueUmbrella in Chapter 1
    // clears the chapter and advances to the Interlude. This is the
    // narrower of chapter1.md's two clear conditions ("TrueUmbrella");
    // the broader "持有任意傘種離開集英樓" location path is deferred to
    // Phase 2 alongside ending wiring (intentional, not a bug). Extend
    // future gates by adding sibling ifs of this exact shape — do not
    // generalise yet.
    bus.Subscribe(EventType::UmbrellaClaimed,
        [&semester](const Event& e) {
            if (e.text == "TrueUmbrella" &&
                semester.Current() == SemesterState::Chapter1_AddDrop) {
                // Seed where the first market returns to (Ch2) before
                // entering it — the InterludeMarket state object is
                // recreated each Transition, so returnTo lives on the
                // machine. CheckChapterGates reads it on Interlude exit;
                // the later Ch2/Ch3 gates re-seed it for markets 2 & 3.
                semester.SetInterludeReturnTo(SemesterState::Chapter2_Midterms);
                semester.Transition(SemesterState::Interlude_Market);
            }
        });
}

// Convenience aggregator — preserves the original single-call entry
// point so existing call sites in main.cpp do not need to change.
inline void WireDefaultSubscribers(
    EventBus&                                                  bus,
    SemesterStateMachine&                                      semester,
    std::string&                                               currentBuildingName)
{
    WireLoggingSubscribers(bus);
    WireStateTransitionSubscribers(bus, semester, currentBuildingName);
}

} // namespace nccu

#endif // EVENT_WIRING_H_
