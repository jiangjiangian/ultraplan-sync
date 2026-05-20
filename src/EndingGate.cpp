#include "EndingGate.h"
#include "Player.h"
#include "SemesterStateMachine.h"
#include "SemesterState.h"
#include "DialogState.h"

namespace nccu {

// S5e-2b: the three endings resolve here, ALL state-guarded to
// Chapter4_Finals (the old Ch1+Flag_BoughtUglyUmbrella→Ending_C
// sibling-if is removed — the Ch1 阿姨 buy in chapter1.md (c) is now a
// pure narrative seed with NO tracked flag; the real Ending-C trigger
// is the Ch4 集英樓 Vendor purchase, which sets Flag_BoughtUglyUmbrella.
// (Cycle-8 audit F1 — see BUGLEDGER — removed the previous inert
// KnowsUgly seed annotation per the B3 precedent.)
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

    // Ending B — the cursed-umbrella path (Ch1 seed persists), karma
    // fallen below zero, OR the cold finale: the player reached the 助教
    // (d) 結算 and chose 「質問／強硬索回」 instead of 體諒
    // (Flag_TaFinaleChoiceMade set without Flag_ConsoledTA). Refusing
    // compassion at the climax — after a sleep-deprived, apologetic 助教
    // tried to make amends — IS thematically 「你成為了你曾經最討厭的那
    // 種人」 (GDD §陸 B 字卡). The GDD's literal B trigger
    // (Flag_TookCursedUmbrella || karma<0) is extended here per the
    // design-governance clause (CLAUDE.md §3); without this the cold
    // choice fell through ALL gates and, because the finale menu
    // self-locks via Flag_TaFinaleChoiceMade, soft-locked the game in
    // Ch4 forever (no ending reachable — a §5 red-line violation).
    const bool coldFinale = player.HasFlag("Flag_TaFinaleChoiceMade") &&
                            !player.HasFlag("Flag_ConsoledTA");
    if (player.HasFlag("Flag_TookCursedUmbrella") ||
        player.GetKarma() < 0 || coldFinale) {
        semester.Transition(SemesterState::Ending_B);
        dialog.Close();
        return;
    }

    // Ending C — bought the 超醜螢光綠雨傘 at the Ch4 集英樓便利商店
    // (Vendor sets Flag_BoughtUglyUmbrella, S5e-2b), OR — the
    // GDD-designated 破財消災 "Normal Ending" — the player reached the
    // 助教 finale and chose 體諒 but did NOT earn Ending A (karma ≤ 80
    // or not holding the Ch4-reclaimed TrueUmbrella). C is the explicit
    // "neither perfect nor evil" default (GDD §陸); making it the
    // guaranteed terminal once the finale choice is made closes the
    // last fall-through: after Flag_TaFinaleChoiceMade, CheckEndingGates
    // is now TOTAL — exactly one of A/B/C always fires, so the spine can
    // never dead-end in Ch4. Strictly gated on Flag_TaFinaleChoiceMade
    // so Ch4 free-roam BEFORE the finale (explore, take cursed, buy the
    // ugly umbrella) is byte-unchanged — only the post-finale resolution
    // gains a default.
    if (player.HasFlag("Flag_BoughtUglyUmbrella") ||
        player.HasFlag("Flag_TaFinaleChoiceMade")) {
        semester.Transition(SemesterState::Ending_C);
        dialog.Close();
        return;
    }
}

} // namespace nccu
