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
    const float vx = v->GetPosition().x;        // {380,1860}
    const float vy = v->GetPosition().y;

    const std::vector<Frame> tr = RunPlan("interact victim\n", 3000);
    REQUIRE_FALSE(tr.empty());
    const Frame& last = tr.back();

    // Player drove into the I3 reach band: flush-24 on the approach axis
    // plus the 8 px kInteractReach margin (dialog opens before flush, then
    // the player is frozen for the dialog), exactly aligned on the other.
    CHECK(std::fabs(last.x - vx) <= 24.0f + 8.0f);   // post-I3 reach band
    CHECK(std::fabs(last.y - vy) <= 24.0f);
    CHECK((std::fabs(last.x - vx) >= 23.0f ||
           std::fabs(last.y - vy) < 1.0f));     // arrived adjacent, not far
}

// The verb's actuation (drive + E) IS correct end-to-end on a NON-blocking
// target: the player walks ONTO the TrueUmbrella (a TransparentUmbrella,
// an Item with BlocksMovement()==false, so the player CAN overlap it) and
// a tap of E reaches the game's beClaimed path — proving the synthetic
// edges are indistinguishable from hand-scripted input to GameController.
//
// Route reconciliation (BUGLEDGER I7): on the shipped collision_mask.png
// the umbrella strip (y≈1280) is sealed off from the spawn road by the
// south wall (y≈1761–1819); the only crossing is the x≈880–1042 gap. The
// old route drove straight up x=750 (the FragileUmbrella column), which
// is walled — the player froze at y≈1821 and the case asserted a position
// it could never reach. The robust, mask+NPC-verified route is: claim
// gate (talk to 苦主 for Flag_PromisedVictim — every umbrella's beClaimed
// is QuestGate-d, see TransparentUmbrella.cpp) → east on the open road →
// up the x=1041 gap → west on the wall-north y≈1750 corridor (clear of
// the mask AND all 5 DefaultNpcSpawns NPCs) → up the clean x=380 column
// → onto the TrueUmbrella{320,1280}. This now also asserts the CLAIM
// (Flag_HasTrueUmbrella, set only by TrueUmbrella::beClaimed), so it
// genuinely exercises the drive+E→beClaimed path its title promises —
// not just a final coordinate. Cross-ref: tests/test_i6_interact_reach
// .cpp (NPC-dialog reach) and tests/test_ch1_spine_reachable.cpp (the
// full Ch1→Ch2 spine on this mask).
TEST_CASE("plan: goto+E actuates the game on a reachable (non-blocking) item") {
    nccu::dialog::SetContentDir(TEST_CONTENT_DIR);
    nccu::gfx::Time::SetFixedStep(1.0f / 60.0f);
    World world("", /*loadSprites=*/false);
    nccu::GameController controller{world};

    ScriptInput in;
    std::istringstream src(
        "interact victim\n"           // Flag_PromisedVictim (claim gate)
        "choose 0\n"
        "advance\nadvance\nadvance\nadvance\nadvance\nadvance\n"
        "goto 500 1900\n"             // onto the open south road
        "goto 1041 1900\n"            // east to the only wall gap
        "goto 1041 1750\n"            // up the gap, north of the S wall
        "goto 380 1750\n"             // west on the clear wall-north corridor
        "goto 380 1290\n"             // up the clean x=380 column
        "interact trueumb 320 1280\n" // drive onto the Item + E => beClaimed
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
    // The drive+E reached TransparentUmbrella::beClaimed end-to-end:
    // Flag_HasTrueUmbrella is set ONLY there (src/TrueUmbrella.cpp), so
    // this proves the synthetic E edge actuated the game on a reachable
    // non-blocking Item — exactly what this case is named for.
    CHECK(p->HasFlag("Flag_HasTrueUmbrella"));
    CHECK(p->HasFlag("Flag_PromisedVictim"));
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
