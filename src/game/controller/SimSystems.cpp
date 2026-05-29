#include "game/controller/SimSystem.h"
#include "game/world/World.h"
#include "game/entities/Player.h"
#include "engine/core/Roles.h"
#include "game/controller/GameObjectQueries.h"
#include "game/world/Physics.h"
#include "game/gfx/Bounds.h"

namespace nccu {

// ── SurvivalSystem ───────────────────────────────────────────────────
// Verbatim extraction of GameController::Update's rain block. BUGLEDGER
// I8 / Cycle 4 / REQUIREMENT #5: the GDD rain-survival loop, live & lethal
// in EVERY chapter, ticked once per frame. THREE states:
//   INSIDE A BUILDING             -> DrainRain          (-10 u/s, full
//                                    recovery, the true haven)
//   OUTDOORS, HOLDING AN UMBRELLA -> ApplyRainSheltered (+1.5 u/s, lethal-
//                                    armed — an umbrella SLOWS the soak)
//   OUTDOORS, NO UMBRELLA         -> ApplyRain          (+5 u/s, lethal —
//                                    UNCHANGED; Ch1 is umbrella-less so Ch1
//                                    rain is byte-identical to before)
// Skipped only in the market interlude and endings (safe, non-gameplay
// states). CurrentBuildingName() is non-empty when the player center is in
// a building trigger zone (refreshed at the end of the previous frame; the
// one-frame latency on a <=10 u/s rate is imperceptible and deterministic).
void SurvivalSystem::Run(SimContext& ctx, float dt) {
    World& world = ctx.world;
    Player* player = world.GetPlayer();
    if (!player) return;
    const SemesterState ss = world.Semester().Current();
    const bool inEnding = ss == SemesterState::Ending_A ||
                          ss == SemesterState::Ending_B ||
                          ss == SemesterState::Ending_C;
    if (ss != SemesterState::Interlude_Market && !inEnding) {
        if (!world.CurrentBuildingName().empty())
            player->DrainRain(dt);                  // full recovery
        else if (player->HasUmbrella())
            player->ApplyRainSheltered(dt, /*lethal=*/true);
        else
            player->ApplyRain(dt, /*lethal=*/true); // full exposure
    }
}

// ── MovementSystem ───────────────────────────────────────────────────
// Capture the player's pre-tick top-left, then tick every IUpdatable.
// ForEachRole maps the role to AsUpdatable() at compile time and skips the
// rest (an umbrella / pickup that no longer carries an Update is simply not
// visited — identical to the old empty-no-op call). The captured position
// is handed to CollisionSystem via the context for the axis-separated
// resolve below.
void MovementSystem::Run(SimContext& ctx, float dt) {
    World& world = ctx.world;
    Player* player = world.GetPlayer();
    ctx.prevPlayerPos = player ? player->GetPosition()
                               : nccu::engine::math::Vec2{0.0f, 0.0f};
    ForEachRole<IUpdatable>(world.Objects(),
                            [dt](IUpdatable& u) { u.Update(dt); });
}

// ── CollisionSystem ──────────────────────────────────────────────────
// Phase B: clamp the player to the world AABB right after Update(). Phase
// B2: axis-separated collision resolution — the terrain mask plus every
// BlocksMovement()-true object's hitbox push the player back on the blocked
// axis only (diagonal slides along walls). Items are deliberately not
// colliders. Verbatim extraction; the float-equality position writes are
// preserved exactly (a write happens iff the resolved value differs, which
// keeps the deterministic state.jsonl byte-identical).
void CollisionSystem::Run(SimContext& ctx, float /*dt*/) {
    using nccu::queries::ForEachActiveExcept;
    World& world = ctx.world;
    Player* player = world.GetPlayer();
    if (!player) return;

    const nccu::engine::math::Vec2 clamped =
        nccu::gfx::ClampToWorld(player->GetPosition(), ctx.playerSize,
                                ctx.worldSize);
    if (clamped.x != player->GetPosition().x ||
        clamped.y != player->GetPosition().y) {
        player->SetPosition(clamped);
    }

    ctx.frameColliders.clear();
    ForEachActiveExcept(world.Objects(), player, [&ctx](GameObject& o) {
        if (!o.BlocksMovement()) return;
        const nccu::engine::math::Vec2 p = o.GetPosition();
        ctx.frameColliders.push_back(
            nccu::engine::math::Rect{p.x, p.y, ctx.playerSize.x, ctx.playerSize.y});
    });
    const nccu::engine::math::Vec2 resolved = nccu::physics::ResolveMove(
        ctx.prevPlayerPos, player->GetPosition(), ctx.playerSize,
        ctx.frameColliders, &world.TerrainMask());
    if (resolved.x != player->GetPosition().x ||
        resolved.y != player->GetPosition().y) {
        player->SetPosition(resolved);
    }
}

// ── SpawnSystem ──────────────────────────────────────────────────────
// The 操場 校慶 lap progress tick (Ch3 only; a cheap no-op every other
// state) THEN the four deferred-spawn passes, in the original order:
//   A1: Ch1 苦主's-umbrella reveal-after-choice (Flag_SuitSeniorChoiceMade)
//   Ch2 散落筆記 reveal-after-wake (Flag_Bookworm)
//   T5: Ch3 TrueUmbrella reveal-after-clue (Flag_KnowsUmbrellaLoc)
//   G-3: Interlude 管理員的傘 return-point (Ch2->Ch3 market + holds loaner)
// Each self-gates + one-shots inside World and is a cheap no-op outside its
// chapter, so this stage is safe to run unconditionally every frame. World
// stays pure data; the system owns the per-frame tick (MVC).
void SpawnSystem::Run(SimContext& ctx, float /*dt*/) {
    World& world = ctx.world;
    world.UpdateSportsLap();
    world.MaybeSpawnChapter1VictimUmbrella();
    world.MaybeSpawnChapter2Notes();
    world.MaybeSpawnChapter3Umbrella();
    world.MaybeSpawnInterludeLibrarianReturn();
}

// ── SweepSystem ──────────────────────────────────────────────────────
// End-of-frame deferred deletion. The mark-then-sweep + the dangling-
// player-pointer guard live on World::Sweep().
void SweepSystem::Run(SimContext& ctx, float /*dt*/) {
    ctx.world.Sweep();
}

} // namespace nccu
