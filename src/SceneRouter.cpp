#include "SceneRouter.h"
#include "ChapterToast.h"
#include "EventBus.h"
#include "InterludeExit.h"
#include "Player.h"
#include "SemesterState.h"
#include "World.h"

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

    // Ch4 entry (chapter4.md L6「傘再度失蹤」): the player walks out of
    // 集英樓 with no umbrella. Reset both the generic HasUmbrella bool
    // AND the TrueUmbrella-specific marker, so Ending A's
    // 持-TrueUmbrella condition only holds if the player RE-claims the
    // Ch4 TrueUmbrella (not a leftover from Ch1/Ch3 nor a stray ctor
    // umbrella).
    if (cur == SemesterState::Chapter4_Finals) {
        if (Player* ip = world.GetPlayer()) {
            ip->SetHasUmbrella(false);
            ip->ClearFlag("Flag_HasTrueUmbrella");
        }
    }

    lastRosterState_ = cur;
}

} // namespace nccu
