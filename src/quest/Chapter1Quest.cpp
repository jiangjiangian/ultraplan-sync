#include "quest/Chapter1Quest.h"
#include "controller/EventBus.h"
#include "entities/Player.h"
#include "dialog/DialogState.h"
#include <string>

namespace nccu {

void TryReturnVictimUmbrella(Player& player, std::string_view npcId,
                             SemesterState state) {
    if (state != SemesterState::Chapter1_AddDrop) return;
    if (npcId != "victim") return;
    if (player.HasFlag("Flag_HasTrueUmbrella")) return;   // already granted

    if (!player.HasFlag("Flag_PromisedVictim")) {
        // The (a) plea / (b) 承諾 DialogChoice owns the promise beat; there
        // is nothing to return before the player has even agreed to help.
        // No-op so the opener routes to (a)/(b) untouched.
        return;
    }

    if (!player.HasFlag(kFlagHasVictimUmbrella)) {
        // Promised but empty-handed — nudge toward the findable umbrella the
        // 西裝學長 dropped near 集英樓 (the QuestFlagPickup). No state change.
        EventBus::Instance().Publish(Event{
            EventType::ShowMessage,
            std::string("「找到我的傘了嗎？先幫他找回那把傘吧——"
                        "聽說那個西裝學長往集英樓跑了。」")});
        return;
    }

    // GRANT — the reciprocity payoff (T2: now a real exchange SCENE that
    // plays BEFORE the chapter clears). The player carried the victim's
    // umbrella back; the 苦主 takes it, then reveals he ALSO found the
    // player's true umbrella and hands it over. This sets HasUmbrella +
    // Flag_HasTrueUmbrella (Ending A's precise condition, EndingGate.cpp)
    // — but it does NOT publish UmbrellaClaimed here. Pre-T2, publishing
    // UmbrellaClaimed inline drove the EventWiring Ch1 sibling-if →
    // Transition(Interlude) on the SAME frame, so the chapter snapped shut
    // BEFORE the (d) 重逢致謝 exchange dialogue could be read (the opener,
    // running right after this in GameController, saw state==Interlude and
    // never routed the victim to (d)).
    //
    // The fix mirrors Ch2's LiftChapter2Clear: grant the flags silently
    // here, let the opener route the victim to his (d) exchange recap
    // (ResolveOpenerSubState: Flag_HasTrueUmbrella → (d)), and defer the
    // UmbrellaClaimed publish to LiftChapter1Clear — which fires it only
    // AFTER the (d) dialogue has closed. So the player reads the exchange,
    // THEN Ch1 clears to the Interlude. The exchange's spoken lines live in
    // chapter1.md 苦主 (d); no inline ShowMessage is needed (it would be a
    // redundant one-line echo of the (d) scene).
    player.ClearFlag(kFlagHasVictimUmbrella);   // 苦主 takes his傘 back
    player.SetHasUmbrella(true);
    player.SetFlag("Flag_HasTrueUmbrella");
}

void LiftChapter1Clear(Player& player, SemesterState state,
                       const DialogState& dialog) {
    if (state != SemesterState::Chapter1_AddDrop) return;
    if (!player.HasFlag("Flag_HasTrueUmbrella")) return;  // grant not done
    if (dialog.Active()) return;                          // (d) still on screen
    if (player.HasFlag(kFlagCh1ClearFired)) return;       // once
    // The (d) 重逢致謝 exchange has played and closed — NOW clear Ch1.
    // Publish in TrueUmbrella::beClaimed's exact pair order — ShowMessage
    // FIRST, UmbrellaClaimed SECOND — so the EventWiring chapter-clear toast
    // wins the single Top HUD slot while the pickup line takes Bottom
    // (reversing the pair re-introduces the Cycle 9.A.2 regression pinned by
    // tests/quest/test_chapter_transitions.cpp). The UmbrellaClaimed(
    // "TrueUmbrella") drives Ch1→Interlude (returnTo Ch2) via the
    // EventWiring Ch1 sibling-if. The once-key makes this fire exactly once
    // even though it is polled every non-dialog frame.
    player.SetFlag(kFlagCh1ClearFired);
    EventBus::Instance().Publish(Event{
        EventType::ShowMessage,
        std::string("傘找回來了。雨還沒停，但你的心安定了一點。")});
    EventBus::Instance().Publish(Event{
        EventType::UmbrellaClaimed, std::string("TrueUmbrella")});
}

bool Ch1IndicatorVisible(std::string_view npcId, bool isQuestGiver,
                         const Player& player) {
    // G3: 苦主 → 西裝學長 → 苦主 sequence. Each spine NPC lights ONLY on its
    // own step so exactly one main `!` is visible at a time (out-of-order
    // contact is redirected by the E-interact hooks, not here).
    const bool grantDone   = player.HasFlag("Flag_HasTrueUmbrella");
    const bool promised    = player.HasFlag("Flag_PromisedVictim");
    const bool seniorChoice = player.HasFlag("Flag_SuitSeniorChoiceMade");

    if (npcId == "victim") {
        // Lit at step 1 (before the promise — give the lead) and again at
        // step 3 (after the 學長 choice — bring his umbrella back). Dark in
        // between (the player is off confronting the 學長) and after the
        // grant (the (d) reunion is then a recap, not an objective).
        if (grantDone) return false;                 // done → dark
        if (!promised) return true;                  // step 1: get the lead
        return seniorChoice;                         // step 3 once 學長 done
    }
    if (npcId == "suit_senior") {
        // Lit at step 2 ONLY: the player has the lead (promised) but has
        // not yet made the 學長 choice. Goes dark the instant the choice is
        // committed (Flag_SuitSeniorChoiceMade), handing the `!` back to
        // 苦主 for step 3. Keyed on npcId (not the roster bit) because the
        // 學長 ships isQuestGiver=false.
        return promised && !seniorChoice && !grantDone;
    }
    // Every other Ch1 npc keeps its roster bit (the 助教 申請書 errand /
    // 學霸 / 阿姨 are isQuestGiver=false → never a main `!`).
    return isQuestGiver;
}

} // namespace nccu
