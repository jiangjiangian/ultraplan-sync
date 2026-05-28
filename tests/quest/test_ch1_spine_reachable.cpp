#include "doctest/doctest.h"
#include "quest/Flags.h"
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
#include "engine/math/Rect.h"
#include "engine/input/Input.h"
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
// 善有善報 redesign + A1 hard-gate: this test pins a robust, mask+NPC-verified
// route that walks the MINIMAL Ch1 reciprocity spine IN ORDER —
//   1. talk to 苦主 at 綜合院館 (Flag_PromisedVictim),
//   2. confront the 西裝學長 at 集英樓 and commit a choice
//      (Flag_SuitSeniorChoiceMade) — which is what makes the umbrella APPEAR
//      (A1: MaybeSpawnChapter1VictimUmbrella; before the choice the pickup
//      does NOT exist in the world, so the spine cannot be skipped),
//   3. find HIS transparent umbrella near 集英樓 (Flag_HasVictimUmbrella),
//   4. carry it BACK to him (the GRANT: TryReturnVictimUmbrella sets
//      Flag_HasTrueUmbrella + publishes UmbrellaClaimed → Ch1 clears →
//      Interlude via the EventWiring sibling-if),
//   5. exit the Interlude south (→ Chapter2_Midterms).
// The chapter clears on RETURNING the umbrella, NOT on grabbing one off the
// ground. All legs route through the x≈1041 gap / wall-north corridor / the
// clear x=1660 column. It drives the REAL ScriptInput+GameController harness
// seam (exactly the Harness ordering). If a future mask edit re-seals the
// gap, the route regresses, or the hard gate dead-ends, the semester never
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
        p && p->HasFlag(nccu::kFlagPromisedVictim),
        p && p->HasFlag(nccu::kFlagHasTrueUmbrella),
        p ? p->GetKarma() : -999,
        f};
}

// The verified minimal-Ch1-reciprocity-spine route. Every leg was traced
// against the real CollisionMask + DefaultNpcSpawns hitboxes (see the
// in-suite "wall gap" sanity case below) via map_registry.py --route and
// run end-to-end through the harness.
const char* kSpineScript =
    // (1) spawn → 苦主 @綜合院館 (1660,1010) through the gap, then promise.
    "goto 1040 1712\n" "goto 1048 1704\n" "goto 1264 1632\n"
    "goto 1280 1624\n" "goto 1296 1616\n" "goto 1312 1608\n"
    "goto 1328 1600\n" "goto 1408 1512\n" "goto 1416 1504\n"
    "goto 1424 1496\n" "goto 1432 1488\n" "goto 1448 1480\n"
    "goto 1496 1456\n" "goto 1504 1448\n" "goto 1512 1440\n"
    "goto 1520 1432\n" "goto 1528 1424\n" "goto 1544 1328\n"
    "goto 1552 1320\n" "goto 1568 1312\n" "goto 1584 1304\n"
    "goto 1592 1296\n" "goto 1660 1120\n"
    "interact victim\n"               // → Flag_PromisedVictim (+5 karma)
    "choose 0\n"
    "advance\nadvance\nadvance\nadvance\nadvance\nadvance\n"
    // (2) A1 hard-gate: confront the 西裝學長 @集英樓 (1620,1560). The 集英樓
    //     rect (1524,1353,224x192) walls its bottom at y≈1545 and the 學長 sits
    //     just SOUTH of it — so a straight WEST drive at y≈1545 jams flush on
    //     the wall (the (1646,1545) stick). Route DOWN the clear EAST corridor
    //     to y≈1620 (BELOW the building) FIRST, THEN WEST to him (map_registry
    //     --route "1660,1120 1752,1620 1620,1610"). `interact suit_senior`
    //     finishes the flush approach + E. Commit choice (d) 善意提醒 (+5,
    //     Flag_HelpedSenior) — this sets Flag_SuitSeniorChoiceMade, which makes
    //     the 苦主's umbrella SPAWN (before this it does not exist anywhere).
    //     The menu packs substates ≥1 ascending (b→0, c→1, d→2), so `choose 2`
    //     picks (d).
    "goto 1744 1168\n" "goto 1752 1620\n" "goto 1620 1610\n"
    "interact suit_senior\n"          // → Flag_SuitSeniorChoiceMade (+5)
    "choose 2\n"
    "advance\nadvance\nadvance\nadvance\nadvance\n"
    "wait 5\n"                        // → MaybeSpawnChapter1VictimUmbrella fires
    // (3) → 苦主's umbrella S of 集英樓 (1700,1610), now spawned; pick it up.
    //     The 學長 approach left the player just south of the building, so a
    //     short EAST hop reaches the pickup.
    "goto 1700 1610\n"
    "interact victimumb 1700 1610\n"  // → Flag_HasVictimUmbrella
    "wait 10\n"
    // (4) carry it BACK to 苦主 up the east corridor; GRANT (silent) + the
    //     (d) 重逢致謝 exchange dialogue. T2: the chapter clear is DEFERRED
    //     until that dialogue CLOSES, so we must read it through (advance)
    //     before LiftChapter1Clear fires UmbrellaClaimed → Interlude.
    "goto 1752 1480\n" "goto 1744 1344\n" "goto 1728 1336\n"
    "goto 1704 1328\n" "goto 1688 1320\n" "goto 1672 1312\n"
    "goto 1660 1120\n"
    "interact victim\n"               // GRANT (flags) + opens (d) exchange
    "advance\nadvance\nadvance\nadvance\nadvance\n"  // read + close the (d) scene
    "wait 20\n"                       // → LiftChapter1Clear fires → Interlude
    // (5) Interlude exit (entry repositions to {500,1500}) → Chapter2.
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
    // B4: the held umbrella + Flag_HasTrueUmbrella are now CLEARED on
    // Chapter2 entry (the「傘又掉了」card is mechanically true — each chapter
    // starts umbrella-less). So this spine, sampled at its END (in Ch2),
    // must NOT still carry the flag. That the Ch1 GRANT actually fired
    // (TryReturnVictimUmbrella → Flag_HasTrueUmbrella + UmbrellaClaimed) is
    // proven HERE by the spine having reached Ch2 at all — Ch1 clears ONLY
    // via that grant on this route (it never touches a morality umbrella) —
    // and is directly pinned WITHIN Ch1 by test_scriptinput_plan's drive+E
    // case. (Pre-B4 this asserted r.hasTrueUmbrella, which conflated "the
    // grant fired" with "the flag survives into Ch2"; B4 separates them.)
    CHECK_FALSE(r.hasTrueUmbrella);
    // 苦主 (b) 承諾 +5 over the karma-50 start, PLUS the 西裝學長 (d) 善意提醒
    // +5 (A1: the spine now routes through the 學長 choice) = karma 60.
    CHECK(r.karma == 60);
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
