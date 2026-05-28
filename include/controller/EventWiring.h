#ifndef EVENT_WIRING_H_
#define EVENT_WIRING_H_
#include "ui/ChapterToast.h"
#include "engine/events/EventBus.h"
#include "state/SemesterStateMachine.h"
#include "world/World.h"
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
                // H2: cycle9 — the Ch1 -> Interlude hop fires here, not
                // in ChapterGate (chapter1 has no Flag_ClearChapter1 stub).
                // Without the toast the only visible change in state.jsonl
                // was the umbrella claim text, masking the chapter snap.
                nccu::PublishChapterTransitionToast(
                    SemesterState::Interlude_Market);
            }
            // S5d-2 Ch3 clear: the 啦啦隊's TrueUmbrella, reclaimed from
            // the 體育館後台道具箱. Same shape as the Ch1 sibling-if
            // above (Ch1-isomorphic physical retrieval) — third market
            // returns to Ch4. Ch2's clear is the 喚醒 + LiftChapter2
            // Clear path instead (a social exchange, no umbrella pickup).
            // ChapterGate.cpp's Flag_Ch3Cleared sibling-if stays as the
            // test_chapter_spine stub; the real in-game path is this.
            if (e.text == "TrueUmbrella" &&
                semester.Current() == SemesterState::Chapter3_SportsDay) {
                semester.SetInterludeReturnTo(SemesterState::Chapter4_Finals);
                semester.Transition(SemesterState::Interlude_Market);
                nccu::PublishChapterTransitionToast(
                    SemesterState::Interlude_Market);
            }
        });
}

// Feeds the transient on-screen banner: every EventType::ShowMessage
// (quest cues, 喚醒提示, 章節清關旁白, ripple reactions, vendor purchase
// text) is mirrored into caller-owned World so the View can render the
// latest one as a fading toast. WireLoggingSubscribers still logs the
// same event to cout — this is an ADDITIONAL subscriber, not a
// replacement. Captures `world` by reference exactly like
// WireStateTransitionSubscribers captures currentBuildingName: the
// caller (GameController) must outlive the subscription OR call
// EventBus::Clear() first — GameController's dtor does the latter.
//
// Cycle 9.G: the event's `slot` field routes the text to the Top or
// Bottom HUD channel on the World (see HudSlot.h). Default Bottom for
// every pre-9.G publisher (handler bodies don't have to change), Top
// only for the three ChapterToast / EndingGate sites that explicitly
// opt in. Same-frame Top + Bottom publishes therefore coexist instead
// of clobbering each other.
inline void WireHudMessageSubscriber(EventBus& bus, World& world) {
    bus.Subscribe(EventType::ShowMessage,
        [&world](const Event& e) { world.SetHudMessage(e.slot, e.text); });
}

// Cycle 9.B H5: turn the previously-dead KarmaChanged channel into a
// visible toast. Player::AddKarma publishes KarmaChanged with the
// signed delta as text ("+5", "-3"); this subscriber re-publishes it
// as a ShowMessage prefixed with 業力, which the HUD subscriber above
// then mirrors into World::HudMessage(). Plan A from the diagnosis:
// karma toasts share the single HUD slot with chapter / narrative
// toasts, so a karma change that lands the same frame as a chapter
// transition is intentionally overwritten by the chapter banner
// (chapter feedback wins). A skipped 0-delta toast keeps the HUD
// clean when callers pass AddKarma(0) (rare, but cheap to filter).
inline void WireKarmaToastSubscriber(EventBus& bus) {
    bus.Subscribe(EventType::KarmaChanged,
        [](const Event& e) {
            if (e.text.empty() || e.text == "+0" || e.text == "-0")
                return;
            // "業力 " + signed-delta text fits well under the dialog
            // box's 28-cell budget (the 業力 ideographs are 2 cells
            // each, the delta is at most 5 ASCII chars including the
            // sign for ±100 — well under 28).
            EventBus::Instance().Publish(
                Event{EventType::ShowMessage, "業力 " + e.text});
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
