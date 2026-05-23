#include "doctest/doctest.h"
#include "harness/ScriptInput.h"
#include "controller/GameController.h"
#include "world/World.h"
#include "entities/Player.h"
#include "dialog/DialogState.h"
#include "dialog/DialogSource.h"
#include "entities/GameObject.h"
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

// Exercises the high-level plan verbs (goto / interact / choose / advance)
// that let timelines drive the game without frame-exact hand-counting.
// Driven through the SAME path the harness uses: ScriptInput is the
// Input::Source and the loop mirrors Harness exactly — per frame
//   script.Advance(); script.ResolvePlan(prevWorldSnapshot);
//   controller.Update();
// with the snapshot lagging one frame (the real harness resolves a verb
// against the World captured at the previous EndFrame). Determinism: the
// resolver is a PURE function of (plan step, World snapshot) — no wall-
// clock, no RNG — so two runs of the same script trace byte-identically.

namespace {

// Per-frame observable trace token; two runs must be element-wise equal.
struct Frame {
    float x, y;
    bool  dialog;
    int   cursor;
    std::string npc;
    bool operator==(const Frame& o) const {
        return x == o.x && y == o.y && dialog == o.dialog &&
               cursor == o.cursor && npc == o.npc;
    }
};

// Runs `script` against a fresh World for up to `maxFrames`, faithfully
// mirroring Harness::BeginFrame/EndFrame ordering. RAII-restores the live
// input source / real timestep on exit.
std::vector<Frame> RunPlan(const std::string& script, int maxFrames) {
    nccu::dialog::SetContentDir(TEST_CONTENT_DIR);
    nccu::gfx::Time::SetFixedStep(1.0f / 60.0f);

    World world("", /*loadSprites=*/false);
    nccu::GameController controller{world};

    ScriptInput in;
    std::istringstream src(script);
    in.Load(src);
    nccu::gfx::Input::SetSource(&in);

    std::vector<Frame> trace;
    const World* snapshot = nullptr;   // null on frame 0, like the harness
    for (int f = 0; f < maxFrames; ++f) {
        in.Advance();
        in.ResolvePlan(snapshot);
        controller.Update();
        snapshot = &world;             // captured "at EndFrame"

        const Player* p = world.GetPlayer();
        const auto& d   = world.Dialog();
        trace.push_back(Frame{
            p ? p->GetPosition().x : 0.0f,
            p ? p->GetPosition().y : 0.0f,
            d.Active(), d.ChoiceCursor(), d.NpcId()});

        if (in.WantsQuit() || (in.HasPlan() && f >= 1 && in.PlanDone()))
            break;
    }

    nccu::gfx::Input::SetSource(nullptr);
    nccu::gfx::Time::SetFixedStep(0.0f);
    return trace;
}

const GameObject* FindNpc(const World& w, const char* id) {
    for (const auto& u : w.Objects())
        if (u && u->IsActive() && u->NpcId() == id) return u.get();
    return nullptr;
}

// 善有善報: the Ch1 苦主 moved to 綜合院館 (1660,1010), north of the
// south-campus wall, so the pure axis-driver `interact victim` can no
// longer reach him straight from the (500,1860) spawn. This gap route
// (auto-derived + mask-verified, `map_registry.py --route
// "500,1860 1660,1120"`) stages the player just south of the victim on the
// clear x=1660 column; the trailing verb then does the flush-approach.
const char* const kRouteToVictimStaging =
    "goto 1040 1712\n" "goto 1048 1704\n" "goto 1264 1632\n"
    "goto 1280 1624\n" "goto 1296 1616\n" "goto 1312 1608\n"
    "goto 1328 1600\n" "goto 1408 1512\n" "goto 1416 1504\n"
    "goto 1424 1496\n" "goto 1432 1488\n" "goto 1448 1480\n"
    "goto 1496 1456\n" "goto 1504 1448\n" "goto 1512 1440\n"
    "goto 1520 1432\n" "goto 1528 1424\n" "goto 1544 1328\n"
    "goto 1552 1320\n" "goto 1568 1312\n" "goto 1584 1304\n"
    "goto 1592 1296\n" "goto 1660 1120\n";

}  // namespace

// `goto` is a pure function of player position + 3 px/frame: it must land
// within the arrive-epsilon and never overshoot past it. Route picked to
// avoid the static quest-NPC walls AND the baked terrain mask (goto is an
// axis driver, not a path-finder — scripts compose waypoints to route
// around blockers).
//
// Route reconciliation (BUGLEDGER I7): the shipped collision_mask.png
// (committed in 964b4ee, AFTER this test was written against an
// all-walkable world) bakes a continuous E-W wall at y≈1761–1819 across
// the whole south campus; its ONLY gap is the column x≈880–1042. The old
// `goto 750 1860 / goto 750 1280` drove straight up x=750, which is
// walled at y≈1819 — the player flush-stopped at y≈1821 forever (`goto`
// correctly does not path-find). The genuinely clear column is the gap
// chute at x≈1000: x=1000 is mask-free top-to-bottom (verified — first
// blocked y == none for the full y span; no static NPC in this column).
// (1000,1300) is a free, NPC-free, mask-free target reachable by pure
// X-then-Y from the spawn. Cross-ref: tests/test_i6_interact_reach.cpp
// (owns the dialog-open guarantee) and tests/test_ch1_spine_reachable.cpp
// (owns the full Ch1→Ch2 progression proof on this same mask).
TEST_CASE("plan: `goto` drives the player to a free target within epsilon") {
    // Player spawns at {500,1860} on the open south road. East along the
    // road to x=1000 (the only wall gap), then straight up the clear
    // chute to y=1300 — all wall- and NPC-free (verified vs the shipped
    // collision_mask.png + DefaultNpcSpawns()).
    const std::vector<Frame> tr =
        RunPlan("goto 1000 1860\ngoto 1000 1300\n", 3000);

    REQUIRE_FALSE(tr.empty());
    const Frame& last = tr.back();
    CHECK(std::fabs(last.x - 1000.0f) < 3.0f);  // within one frame's travel
    CHECK(std::fabs(last.y - 1300.0f) < 3.0f);
}

// `interact <npcId>` looks up the NPC's LIVE world position and drives the
// player to it deterministically (pure function of the two positions).
// Post-I3/I6 geometry: the verb mirrors GameController's landed I3 reach
// probe — it drives at the NPC ORIGIN and presses E once an 8 px
// (kInteractReach) inflated player AABB overlaps the NPC, ~8 px BEFORE the
// movement collider flush-stop; the controller then opens the dialog and
// returns (no movement while a dialog is up), freezing the player ~27 px
// from the origin (flush-24 + the reach margin). So the drive endpoint is
// in the [≈24, 24+8] reach band, NOT flush-exactly-24 (the pre-I3 bound
// this once asserted held only because the I6 bug soft-locked the verb at
// the flush stop). This case guards the drive + determinism; that the
// dialog actually OPENS is guarded by test_i6_interact_reach.cpp.
TEST_CASE("plan: `interact victim` deterministically reaches the NPC") {
    World probeWorld("", /*loadSprites=*/false);
    const GameObject* v = FindNpc(probeWorld, "victim");
    REQUIRE(v != nullptr);
    const float vx = v->GetPosition().x;        // {1660,1010}
    const float vy = v->GetPosition().y;

    const std::vector<Frame> tr =
        RunPlan(std::string(kRouteToVictimStaging) + "interact victim\n", 4000);
    REQUIRE_FALSE(tr.empty());
    const Frame& last = tr.back();

    // Player routed through the gap, then drove into the I3 reach band:
    // flush-24 on the approach axis (Y here — staged south of the victim)
    // plus the 8 px kInteractReach margin (dialog opens before flush, then
    // the player is frozen for the dialog), exactly aligned on the other (X).
    CHECK(std::fabs(last.y - vy) <= 24.0f + 8.0f);   // post-I3 reach band
    CHECK(std::fabs(last.x - vx) <= 24.0f);
    CHECK((std::fabs(last.y - vy) >= 23.0f ||
           std::fabs(last.x - vx) < 1.0f));     // arrived adjacent, not far
}

// The verb's actuation (drive + E) IS correct end-to-end on a NON-blocking
// target: the player walks ONTO the 苦主's-umbrella QuestFlagPickup (an
// Item with BlocksMovement()==false, so the player CAN overlap it) and a
// tap of E reaches the game's OnPickup path — proving the synthetic edges
// are indistinguishable from hand-scripted input to GameController.
//
// 善有善報 reconciliation: the world TrueUmbrella was REMOVED (the 苦主
// grants it now), so the old "drive onto TrueUmbrella{320,1280}" target no
// longer exists. The findable victim's-umbrella pickup S of 集英樓
// (1700,1610) is the natural non-blocking-Item replacement. Route (BUGLEDGER
// I7): cross the south wall (y≈1761–1819) only via the x≈880–1042 gap, then
// up the clear EAST corridor (x≈1744-1752) to the wide-open umbrella spot —
// auto-derived + mask-verified (`map_registry.py --route`). Asserts
// Flag_HasVictimUmbrella (set ONLY by the QuestFlagPickup's OnPickup,
// ChapterQuestItems(Ch1)), so it genuinely exercises the drive+E→OnPickup
// path its title promises — not just a final coordinate. Cross-ref:
// tests/test_i6_interact_reach.cpp (NPC-dialog reach) and
// tests/test_ch1_spine_reachable.cpp (the full Ch1→Ch2 spine on this mask).
TEST_CASE("plan: goto+E actuates the game on a reachable (non-blocking) item") {
    nccu::dialog::SetContentDir(TEST_CONTENT_DIR);
    nccu::gfx::Time::SetFixedStep(1.0f / 60.0f);
    World world("", /*loadSprites=*/false);
    nccu::GameController controller{world};

    ScriptInput in;
    std::istringstream src(
        "goto 1040 1712\n" "goto 1048 1704\n" "goto 1264 1632\n"
        "goto 1280 1624\n" "goto 1296 1616\n" "goto 1312 1608\n"
        "goto 1328 1600\n" "goto 1700 1610\n"
        "interact victimumb 1700 1610\n" // drive onto the Item + E => OnPickup
        "wait 20\n");
    in.Load(src);
    nccu::gfx::Input::SetSource(&in);

    const World* snap = nullptr;
    bool planDone = false;
    for (int f = 0; f < 8000 && !planDone; ++f) {
        in.Advance();
        in.ResolvePlan(snap);
        controller.Update();
        snap = &world;
        if (in.HasPlan() && f >= 1 && in.PlanDone()) planDone = true;
    }
    nccu::gfx::Input::SetSource(nullptr);
    nccu::gfx::Time::SetFixedStep(0.0f);

    const Player* p = world.GetPlayer();
    REQUIRE(p != nullptr);
    // The drive+E reached QuestFlagPickup::OnPickup end-to-end:
    // Flag_HasVictimUmbrella is set ONLY there (ChapterQuestItems(Ch1)), so
    // this proves the synthetic E edge actuated the game on a reachable
    // non-blocking Item — exactly what this case is named for.
    CHECK(p->HasFlag("Flag_HasVictimUmbrella"));
}

// The headline guarantee: two runs of the SAME script (mixing every verb)
// trace byte-identical state — the in-memory equivalent of two identical
// state.jsonl files.
TEST_CASE("plan: replay is deterministic — two runs are byte-identical") {
    const std::string script =
        "goto 750 1860\n"
        "interact victim\n"
        "advance\n"
        "goto 750 1280\n";
    const std::vector<Frame> a = RunPlan(script, 3000);
    const std::vector<Frame> b = RunPlan(script, 3000);

    REQUIRE(a.size() == b.size());
    bool identical = true;
    for (std::size_t i = 0; i < a.size(); ++i)
        if (!(a[i] == b[i])) { identical = false; break; }
    CHECK(identical);
}

// The classic `<frame> <action>` grammar must keep working unchanged when
// interleaved with verbs (additive — never a regression).
TEST_CASE("plan: classic timed directives still parse alongside verbs") {
    ScriptInput in;
    std::istringstream src(
        "# a verb and classic lines in one file\n"
        "goto 100 100\n"
        "0 down D\n"
        "2 quit\n");
    in.Load(src);
    CHECK(in.HasPlan());

    in.Advance();                 // frame 0: classic D goes down
    CHECK(in.IsDown(nccu::gfx::Key::D));
    CHECK(in.IsPressed(nccu::gfx::Key::D));
    in.Advance();                 // frame 1: still held, no edge
    CHECK(in.IsDown(nccu::gfx::Key::D));
    CHECK_FALSE(in.IsPressed(nccu::gfx::Key::D));
    in.Advance();                 // frame 2: classic quit fires
    CHECK(in.WantsQuit());
}
