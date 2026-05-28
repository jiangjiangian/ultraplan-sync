#ifndef CHAPTER_TOAST_H_
#define CHAPTER_TOAST_H_
#include "engine/events/EventBus.h"
#include "state/SemesterState.h"
#include <string>

namespace nccu {

// H2 (Cycle 9): every chapter / interlude / ending transition publishes a
// short ShowMessage so the player gets visible feedback the moment the
// semester FSM advances. Before this, ChapterGate / EventWiring /
// EndingGate ran Transition() silently and only the destination's
// QuestObjective hinted at progress — the playtest log captured 5
// consecutive `Transition()`s with `events=[]`, so the player saw nothing
// but a snapped roster (BUGLEDGER cycle9 H2).
//
// Pure data: maps the destination SemesterState to its display text.
// Returned by value (not const-ref) so a future caller could decorate it
// without forcing a static lifetime on every variant. All strings stay
// well within the dialog-box 28-cell budget (CLAUDE.md §6) so the HUD
// banner and a future spill into the dialog system look identical.
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

// Publish the toast on the global EventBus. One call per Transition site
// (EventWiring Ch1/Ch3 -> Interlude, ChapterGate Ch2/Ch3/Interlude exit,
// EndingGate Ch4 -> Ending_A/B/C). Skip the publish on the rare branch
// that calls this with the SAME destination as the current state — the
// caller protects against a no-op transition. Empty toast text (should
// never happen post-switch coverage) becomes a no-op so a future enum
// addition cannot accidentally emit a blank banner.
//
// Cycle 9.G: chapter / ending transition toasts go on HudSlot::Top so
// the same-frame Bottom-slot publishes (TrueUmbrella pickup line, IL
// arrival hint, karma toasts) coexist with — instead of clobbering —
// the chapter banner. Before the split, the arrival hint published one
// frame after the chapter toast and overwrote the same single slot;
// the chapter line was visible for 0.02 s (cycle9f-post-iteration-
// diagnosis §B).
//
// Plan P2: `bus` is injected (was a header singleton call to
// EventBus::Instance()). Callers thread the bus from main.cpp →
// GameController member → the gate/router that decides the
// transition, so this header no longer carries an implicit dependency
// on the global Instance().
inline void PublishChapterTransitionToast(EventBus& bus, SemesterState target) {
    const std::string msg = ChapterTransitionToast(target);
    if (msg.empty()) return;
    bus.Publish(Event{EventType::ShowMessage, msg, nccu::HudSlot::Top});
}

// H3 (Cycle 9): the Interlude's south-band exit zone is a silent trigger
// (InterludeExit.h) — the player walked into it with zero feedback before
// the chapter snapped. These two text constants give the position-trigger
// the same visible heartbeat the new chapter toasts give the FSM events.
// First arrival into the Interlude itself is the H2 toast; this extra
// hint nudges the player toward the exit corridor without crowding the
// chapter-clear banner. The exit-zone confirmation fires once per visit
// (GameController latches it), so the toast log never spams.
inline constexpr const char* kInterludeArrivalHint =
    "市集中央。逛完後往南離開";
inline constexpr const char* kInterludeExitPrep =
    "準備離開市集";

// Once-per-visit latch helper for the south-band exit toast. Returns true
// (and publishes) on the first call after the latch has been reset to
// false; subsequent calls with `latched == true` are no-ops so the
// player wobbling across the y=1900 line never spams the HUD. Pure logic
// so the GameController integration and the regression test exercise the
// same code path. GameController resets the latch on Interlude entry; the
// caller is responsible for that lifecycle.
//
// Plan P2: `bus` is injected — see PublishChapterTransitionToast above.
inline bool MaybeAnnounceInterludeExit(EventBus& bus, bool& latched) {
    if (latched) return false;
    latched = true;
    bus.Publish(Event{EventType::ShowMessage, kInterludeExitPrep});
    return true;
}

} // namespace nccu

#endif // CHAPTER_TOAST_H_
