#include "game/state/EndingGate.h"
#include "game/quest/Flags.h"
#include "game/state/ChapterToast.h"
#include "engine/events/EventBus.h"
#include "game/entities/Player.h"
#include "game/state/SemesterStateMachine.h"
#include "game/state/SemesterState.h"
#include "game/dialog/DialogState.h"

namespace nccu {

// S5e-2b: the three endings resolve here, ALL state-guarded to
// Chapter4_Finals (the old Ch1+Flag_BoughtUglyUmbrella→Ending_C
// sibling-if is removed — the Ch1 阿姨 buy in chapter1.md (c) is now a
// pure narrative seed with NO tracked flag; the real Ending-C trigger
// is the Ch4 集英樓 Vendor purchase, which sets Flag_BoughtUglyUmbrella.
// (Cycle-8 audit F1 — see BUGLEDGER — removed the previous inert
// KnowsUgly seed annotation per the B3 precedent.)
// Precedence A → B → D → C (G1): the honest high-karma path wins, then
// the cursed/fallen path, then the bittersweet 風雨同行 (體諒 without the
// full A), then the buy-out / 平穩 default. Each closes any stale dialog
// and returns so only one fires per poll.
void CheckEndingGates(EventBus& bus, Player& player,
                      SemesterStateMachine& semester, DialogState& dialog) {
    if (semester.Current() != SemesterState::Chapter4_Finals) return;

    // G2 — endings stop being abrupt. The gate must NOT resolve while a
    // conversation / inner-monologue narration is on screen: a closing
    // beat shown the same frame the deciding flag is set (the 助教 finale
    // nextLines, or the buy/grab/claim 自白 opened by
    // TryOpenEndingConfession) is READ FIRST, then the ending transitions
    // on a later poll once the box has CLOSED. Without this, the choice
    // confirm set the flag and CheckEndingGates fired Transition()+Close()
    // the SAME frame, discarding the nextLines the player never got to see
    // (the owner's 「不要突然按下選項後跳結局」). The deciding flags are all
    // persistent, so deferring only delays — never drops — the resolution;
    // GameController re-polls this every non-dialog frame (sibling of the
    // CheckChapterGates poll), so it fires the instant the narration closes.
    if (dialog.Active()) return;

    // Ending A — karma > 80 AND holding the (Ch4-reclaimed) TrueUmbrella
    // AND chose 體諒 with 助教 (Flag_ConsoledTA, set by the S5e-2d
    // choice). Flag_HasTrueUmbrella is TrueUmbrella-specific and was
    // cleared on Ch4 entry, so it means exactly "re-claimed in Ch4".
    if (player.GetKarma() > 80 &&
        player.HasFlag(kFlagHasTrueUmbrella) &&
        player.HasFlag(kFlagConsoledTA)) {
        semester.Transition(SemesterState::Ending_A);
        // H2: even the terminal screen flashes a toast first — without it
        // the FSM hop into Ending_A was indistinguishable in state.jsonl
        // from a frame where nothing happened (cycle9 H2 evidence).
        PublishChapterTransitionToast(bus, SemesterState::Ending_A);
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
    const bool coldFinale = player.HasFlag(kFlagTaFinaleChoiceMade) &&
                            !player.HasFlag(kFlagConsoledTA);
    if (player.HasFlag(kFlagTookCursedUmbrella) ||
        player.GetKarma() < 0 || coldFinale) {
        semester.Transition(SemesterState::Ending_B);
        PublishChapterTransitionToast(bus, SemesterState::Ending_B);
        dialog.Close();
        return;
    }

    // Ending D (G1) — 風雨同行. Reaching this branch means the player chose
    // 體諒 (Flag_ConsoledTA) but did NOT clear Ending A (precedence A above
    // already consumed karma>80 + 持真傘 + 體諒) and did NOT fall to Ending
    // B (cursed / karma<0 / cold finale all rejected above — and 體諒
    // implies !coldFinale). So Flag_ConsoledTA alone here ⇒ a kind heart
    // with karma in [0,80]: a bittersweet, NOT-bad ending. This is the
    // owner's resolution of the open karma question — 體諒+karma≤80 now
    // lands D (the 破傘 / FragileBroken outcome), no longer the C 破財消災
    // default. Placed BEFORE C so 體諒-but-not-perfect prefers the warmer
    // D over the buy-out C; a 體諒 player who ALSO bought the ugly umbrella
    // still gets the more meaningful D (the moral choice outranks a
    // shopping decision). EndingSummary feeds the UI the 破傘 glyph
    // (EndingView endingUmbrellaLook D → FragileBroken).
    if (player.HasFlag(kFlagConsoledTA)) {
        semester.Transition(SemesterState::Ending_D);
        PublishChapterTransitionToast(bus, SemesterState::Ending_D);
        dialog.Close();
        return;
    }

    // Ending C — bought the 超醜螢光綠雨傘 at the Ch4 集英樓便利商店
    // (Vendor sets Flag_BoughtUglyUmbrella, S5e-2b): the GDD-designated
    // 破財消災 "Normal Ending". The Flag_TaFinaleChoiceMade disjunct is
    // retained as the belt-and-braces TOTAL default (it is now logically
    // pre-empted by the branches above — 質問 → coldFinale → B, 體諒 →
    // A/D — but kept so the gate provably resolves for ANY post-finale
    // flag combination, present or future, and can NEVER dead-end in Ch4;
    // CLAUDE.md §5 winnability). With the 4-ending tree the gate is TOTAL
    // once Flag_TaFinaleChoiceMade is set: exactly one of A/B/D fires for
    // a finale path, and C catches the buy-out / any residual. Strictly
    // gated so pre-finale Ch4 free-roam (explore, take cursed, buy the
    // ugly umbrella) is byte-unchanged.
    if (player.HasFlag(kFlagBoughtUglyUmbrella) ||
        player.HasFlag(kFlagTaFinaleChoiceMade)) {
        semester.Transition(SemesterState::Ending_C);
        PublishChapterTransitionToast(bus, SemesterState::Ending_C);
        dialog.Close();
        return;
    }
}

} // namespace nccu
