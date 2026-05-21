#include "SceneRouter.h"
#include "ChapterToast.h"
#include "EventBus.h"
#include "InterludeExit.h"
#include "Player.h"
#include "SemesterState.h"
#include "World.h"

namespace nccu {

void SceneRouter::Settle(World& world) {
    const SemesterState cur = world.Semester().Current();
    if (cur == lastRosterState_) return;

    // ----- Roster swap -----
    // Make the chapter NPCs follow the FSM. World owns the actual
    // remove/spawn pass via a single deferred sweep — never mid-
    // iteration, never reorders element 0 (Player). Pure data
    // mutation, no event publish, no input dependency.
    world.RespawnChapterRoster(cur);

    // ----- Per-destination side effects -----

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
