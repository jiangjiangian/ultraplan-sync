#include "doctest/doctest.h"
#include "controller/SimSystem.h"
#include "world/World.h"
#include "entities/Player.h"
#include "entities/NPC.h"
#include "engine/math/Vec2.h"
#include "engine/math/Rect.h"

#include <cstdint>
#include <memory>
#include <vector>

// Focused unit tests for the model-advance pipeline extracted from
// GameController::Update (awsome_cpp.md §6/§7). Each ISystem operates on
// World&/Player& only — no input, no raylib — so a headless World
// (loadSprites=false) can drive it directly and assert the stage's single
// responsibility in isolation. The byte-identical state.jsonl gate proves
// the WHOLE pipeline matches the old inline order; these pin each stage's
// contract so a future change to one stage is caught at the unit level.

using nccu::CollisionSystem;
using nccu::MovementSystem;
using nccu::SimContext;
using nccu::SpawnSystem;
using nccu::SurvivalSystem;
using nccu::SweepSystem;
using nccu::SemesterState;
using nccu::World;

namespace {

// A fresh SimContext over a World, mirroring how GameController builds it
// each frame (the two geometry constants + a reused scratch collider list).
struct Fixture {
    World                          w{"", /*loadSprites=*/false};
    std::vector<nccu::gfx::Rect>   colliders;
    SimContext ctx() {
        return SimContext{w, nccu::gfx::Vec2{2048.0f, 2048.0f},
                          nccu::gfx::Vec2{24.0f, 24.0f}, colliders, {}};
    }
};

} // namespace

// ── SurvivalSystem: the 3-way rain branch ───────────────────────────
TEST_CASE("SurvivalSystem: outdoors umbrella-less ACCRUES rain (Ch1 default)") {
    Fixture f;
    Player* p = f.w.GetPlayer();
    REQUIRE(p != nullptr);
    REQUIRE(p->HasUmbrella() == false);
    REQUIRE(f.w.CurrentBuildingName().empty());
    const float before = p->GetRainMeter();
    SurvivalSystem sys;
    SimContext c = f.ctx();
    sys.Run(c, 1.0f);                       // 1 s exposed
    CHECK(p->GetRainMeter() > before);      // +5 u/s accrual
}

TEST_CASE("SurvivalSystem: outdoors WITH umbrella accrues at the reduced rate") {
    Fixture f;
    Player* p = f.w.GetPlayer();
    p->SetHasUmbrella(true);
    const float before = p->GetRainMeter();
    SurvivalSystem sys;
    SimContext c = f.ctx();
    sys.Run(c, 1.0f);
    // Sheltered accrual is positive but slower than the exposed +5.
    CHECK(p->GetRainMeter() > before);
    CHECK(p->GetRainMeter() - before < 5.0f);
}

TEST_CASE("SurvivalSystem: inside a building DRAINS rain") {
    Fixture f;
    Player* p = f.w.GetPlayer();
    p->ApplyRain(2.0f, /*lethal=*/false);   // soak first so there is room to drain
    const float soaked = p->GetRainMeter();
    REQUIRE(soaked > 0.0f);
    f.w.CurrentBuildingName() = "圖書館";    // now sheltered indoors
    SurvivalSystem sys;
    SimContext c = f.ctx();
    sys.Run(c, 0.5f);
    CHECK(p->GetRainMeter() < soaked);      // -10 u/s recovery
}

TEST_CASE("SurvivalSystem: no rain tick in the market interlude") {
    Fixture f;
    Player* p = f.w.GetPlayer();
    f.w.Semester().Transition(SemesterState::Interlude_Market);
    const float before = p->GetRainMeter();
    SurvivalSystem sys;
    SimContext c = f.ctx();
    sys.Run(c, 5.0f);                        // long tick, but skipped
    CHECK(p->GetRainMeter() == doctest::Approx(before));
}

// ── MovementSystem: capture prev pos + tick objects ─────────────────
TEST_CASE("MovementSystem captures the pre-tick player position into the ctx") {
    Fixture f;
    Player* p = f.w.GetPlayer();
    p->SetPosition(nccu::gfx::Vec2{123.0f, 456.0f});
    MovementSystem sys;
    SimContext c = f.ctx();
    sys.Run(c, 0.016f);
    CHECK(c.prevPlayerPos.x == doctest::Approx(123.0f));
    CHECK(c.prevPlayerPos.y == doctest::Approx(456.0f));
}

// ── CollisionSystem: clamp the player into the world AABB ───────────
TEST_CASE("CollisionSystem clamps an out-of-bounds player back into the world") {
    Fixture f;
    Player* p = f.w.GetPlayer();
    // Park the player past the right/bottom edge; the clamp must pull it
    // back to worldSize - playerSize on each axis.
    p->SetPosition(nccu::gfx::Vec2{9000.0f, 9000.0f});
    CollisionSystem sys;
    SimContext c = f.ctx();
    c.prevPlayerPos = p->GetPosition();      // Movement would have set this
    sys.Run(c, 0.016f);
    CHECK(p->GetPosition().x <= 2048.0f - 24.0f);
    CHECK(p->GetPosition().y <= 2048.0f - 24.0f);
    CHECK(p->GetPosition().x >= 0.0f);
    CHECK(p->GetPosition().y >= 0.0f);
}

// ── SpawnSystem: cheap no-op outside its chapters, never crashes ────
TEST_CASE("SpawnSystem is a safe no-op in Ch1 default (no deferred spawn armed)") {
    Fixture f;
    const std::size_t before = f.w.Objects().size();
    SpawnSystem sys;
    SimContext c = f.ctx();
    sys.Run(c, 0.016f);
    // None of the four deferred spawns is armed in a fresh Ch1 (no flags
    // set), and the lap tick is a no-op outside Ch3 — so the roster is
    // unchanged. The point is that running the whole stage is correct and
    // crash-free regardless of chapter.
    CHECK(f.w.Objects().size() == before);
}

// ── SweepSystem / World::Sweep ──────────────────────────────────────
TEST_CASE("SweepSystem removes a deactivated object, keeps the Player at front") {
    Fixture f;
    Player* p = f.w.GetPlayer();
    REQUIRE(p != nullptr);
    REQUIRE(f.w.Objects().front().get() == static_cast<GameObject*>(p));
    // Append a dead NPC at the back, then sweep.
    auto npc = std::make_unique<NPC>(nccu::gfx::Vec2{50, 50},
                                     std::vector<std::string>{"hi"});
    npc->Deactivate();
    f.w.Objects().push_back(std::move(npc));
    const std::size_t before = f.w.Objects().size();
    SweepSystem sys;
    SimContext c = f.ctx();
    sys.Run(c, 0.016f);
    CHECK(f.w.Objects().size() == before - 1);          // dead NPC reaped
    CHECK(f.w.Objects().front().get() ==                 // front still Player
          static_cast<GameObject*>(p));
    CHECK(f.w.GetPlayer() == p);                         // cache still valid
}

TEST_CASE("World::Sweep clears the cached player pointer when the Player dies") {
    World w{"", /*loadSprites=*/false};
    Player* p = w.GetPlayer();
    REQUIRE(p != nullptr);
    // The ctor spawns a chapter roster too, so the world is not empty —
    // the contract on player death is that the cache is cleared (before the
    // erase frees the object) and the player object itself is gone.
    const std::size_t before = w.Objects().size();
    // Capture the player's address as an integer BEFORE the sweep frees it
    // (comparing a freed pointer's *value* is unspecified; an integer copy
    // is always well-defined), so the "no survivor is the old player" check
    // never touches an invalid pointer.
    const std::uintptr_t deadAddr =
        reinterpret_cast<std::uintptr_t>(static_cast<GameObject*>(p));
    p->Deactivate();                  // mark the player dead this frame
    w.Sweep();
    CHECK(w.GetPlayer() == nullptr);  // cache cleared before the erase freed it
    CHECK(w.Objects().size() == before - 1);  // the player object was removed
    // No surviving object lives at the (freed) player's old address.
    for (const auto& o : w.Objects())
        CHECK(reinterpret_cast<std::uintptr_t>(o.get()) != deadAddr);
}

TEST_CASE("World::Sweep is a no-op when nothing is dead") {
    World w{"", /*loadSprites=*/false};
    const std::size_t before = w.Objects().size();
    Player* p = w.GetPlayer();
    w.Sweep();
    CHECK(w.Objects().size() == before);
    CHECK(w.GetPlayer() == p);
}
