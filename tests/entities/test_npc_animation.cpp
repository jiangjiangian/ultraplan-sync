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
    // seconds and confirm it both faces a heading AND advances a stride
    // column at some point (not pinned to the idle column forever).
    std::set<int> columnsSeen;
    bool sawMovingRowMatch = true;
    for (int i = 0; i < 240; ++i) {
        const nccu::gfx::Vec2 before = npc.GetPosition();
        npc.Update(kDt);
        const nccu::gfx::Vec2 after = npc.GetPosition();
        const NPC::RenderCell c = npc.CurrentRenderCell();
        columnsSeen.insert(c.col);
        // When the NPC actually displaced this frame, the rendered row must
        // equal the WalkCycle row for that net heading (the render reads the
        // same facing the move set).
        const nccu::gfx::Vec2 step{after.x - before.x, after.y - before.y};
        if (step.x != 0.0f || step.y != 0.0f) {
            if (c.row != nccu::gfx::WalkRowForFacing(step))
                sawMovingRowMatch = false;
        }
    }
    CHECK(sawMovingRowMatch);          // facing always matches the move
    // It advanced a real stride column at least once (left foot 0 OR right
    // foot 2) — i.e. it animated rather than standing on the idle column.
    CHECK((columnsSeen.count(0) == 1 || columnsSeen.count(2) == 1));
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
