#include "quest/ChapterGate.h"
#include "quest/Flags.h"
#include "ui/ChapterToast.h"
#include "engine/events/EventBus.h"
#include "entities/Player.h"
#include "state/SemesterStateMachine.h"
#include "state/SemesterState.h"
#include "dialog/DialogState.h"

namespace nccu {

void CheckChapterGates(EventBus& bus, Player& player,
                       SemesterStateMachine& semester, DialogState& dialog) {
    // Chapter 2 cleared -> back to the market, which then returns to Ch3.
    // Flag_Ch2Cleared is a deliberate spine stub: it will be set by the
    // real Chapter 2 quest in S5c. Wiring the transition now makes the
    // full progression traversable / testable before that content exists.
    if (semester.Current() == SemesterState::Chapter2_Midterms &&
        player.HasFlag(kFlagCh2Cleared)) {
        semester.SetInterludeReturnTo(SemesterState::Chapter3_SportsDay);
        semester.Transition(SemesterState::Interlude_Market);
        // H2: announce the destination so the player sees the FSM move.
        // Publish AFTER Transition() so a subscriber that reacts to the
        // text (HUD mirror) reads the already-current state if it asks.
        PublishChapterTransitionToast(bus, SemesterState::Interlude_Market);
        dialog.Close();
    }

    // Chapter 3 cleared -> back to the market, which then returns to Ch4.
    // Flag_Ch3Cleared is the matching spine stub for S5d's Chapter 3 quest.
    if (semester.Current() == SemesterState::Chapter3_SportsDay &&
        player.HasFlag(kFlagCh3Cleared)) {
        semester.SetInterludeReturnTo(SemesterState::Chapter4_Finals);
        semester.Transition(SemesterState::Interlude_Market);
        PublishChapterTransitionToast(bus, SemesterState::Interlude_Market);
        dialog.Close();
    }

    // Leaving the Interlude returns to whichever chapter seeded returnTo
    // (Ch1's UmbrellaClaimed gate -> Ch2; the two ifs above -> Ch3 / Ch4).
    // Flag_LeaveInterlude is set by the 公告板 NPC in S5b-2; the spine test
    // sets it directly. Consume (clear) it before transitioning so a later
    // re-entry into the Interlude doesn't instantly exit again.
    if (semester.Current() == SemesterState::Interlude_Market &&
        player.HasFlag(kFlagLeaveInterlude)) {
        player.ClearFlag(kFlagLeaveInterlude);
        const SemesterState target = semester.InterludeReturnTo();
        semester.Transition(target);
        PublishChapterTransitionToast(bus, target);
        dialog.Close();
    }
}

} // namespace nccu
