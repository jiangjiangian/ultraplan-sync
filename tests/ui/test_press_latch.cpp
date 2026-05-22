// Regression for the "single keypress leaks across blocking screens" bug:
// the interactive title / character-select / help screens share the global
// raylib key state, and the "pressed" edge survives until the next
// EndDrawing poll. When one screen returned on Enter and the next began
// before that poll, the same Enter fired twice — 遊戲說明 opened then shut
// instantly, and 開始遊戲 skipped straight past character select.
//
// PressLatch (include/ui/PressLatch.h) fixes it by ignoring a press until
// the key has been seen released since the latch armed. These cases pin
// that contract; they are pure logic (no window / GL) so they run headless.
//
// Revert-verify: drop the `if (!down) armed_ = true;` line (or have Fired
// ignore `down`) and the inherited-press cases below start firing.

#include "doctest/doctest.h"
#include "ui/PressLatch.h"

using nccu::PressLatch;

TEST_CASE("PressLatch suppresses a press inherited held from a prior screen") {
    PressLatch l;
    // Key already down when the latch is created (the Enter carried over
    // from the previous screen). It must NOT fire, no matter how long it
    // stays held.
    CHECK_FALSE(l.Fired(/*down=*/true, /*pressed=*/true));   // entry frame
    CHECK_FALSE(l.Fired(true, false));                       // still held
    CHECK_FALSE(l.Fired(true, false));

    // Release arms the latch; the next fresh press fires exactly once and
    // does not auto-repeat while held.
    CHECK_FALSE(l.Fired(false, false));                      // released → armed
    CHECK(l.Fired(true, true));                              // fresh press fires
    CHECK_FALSE(l.Fired(true, false));                       // held, no repeat
    CHECK_FALSE(l.Fired(true, true));                        // no double-fire
}

TEST_CASE("PressLatch fires on the first press when the key starts released") {
    PressLatch l;
    CHECK_FALSE(l.Fired(false, false));   // frame 1: key up → arms
    CHECK(l.Fired(true, true));           // first real press fires
}

TEST_CASE("PressLatch models the title -> 遊戲說明 -> title round trip") {
    // One physical Enter must open help and NOT also dismiss it; and the
    // held dismiss-Enter must not re-open help back at the title.
    PressLatch titleConfirm;
    CHECK_FALSE(titleConfirm.Fired(false, false));  // arrive, Enter up → armed
    CHECK(titleConfirm.Fired(true, true));          // Enter on 遊戲說明 → open

    // Help page latch: the opening Enter is still down on entry.
    PressLatch helpDismiss;
    CHECK_FALSE(helpDismiss.Fired(true, true));     // entry frame: suppressed
    CHECK_FALSE(helpDismiss.Fired(true, false));    // still holding from title
    CHECK_FALSE(helpDismiss.Fired(false, false));   // released → armed
    CHECK(helpDismiss.Fired(true, true));           // fresh Enter → dismiss

    // Back at the title with the dismiss-Enter still held: must NOT fire
    // (titleConfirm already fired and stays disarmed until release).
    CHECK_FALSE(titleConfirm.Fired(true, true));    // stale dismiss edge
    CHECK_FALSE(titleConfirm.Fired(true, false));
    CHECK_FALSE(titleConfirm.Fired(false, false));  // released → re-armed
    CHECK(titleConfirm.Fired(true, true));          // a new press works again
}

TEST_CASE("PressLatch models title 開始遊戲 -> character-select handoff") {
    // The Enter that confirmed 開始遊戲 is still held when character select
    // begins; its own latch must swallow that press so persona 0 is not
    // auto-confirmed on frame 1.
    PressLatch selectConfirm;
    CHECK_FALSE(selectConfirm.Fired(true, true));   // inherited Enter: ignored
    CHECK_FALSE(selectConfirm.Fired(true, false));  // navigating, still held
    CHECK_FALSE(selectConfirm.Fired(false, false)); // released → armed
    CHECK(selectConfirm.Fired(true, true));         // player confirms a persona
}
