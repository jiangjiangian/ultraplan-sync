#ifndef SCENE_ROUTER_H_
#define SCENE_ROUTER_H_
#include "SemesterState.h"

namespace nccu {

class World;

// Cycle 10.P0a (awsome_cpp.md §6): extracts the chapter/interlude/
// ending transition observer out of GameController. Three
// responsibilities, one focus — "make the World follow the FSM":
//
//   1. Detect a SemesterState change vs the last-observed value.
//   2. Apply the per-destination side effects (NPC roster respawn,
//      Interlude entry repositioning + consumable wipe, Ch4 umbrella
//      reset, exit-latch reset, the Interlude arrival ShowMessage).
//   3. Stamp the lastRosterState_ cursor so a re-call is a no-op.
//
// Settle is called once per frame at the top of GameController::
// Update() — identical placement to the pre-Cycle-10 inline block, so
// the 3-ending observable timeline (state.jsonl) is byte-identical to
// baseline. The L8 entry (cycle9f §G.1) about the 1-frame npcs[]
// visual lag is left OPEN here; closing it without changing the
// observable timeline needs a second-pass split into roster + side-
// effects halves, deferred to Cycle 10.P0b.
//
// Owns no World state directly — it carries one `SemesterState
// lastRosterState_` cursor and one `bool interludeExitZoneLatched_`,
// both moved verbatim from the previous GameController. Mutates the
// World through its existing public API (RespawnChapterRoster,
// SetPosition, ClearConsumables, ClearFlag, SetHasUmbrella) so MVC
// purity is preserved — World stays pure data, no raylib here.
class SceneRouter {
public:
    // Initial cursor: the SemesterState the World was constructed with.
    // Caller passes World::Semester().Current() once at GameController
    // ctor time. Same shape as the previous `lastRosterState_(...)`.
    explicit SceneRouter(SemesterState initial) noexcept
        : lastRosterState_(initial) {}

    SceneRouter(const SceneRouter&)            = delete;
    SceneRouter& operator=(const SceneRouter&) = delete;

    // Top-of-Update observer. Detects + applies any FSM change since
    // the last Settle() call (roster swap + per-destination side
    // effects + cursor stamp). Idempotent: no-op when the World's
    // current semester equals the cached lastRosterState_.
    void Settle(World& world);

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

private:
    SemesterState lastRosterState_;
    // H3 (cycle9): once-per-visit latch for the Interlude exit-zone
    // toast. Reset to false in the Interlude-arrival branch of
    // Settle() so a re-visit reissues the cue exactly once. Same
    // semantics as the previous GameController::interludeExitZoneLatched_.
    bool          interludeExitZoneLatched_ = false;
};

} // namespace nccu

#endif // SCENE_ROUTER_H_
