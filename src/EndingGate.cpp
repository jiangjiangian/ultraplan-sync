#include "EndingGate.h"
#include "Player.h"
#include "SemesterStateMachine.h"
#include "SemesterState.h"
#include "DialogState.h"

namespace nccu {

// S5e-2b: the three endings resolve here, ALL state-guarded to
// Chapter4_Finals (the old Ch1+Flag_BoughtUglyUmbrella→Ending_C
// sibling-if is removed — per C.1 the Ch1 阿姨 buy now only seeds
// Flag_KnowsUglyUmbrella; the real purchase is the Ch4 集英樓 Vendor).
// Precedence A → B → C (plan §F.7): the honest high-karma path wins,
// then the cursed/fallen path, then the buy-out. Each closes any
// stale dialog and returns so only one fires per poll.
void CheckEndingGates(Player& player, SemesterStateMachine& semester,
                      DialogState& dialog) {
    if (semester.Current() != SemesterState::Chapter4_Finals) return;

    // Ending A — karma > 80 AND holding the (Ch4-reclaimed) TrueUmbrella
    // AND chose 體諒 with 助教 (Flag_ConsoledTA, set by the S5e-2d
    // choice). Flag_HasTrueUmbrella is TrueUmbrella-specific and was
    // cleared on Ch4 entry, so it means exactly "re-claimed in Ch4".
    if (player.GetKarma() > 80 &&
        player.HasFlag("Flag_HasTrueUmbrella") &&
        player.HasFlag("Flag_ConsoledTA")) {
        semester.Transition(SemesterState::Ending_A);
        dialog.Close();
        return;
    }

    // Ending B — the cursed-umbrella path (Ch1 seed persists) or karma
    // fallen below zero.
    if (player.HasFlag("Flag_TookCursedUmbrella") || player.GetKarma() < 0) {
        semester.Transition(SemesterState::Ending_B);
        dialog.Close();
        return;
    }

    // Ending C — bought the 超醜螢光綠雨傘 at the Ch4 集英樓便利商店
    // (Vendor sets Flag_BoughtUglyUmbrella, S5e-2b).
    if (player.HasFlag("Flag_BoughtUglyUmbrella")) {
        semester.Transition(SemesterState::Ending_C);
        dialog.Close();
        return;
    }
}

} // namespace nccu
