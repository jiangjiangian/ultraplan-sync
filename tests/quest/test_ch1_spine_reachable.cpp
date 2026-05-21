#include "doctest/doctest.h"
#include "harness/ScriptInput.h"
#include "controller/GameController.h"
#include "world/World.h"
#include "entities/Player.h"
#include "dialog/DialogState.h"
#include "dialog/DialogSource.h"
#include "quest/ChapterVendors.h"
#include "state/SemesterState.h"
#include "world/CollisionMask.h"
#include "quest/NpcSpawns.h"
#include "gfx/Rect.h"
#include "gfx/Input.h"
#include "gfx/Time.h"

#include <cmath>
#include <sstream>
#include <string>
#include <vector>

#ifndef TEST_CONTENT_DIR
#error "TEST_CONTENT_DIR must be defined by the build system"
#endif

using nccu::ScriptInput;
using nccu::World;

// BUGLEDGER I7 regression — Ch1→Ch2 progression must stay reachable on
// the SHIPPED collision_mask.png.
//
// Defect (perception-found, root-caused from src): collision_mask.png
// (versioned in git 964b4ee "version runtime-required assets", AFTER the
// harness ending scripts + the plan unit tests were authored against an
// all-walkable world) bakes a continuous E-W wall at y≈1761–1819 across
// the whole south campus. Its ONLY vertical gap is the column x≈880–1042.
// Every committed `goto` route drove straight up a WALLED column (x=320 /
// 560 / 750 / 1140 / 1180 / 1500 / 1560 / 1706), so the player flush-
// stopped at y≈1821 and never progressed — Ch1 soft-locked, no ending
// reached, byte-identically every run. `goto` is a pure axis driver (NOT
// a path-finder, by design — ScriptInput.cpp:260-270); the routes, not
// the engine, were stale (Verdict B). The campus is genuinely traversable
// (test_spawn_reachability's flood-fill passes); it just requires routing
// through the gap.
//
// This test pins a robust, mask+NPC-verified route that walks the
// MINIMAL Ch1 spine — talk to 苦主 (Flag_PromisedVictim, the umbrella
// claim gate), claim the TrueUmbrella (the Ch1 clear → Interlude via the
// UmbrellaClaimed EventWiring sibling-if), then exit the Interlude south
// (→ Chapter2_Midterms) — entirely through the x≈1041 gap and the
// wall-north y≈1750 corridor. It drives the REAL ScriptInput+
// GameController harness seam (exactly the Harness ordering). If a future
// mask edit re-seals the gap, or the route regresses, the semester never
// reaches Ch2 and this fails.
//
// Companion guarantees: test_scriptinput_plan.cpp (goto reaches a clear
// target / drive+E claims a non-blocking Item, both on this same mask)
// and test_i6_interact_reach.cpp (the interact verb opens NPC dialog).

namespace {

struct SpineResult {
    nccu::SemesterState semester;
    bool  promisedVictim;
    bool  hasTrueUmbrella;
    int   karma;
    int   frames;
};

SpineResult RunSpine(const std::string& script, int maxFrames) {
    nccu::dialog::SetContentDir(TEST_CONTENT_DIR);
    nccu::SetVendorContentDir(TEST_CONTENT_DIR);
    nccu::ReloadVendors();
    nccu::gfx::Time::SetFixedStep(1.0f / 60.0f);

    World world("", /*loadSprites=*/false);
    nccu::GameController controller{world};

    ScriptInput in;
    std::istringstream src(script);
    in.Load(src);
    nccu::gfx::Input::SetSource(&in);

    const World* snap = nullptr;
    int f = 0;
    for (; f < maxFrames; ++f) {
        in.Advance();
        in.ResolvePlan(snap);
        controller.Update();
        snap = &world;
        if (in.WantsQuit() || (in.HasPlan() && f >= 1 && in.PlanDone()))
            break;
    }
    nccu::gfx::Input::SetSource(nullptr);
    nccu::gfx::Time::SetFixedStep(0.0f);

    const Player* p = world.GetPlayer();
    return SpineResult{
        world.Semester().Current(),
        p && p->HasFlag("Flag_PromisedVictim"),
        p && p->HasFlag("Flag_HasTrueUmbrella"),
        p ? p->GetKarma() : -999,
        f};
}

// The verified minimal-Ch1-spine route. Every leg was traced against the
// real CollisionMask + DefaultNpcSpawns hitboxes (see the in-suite
// "wall gap" sanity case below) and run end-to-end through the harness.
const char* kSpineScript =
    "interact victim\n"               // → Flag_PromisedVictim (+karma)
    "choose 0\n"
    "advance\nadvance\nadvance\nadvance\nadvance\nadvance\n"
    "goto 500 1900\n"                 // onto the open south road
    "goto 1041 1900\n"                // east to the ONLY wall gap
    "goto 1041 1750\n"                // up the gap, north of the S wall
    "goto 380 1750\n"                 // west on the clear wall-north corridor
    "goto 380 1290\n"                 // up the clean x=380 column
    "interact trueumb 320 1280\n"     // claim TrueUmbrella → Ch1 clears → IL
    "wait 20\n"
    "goto 380 1750\n"                 // IL entry is {500,1500}; back to the
    "goto 1041 1750\n"                // gap (x=500 column is walled), then
    "goto 1041 1965\n"                // down through it into the IL exit zone
    "wait 40\n";                      // → Chapter2_Midterms

}  // namespace

TEST_CASE("I7: the shipped mask's south wall has exactly the x≈880-1042 gap") {
    // Grounds the route: prove the defect's geometry from the real asset
    // so the route's premise can't silently rot. Degrades gracefully if
    // the asset is absent (fresh clone) — then everything is walkable and
    // the spine trivially holds, so skip the geometry assertions.
    const nccu::CollisionMask m = nccu::LoadTerrainMask();
    if (m.Empty()) {
        MESSAGE("terrain mask absent — gap-geometry check skipped");
        return;
    }
    constexpr float B = 24.0f;
    // Walled columns the OLD routes used: blocked going up from the road.
    for (float x : {320.f, 560.f, 750.f, 1140.f, 1500.f, 1560.f, 1706.f}) {
        bool blockedUp = false;
        for (float y = 1858.f; y >= 1300.f; y -= 2.f)
            if (m.BlockedBox(x, y, B, B)) { blockedUp = true; break; }
        INFO("old-route column x=" << x << " should be wall-blocked");
        CHECK(blockedUp);
    }
    // The gap column x=1000 is clear top-to-bottom (the route's premise).
    bool gapClear = true;
    for (float y = 1900.f; y >= 1280.f; y -= 2.f)
        if (m.BlockedBox(1000.f, y, B, B)) { gapClear = false; break; }
    CHECK(gapClear);
    // The wall-north corridor y=1750 is clear of mask AND every Ch1 NPC
    // from the gap (x=1041) west to the x=380 climb column.
    std::vector<nccu::gfx::Rect> npc;
    for (const auto& n : nccu::DefaultNpcSpawns())
        npc.push_back({n.pos.x, n.pos.y, B, B});
    bool corridorClear = true;
    for (float x = 1041.f; x >= 380.f; x -= 2.f) {
        nccu::gfx::Rect a{x, 1750.f, B, B};
        bool hit = m.BlockedBox(x, 1750.f, B, B);
        for (const auto& r : npc) if (a.Intersects(r)) hit = true;
        if (hit) { corridorClear = false; break; }
    }
    CHECK(corridorClear);
}

TEST_CASE("I7: minimal Ch1 spine reaches Chapter 2 on the shipped mask") {
    const SpineResult r = RunSpine(kSpineScript, 9000);

    // The umbrella-claim gate fired (talked to 苦主).
    CHECK(r.promisedVictim);
    // The drive+E reached TrueUmbrella::beClaimed (Flag set only there).
    CHECK(r.hasTrueUmbrella);
    // 苦主 (b) 承諾 grants +5 over the karma-50 start.
    CHECK(r.karma == 55);
    // The spine progressed Ch1 → Interlude → Chapter2_Midterms. THIS is
    // the assertion that fails when the route is blocked (the player
    // soft-locks in Ch1 and the semester never advances): with the old
    // walled `goto 750 …` route the semester stayed Chapter1_AddDrop.
    CHECK(r.semester == nccu::SemesterState::Chapter2_Midterms);
    // Bounded — a soft-locked run would burn all 9000 frames; the
    // verified route completes well under that.
    CHECK(r.frames < 9000);
}

// Replay determinism (CLAUDE.md §4 tripwire): the resolver is a pure
// function of (plan step, World snapshot) — two runs of this exact
// script must trace byte-identical end state.
TEST_CASE("I7: the Ch1-spine route is deterministic across two runs") {
    const SpineResult a = RunSpine(kSpineScript, 9000);
    const SpineResult b = RunSpine(kSpineScript, 9000);
    CHECK(a.semester == b.semester);
    CHECK(a.promisedVictim == b.promisedVictim);
    CHECK(a.hasTrueUmbrella == b.hasTrueUmbrella);
    CHECK(a.karma == b.karma);
    CHECK(a.frames == b.frames);
    // And it actually reached Ch2 (guards against "deterministically
    // stuck" trivially passing the equality checks).
    CHECK(a.semester == nccu::SemesterState::Chapter2_Midterms);
}
