#ifndef SCENE_ROUTER_H_
#define SCENE_ROUTER_H_
#include "SemesterState.h"

namespace nccu {

class World;

// Cycle 10.P0a (awsome_cpp.md §6) + 10.P0b (L8 fix): extracts the
// chapter/interlude/ending transition observer out of GameController.
// Three responsibilities, one focus — "make the World follow the FSM":
//
//   1. Detect a SemesterState change vs the last-observed value.
//   2. Apply the per-destination side effects (NPC roster respawn,
//      Interlude entry repositioning + consumable wipe, Ch4 umbrella
//      reset, exit-latch reset, the Interlude arrival ShowMessage).
//   3. Stamp the lastRosterState_ so the SAME frame's subsequent reads
//      (View / state.jsonl) see a coherent {semester, npcs[], player}.
//
// L8 (cycle9f §G.1, BUGLEDGER L8): pre-Cycle 10 the Controller
// observed the state change at the TOP of the next Update() — one
// frame after a CheckChapterGates / CheckEndingGates / EventWiring
// transition fired. Result: the View painted one frame with
// semester=NEW but npcs[]=OLD; state.jsonl recorded the same
// mismatch on every one of the 7 spine transitions.
//
// Settle is intentionally split into TWO entry points so the fix is
// surgically visual:
//
//   SettleRoster(World)
//     Roster swap ONLY (chapter NPCs follow the FSM). Called at the
//     END of Update() so the View paints — and state.jsonl records —
//     the new roster on the same frame the transition fires. No
//     player-position write, no consumable wipe, no event publish,
//     no flag mutation: those are the side-effects the harness's
//     read-only world snapshot uses to resolve the NEXT frame's plan
//     step, so changing them mid-frame would break the script (the
//     interact verb's "arrived" check reads player pos from this
//     snapshot — verified: a single-shot version that teleported on
//     the transition frame stalled ending A in Interlude #3).
//
//   SettleSideEffects(World)
//     The other half (player repositioning, ClearConsumables, Ch4
//     umbrella reset, exit-latch reset, the Interlude arrival
//     ShowMessage). Called at the TOP of Update() — exactly when the
//     pre-Cycle 10 inline block fired — so the harness sees the same
//     observable {player.pos, consumables, flags, events} timeline.
//
// Each call internally checks `cur == lastRosterState_` (or its own
// cursor) so calling either alone is safe. The combined effect (one
// tick later, the same observable result as before) keeps the 3-
// ending state.jsonl identical to baseline except for the npcs[]
// field on the 7 transition frames per run.
class SceneRouter {
public:
    // Initial cursor: the SemesterState the World was constructed with.
    // Caller passes World::Semester().Current() once at GameController
    // ctor time. Same shape as the previous `lastRosterState_(...)`.
    explicit SceneRouter(SemesterState initial) noexcept
        : lastRosterState_(initial),
          lastRosterRespawnState_(initial) {}

    SceneRouter(const SceneRouter&)            = delete;
    SceneRouter& operator=(const SceneRouter&) = delete;

    // L8 fix: roster swap ONLY. Called at end-of-Update so the same
    // frame the FSM transitions paints a coherent npcs[] list. Safe
    // to call when no transition happened (idempotent: tracks a
    // separate cursor — lastRosterRespawnState_ — so it can advance
    // before SettleSideEffects without one starving the other).
    void SettleRoster(World& world);

    // Side-effect half: player position + consumables + flags + arrival
    // hint + latch reset. Called at top-of-Update so the observable
    // timeline (harness state.jsonl) is unchanged from pre-Cycle 10.
    // Stamps lastRosterState_ so the NEXT transition's SettleRoster
    // can detect another change. If SettleRoster was somehow skipped
    // (e.g. a future code path), this defensively does the roster
    // respawn too — both halves run exactly once per transition.
    void SettleSideEffects(World& world);

    // Read-only latch peek. GameController forwards it to
    // MaybeAnnounceInterludeExit when the player crosses the south
    // band — kept here so all transition-related state lives in one
    // place. Pre-Cycle 10 this was an inline bool on the Controller.
    [[nodiscard]] bool& InterludeExitLatchMut() noexcept {
        return interludeExitZoneLatched_;
    }

    // Test inspection.
    [[nodiscard]] SemesterState LastRosterState() const noexcept {
        return lastRosterState_;
    }
    [[nodiscard]] SemesterState LastRosterRespawnState() const noexcept {
        return lastRosterRespawnState_;
    }

private:
    // Cursor for SettleSideEffects: the FSM state the last side-effect
    // pass was applied for. Advances only in SettleSideEffects.
    SemesterState lastRosterState_;
    // Cursor for SettleRoster: the FSM state the last roster respawn
    // ran for. Separate so SettleRoster can run BEFORE
    // SettleSideEffects without one starving the other (each is
    // idempotent under its own cursor).
    SemesterState lastRosterRespawnState_;
    // H3 (cycle9): once-per-visit latch for the Interlude exit-zone
    // toast. Reset to false in the Interlude-arrival branch of
    // SettleSideEffects so a re-visit reissues the cue exactly once.
    bool          interludeExitZoneLatched_ = false;
};

} // namespace nccu

#endif // SCENE_ROUTER_H_
