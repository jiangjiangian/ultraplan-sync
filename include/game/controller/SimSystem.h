#ifndef SIM_SYSTEM_H_
#define SIM_SYSTEM_H_
#include "engine/math/Rect.h"
#include "engine/math/Vec2.h"
#include <vector>

namespace nccu {

class World;

// The per-frame simulation pipeline (awsome_cpp.md §6/§7 — the Controller
// "容易膨脹成 God Class，內部仍須職責分離"; §7 SRP). GameController::Update
// used to inline every simulation stage as one ~793-line god-method. Each
// SPATIALLY-isolated, reusable stage is now an ISystem with ONE
// responsibility, run by the controller in the EXACT same order as before
// (so the harness state.jsonl stays byte-identical). These are exactly the
// reusable model-side stages an Assignment-#6 survival game also needs (a
// CollisionSystem + a Spawner), now first-class types rather than buried in
// the controller.
//
// MVC purity: a system operates ONLY on the model (World& / Player&). It
// reads no input devices, makes no raylib calls, renders nothing — those
// stay in the controller (input) and View (render). The per-screen input
// blocks (ending menu / pause / dialog-advance / inventory) and the
// E-interact dispatch are NOT systems; they remain in the controller layer
// because they read input.

// Model-only context threaded through the pipeline for one frame. Bundles
// the World, the two geometry constants the collision stage needs, a reused
// scratch collider list (so the per-frame vector isn't reallocated), and
// the previous-frame player position handed from MovementSystem to
// CollisionSystem. No raylib, no input — pure data.
struct SimContext {
    World&                          world;
    nccu::gfx::Vec2                 worldSize;
    nccu::gfx::Vec2                 playerSize;
    std::vector<nccu::gfx::Rect>&   frameColliders;  // reused scratch
    // Set by MovementSystem (the player's top-left BEFORE this frame's
    // object Update tick), read by CollisionSystem for the axis-separated
    // resolve. Lives in the context so the two stages stay decoupled types
    // yet preserve the exact prev→resolve handoff the inline code had.
    nccu::gfx::Vec2                 prevPlayerPos{0.0f, 0.0f};
};

// One ordered simulation stage. Run() advances the model by `dt` seconds.
struct ISystem {
    virtual ~ISystem() = default;
    virtual void Run(SimContext& ctx, float dt) = 0;
};

// ── Concrete stages, in pipeline order ───────────────────────────────

// Rain-survival accrual/drain (BUGLEDGER I8 / Cycle 4 / REQUIREMENT #5).
// Three states: inside a building → DrainRain (-10 u/s); outdoors with an
// umbrella → ApplyRainSheltered (+1.5 u/s); outdoors umbrella-less →
// ApplyRain (+5 u/s, lethal). Skipped in the market interlude and endings.
struct SurvivalSystem final : ISystem {
    void Run(SimContext& ctx, float dt) override;
};

// Captures the player's pre-tick position into ctx.prevPlayerPos, then
// ticks every IUpdatable object (ForEachRole<IUpdatable>). The ISP role
// dispatch is unchanged — only objects that play the IUpdatable role move.
struct MovementSystem final : ISystem {
    void Run(SimContext& ctx, float dt) override;
};

// Player AABB resolution: clamp to the world box, then axis-separated
// resolve against every BlocksMovement() object's hitbox plus the terrain
// mask (the #6 CollisionSystem). Items are deliberately not colliders.
struct CollisionSystem final : ISystem {
    void Run(SimContext& ctx, float dt) override;
};

// Deferred per-frame world spawns (the #6 Spawner): the 操場 lap tick then
// the four self-gating MaybeSpawn passes (Ch1 victim umbrella / Ch2 notes /
// Ch3 umbrella / Interlude librarian return). Each self-gates + is a cheap
// no-op outside its chapter, exactly as the inline calls were.
struct SpawnSystem final : ISystem {
    void Run(SimContext& ctx, float dt) override;
};

// End-of-frame mark-then-sweep (deferred deletion). Delegates to
// World::Sweep() — kept as a stage so the pipeline owns the full ordered
// frame and a future system can be inserted before/after the sweep.
struct SweepSystem final : ISystem {
    void Run(SimContext& ctx, float dt) override;
};

} // namespace nccu

#endif // SIM_SYSTEM_H_
