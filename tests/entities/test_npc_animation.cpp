#include "doctest/doctest.h"
#include "entities/NPC.h"
#include "gfx/WalkCycle.h"

#include <set>
#include <vector>

// U1-T3: a wandering / 校慶-runner NPC plays the Pipoya 4-direction walk
// cycle (owner's request); a stationary dialogue / quest archetype NPC
// stays on its idle frame. NPC::CurrentRenderCell() is the single place the
// {col,row} is chosen, so these pin the actual render selection without a
// GL context (the textured blit itself is GL-gated). The cell is a pure
// read of already-simulated heading/anim state — none of it is serialised,
// so state.jsonl stays byte-identical (verified separately vs baseline).

namespace {
constexpr float kDt = 1.0f / 60.0f;   // the harness fixed step
}

TEST_CASE("Stationary archetype NPC always renders the idle cell (col 1, row 0)") {
    NPC npc(nccu::gfx::Vec2{500.0f, 500.0f},
            std::vector<std::string>{"hi"}, /*isQuestGiver=*/true, "ta");
    // Before any Update.
    CHECK(npc.CurrentRenderCell().col == 1);
    CHECK(npc.CurrentRenderCell().row == 0);
    // Update is a no-op for a stationary NPC (it stands at its post), so the
    // idle cell is invariant across frames.
    for (int i = 0; i < 120; ++i) npc.Update(kDt);
    CHECK(npc.CurrentRenderCell().col == 1);
    CHECK(npc.CurrentRenderCell().row == 0);
}

TEST_CASE("校慶 circular runner walk-animates and faces its motion") {
    NPC npc(nccu::gfx::Vec2{1000.0f, 1000.0f}, {});
    // Centre below the start point so the first tangential step heads a
    // known way; angularSpeed positive => counter-clockwise.
    npc.EnableCircularRun(nccu::gfx::Vec2{1000.0f, 1100.0f},
                          /*radius=*/100.0f, /*angularSpeed=*/1.5f,
                          /*startAngle=*/ -1.5707963f /* -pi/2: top of circle */);

    std::set<int> columnsSeen;
    std::set<int> rowsSeen;
    for (int i = 0; i < 240; ++i) {        // 4 s — well over a full lap
        npc.Update(kDt);
        const NPC::RenderCell c = npc.CurrentRenderCell();
        columnsSeen.insert(c.col);
        rowsSeen.insert(c.row);
        // The row is always a valid Pipoya facing row.
        CHECK(c.row >= 0);
        CHECK(c.row <= 3);
    }
    // Over a lap it cycles through BOTH stride columns (0 and 2), not just
    // the idle column — i.e. it actually animates, not slides.
    CHECK(columnsSeen.count(0) == 1);
    CHECK(columnsSeen.count(2) == 1);
    // Circling the track faces it through multiple directions (more than one
    // row), proving the heading drives the row.
    CHECK(rowsSeen.size() >= 2);
}

TEST_CASE("Ambient wanderer leaves the idle pose once it actually moves") {
    NPC npc(nccu::gfx::Vec2{1000.0f, 1000.0f}, {});
    npc.EnableWander(/*speed=*/40.0f, /*seed=*/12345u);  // deterministic PRNG
    // No wander mask set => it moves freely (open ground), so it WILL
    // displace on most frames and the walk cycle advances. Drive a couple of
    // seconds and confirm it both faces a VALID heading row AND advances a
    // stride column at some point (not pinned to the idle column forever).
    std::set<int> columnsSeen;
    for (int i = 0; i < 240; ++i) {
        npc.Update(kDt);
        const NPC::RenderCell c = npc.CurrentRenderCell();
        columnsSeen.insert(c.col);
        // The rendered row is always a valid Pipoya facing row.
        CHECK(c.row >= 0);
        CHECK(c.row <= 3);
    }
    // It advanced a real stride column at least once (left foot 0 OR right
    // foot 2) — i.e. it animated rather than standing on the idle column.
    CHECK((columnsSeen.count(0) == 1 || columnsSeen.count(2) == 1));
}

// A-T1 (走動 NPC 詭異抖動) regression. The jitter was: facing_ was keyed to
// the per-frame NET displacement (step), and WalkRowForFacing picks the row
// by dominant axis. When a wanderer slides along a wall / world edge, the
// blocked axis is zeroed so the net step flip-flops between axis-aligned
// (e.g. {d,0}) and diagonal, flipping the facing row left↔down EVERY frame.
// The fix keys facing_ to the STABLE retarget heading (wanderDir_), constant
// for a whole 1–3 s leg, so the row is invariant across the slide.
//
// Deterministic construction: seed 51 makes the FIRST wander heading the
// down-right diagonal {1,1} (verified — see the seed table comment). Park
// the NPC flush against the BOTTOM world edge (y = kSize-kPlayerHeight). A
// {1,1} move then clamps y (already at the floor) while x keeps increasing,
// so the NET step is {d,0} (axis-aligned RIGHT) on every frame even though
// the intended heading is the down-right diagonal. WalkRowForFacing of the
// diagonal {1,1} ties to vertical => row 0 (down); of the step {d,0} =>
// row 2 (right). So:
//   * FIXED (facing = wanderDir_):  row stays 0 (down) — stable, no jitter.
//   * BUGGY (facing = step):        row is 2 (right) — and would flicker as
//                                   the slide alternates blocked axes.
// The CHECK below FAILS on the pre-fix code (it would see row 2 / a row
// that changes), and PASSES on the fix (row pinned to the stable heading).
TEST_CASE("A-T1: a wall-sliding wanderer holds ONE stable facing row (no jitter)") {
    constexpr float kEdge = 2048.0f - 24.0f;   // kSize - kPlayerHeight
    NPC npc(nccu::gfx::Vec2{1500.0f, kEdge}, {});   // flush against the floor
    npc.EnableWander(/*speed=*/40.0f, /*seed=*/51u);  // first heading {1,1}

    std::set<int> movingRows;
    int movingFrames = 0;
    // The first leg lasts >= 60 frames (retarget interval 1–3 s), and the
    // NPC slides right along the floor the whole time, so inspect a window
    // well inside that single constant-heading leg.
    for (int i = 0; i < 50; ++i) {
        const nccu::gfx::Vec2 before = npc.GetPosition();
        npc.Update(kDt);
        const nccu::gfx::Vec2 after = npc.GetPosition();
        const nccu::gfx::Vec2 step{after.x - before.x, after.y - before.y};
        if (step.x != 0.0f || step.y != 0.0f) {
            ++movingFrames;
            movingRows.insert(npc.CurrentRenderCell().row);
            // The NET step is axis-aligned RIGHT (y is pinned at the floor),
            // confirming this IS the jitter-inducing slide the bug hit.
            CHECK(step.x > 0.0f);
            CHECK(step.y == doctest::Approx(0.0f));
        }
    }
    REQUIRE(movingFrames > 0);            // it really did slide
    // The fix: ONE facing row across the whole slide, and it is the STABLE
    // heading's row (down = 0), NOT the per-frame step's row (right = 2).
    CHECK(movingRows.size() == 1);                       // no flicker
    CHECK(movingRows.count(0) == 1);                     // stable heading (down)
    CHECK(movingRows.count(2) == 0);                     // not the step's row
}

TEST_CASE("A paused wanderer (no displacement) shows the idle column") {
    // Force a stall: seed a wanderer, but only inspect frames where the net
    // move was zero — those must render the idle column (Update resets
    // animStep_ to 0 when moving_ is false).
    NPC npc(nccu::gfx::Vec2{1000.0f, 1000.0f}, {});
    npc.EnableWander(/*speed=*/40.0f, /*seed=*/777u);
    int idleFramesChecked = 0;
    for (int i = 0; i < 600; ++i) {
        const nccu::gfx::Vec2 before = npc.GetPosition();
        npc.Update(kDt);
        const nccu::gfx::Vec2 after = npc.GetPosition();
        if (before.x == after.x && before.y == after.y) {
            CHECK(npc.CurrentRenderCell().col == 1);   // idle column
            ++idleFramesChecked;
        }
    }
    // The PRNG includes a pause heading (idx 8 = {0,0}) plus world-edge
    // clamps, so over 10 s at least one zero-displacement frame occurs.
    CHECK(idleFramesChecked > 0);
}
