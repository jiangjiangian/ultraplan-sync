#include "game/controller/SceneRouter.h"
#include "game/quest/Flags.h"
#include "game/state/ChapterToast.h"
#include "engine/events/EventBus.h"
#include "game/state/InterludeExit.h"
#include "game/entities/Player.h"
#include "game/state/SemesterState.h"
#include "game/world/World.h"

namespace nccu {

void SceneRouter::SettleRoster(World& world) {
    // L8 fix (cycle9f §G.1): roster swap ONLY, no observable side
    // effects. Runs at end-of-Update so the View paints — and
    // state.jsonl records — npcs[] from the NEW chapter on the
    // transition frame instead of one frame late. Pre-Cycle 10 the
    // entire respawn + side-effect block lived at top-of-Update; the
    // view-only-affected half (chapter NPC list) is hoisted forward
    // here so the visible lag closes without touching the harness's
    // {player.pos, consumables, flags, events} observation. The other
    // half stays at top-of-Update (SettleSideEffects below), so the
    // 3-ending state.jsonl stream is byte-identical to baseline
    // EXCEPT for the npcs[] field on the 7 transition frames per
    // ending run — the documented, intentional byte change.
    const SemesterState cur = world.Semester().Current();
    if (cur == lastRosterRespawnState_) return;

    // World owns the actual remove/spawn pass via a single deferred
    // sweep — never mid-iteration, never reorders element 0 (Player).
    // Pure data mutation, no event publish, no input dependency.
    world.RespawnChapterRoster(cur);

    lastRosterRespawnState_ = cur;
}

void SceneRouter::SettleSideEffects(World& world) {
    // Side-effect half of the transition observer. Runs at TOP of
    // Update — exactly when the pre-Cycle 10 inline block fired — so
    // the harness sees the same observable {player.pos, consumables,
    // flags, events} timeline as before. The L8 fix above already
    // closed the npcs[] visible lag; this entry point keeps the rest
    // of the timeline byte-identical.
    const SemesterState cur = world.Semester().Current();
    if (cur == lastRosterState_) return;

    // Idempotency belt-and-braces: if the roster wasn't already
    // respawned for this state (the end-of-Update SettleRoster was
    // skipped for any reason), do it now so SpawnChapterNpcs is not
    // duplicated. RespawnChapterRoster is itself idempotent, but
    // calling it twice does redundant work — the cursor avoids that.
    if (cur != lastRosterRespawnState_) {
        world.RespawnChapterRoster(cur);
        lastRosterRespawnState_ = cur;
    }

    // ----- Per-destination side effects (the harness-observable half) -----

    // Arriving at the market: place the player at its entrance, well
    // north of the south exit band. Without this a chapter that ended
    // in the south would leave the player already inside the exit
    // zone, instantly bouncing them straight back out (a skipped
    // market). Chapter entry points are an S5c/d/e concern; the
    // Interlude is the only state S5b-2 owns.
    if (cur == SemesterState::Interlude_Market) {
        if (Player* ip = world.GetPlayer()) {
            ip->SetPosition(nccu::kInterludeEntry);
            // S5b-4 "消耗品當章用完": re-entering the market wipes the
            // consumable inventory, so what was bought for one chapter
            // can't be hoarded across the market boundary into the next
            // — every market visit is a fresh "buy for the chapter
            // ahead" decision (the loop's tension).
            ip->ClearConsumables();
        }
        // H3 (cycle9): the previous chapter's clear toast lands the same
        // frame as the FSM hop; this hint overwrites it ~1 frame later
        // so the player sees BOTH (the snap, then the direction) on the
        // same arrival. Reset the south-band latch so the exit toast
        // fires once per visit (and again on re-entry, never twice in
        // a row).
        EventBus::Instance().Publish(
            Event{EventType::ShowMessage, kInterludeArrivalHint});
        interludeExitZoneLatched_ = false;
    }

    // B4 — the per-chapter「傘又掉了」card is now MECHANICALLY TRUE. The
    // held umbrella was previously cleared ONLY on Ch4 entry, so a傘 claimed
    // in Ch1 (or borrowed in Ch2) persisted into Ch2/Ch3 — the View showed
    // the「傘又掉了」card while the bag still listed the umbrella (the bug:
    // 真傘 still in the Ch2 bag). Each new chapter starts fresh: the player
    // walks in umbrella-less and that chapter re-provides one (Ch2 the 管理員
    // loaner, Ch3 the reclaimed真傘, Ch4 the finale/reclaim), so arriving
    // empty-handed is by design (re-confirmed: every chapter has its own
    // umbrella path — winnable).
    //
    // SetHasUmbrella(false) ALSO empties the held-umbrella slot (Player.h),
    // so the bag's umbrella row disappears — the card and the bag finally
    // agree. Clearing Flag_HasTrueUmbrella keeps Ending A's 持-TrueUmbrella
    // condition meaning exactly "re-claimed THIS run's Ch4 TrueUmbrella"
    // (EndingGate.cpp): Ch3's beClaimed re-sets it, Ch4 entry clears it
    // again, the Ch4 reclaim/finale re-sets it — and it is consequential
    // ONLY in Chapter4_Finals (CheckEndingGates early-returns elsewhere), so
    // clearing it on Ch2/Ch3 entry is safe. Entry-only (the cur !=
    // lastRosterState_ guard at the top of SettleSideEffects fires this once
    // per chapter entry). Money + Flag_FoundForm + this chapter's freshly-
    // acquired items are the only survivors across the Interlude.
    if (cur == SemesterState::Chapter2_Midterms ||
        cur == SemesterState::Chapter3_SportsDay ||
        cur == SemesterState::Chapter4_Finals) {
        if (Player* ip = world.GetPlayer()) {
            ip->SetHasUmbrella(false);
            ip->ClearFlag(kFlagHasTrueUmbrella);
            // P2 cursed-taint decay: at every numbered-chapter entry, bleed
            // -5 * cursedTaint_ from karma. A taint-0 run skips the AddKarma
            // call entirely (no KarmaChanged published) so non-cursed playtests
            // stay byte-identical to the oracle; a taint=1 run loses -5 here on
            // each of Ch2/Ch3/Ch4 entry (= -15 total over a clean Ch1 cursed
            // pickup); a taint=2 run doubles that to -10/transition, etc.
            ip->ApplyCursedTaintDecay();
        }
    }

    lastRosterState_ = cur;
}

} // namespace nccu
