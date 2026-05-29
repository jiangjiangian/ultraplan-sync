#include "doctest/doctest.h"
#include "game/gfx/WalkCycle.h"

using nccu::engine::math::Vec2;
using nccu::gfx::WalkColumn;
using nccu::gfx::WalkRowForFacing;
using nccu::gfx::kWalkColumns;

// U1-T3: the shared Pipoya walk-sheet maths the Player and a wandering /
// 校慶-runner NPC both use. Pure functions → headlessly testable; pins the
// exact cell selection NPC::Render relies on (its textured blit is GL-gated
// and not directly testable without a GL context).

TEST_CASE("WalkColumn cycles idle -> left -> idle -> right over 4 steps") {
    // The strip is idle(1), left-foot(0), idle(1), right-foot(2): the two
    // stride frames are separated by the idle frame so footfall reads
    // naturally — step 0 is the at-rest/idle pose.
    CHECK(WalkColumn(0) == 1);
    CHECK(WalkColumn(1) == 0);
    CHECK(WalkColumn(2) == 1);
    CHECK(WalkColumn(3) == 2);
    // Wraps for any integer (the NPC/Player counters do % 4 already, but
    // the helper must be total so a stale step never indexes out of range).
    CHECK(WalkColumn(4) == WalkColumn(0));
    CHECK(WalkColumn(7) == WalkColumn(3));
    CHECK(WalkColumn(-1) == WalkColumn(3));   // negative wraps too
    CHECK(WalkColumn(-4) == WalkColumn(0));
}

TEST_CASE("WalkColumn matches the canonical kWalkColumns table") {
    for (int s = 0; s < 4; ++s)
        CHECK(WalkColumn(s) == kWalkColumns[static_cast<std::size_t>(s)]);
}

TEST_CASE("WalkRowForFacing maps the four cardinal headings (0=down..3=up)") {
    CHECK(WalkRowForFacing(Vec2{0.0f,  1.0f}) == 0);   // down
    CHECK(WalkRowForFacing(Vec2{-1.0f, 0.0f}) == 1);   // left
    CHECK(WalkRowForFacing(Vec2{1.0f,  0.0f}) == 2);   // right
    CHECK(WalkRowForFacing(Vec2{0.0f, -1.0f}) == 3);   // up
}

TEST_CASE("WalkRowForFacing: dominant axis wins, ties go vertical") {
    // |x| > |y| -> horizontal row.
    CHECK(WalkRowForFacing(Vec2{-3.0f, 1.0f}) == 1);   // mostly left
    CHECK(WalkRowForFacing(Vec2{ 3.0f, 1.0f}) == 2);   // mostly right
    // |y| >= |x| -> vertical row (perfect diagonal ties to up/down).
    CHECK(WalkRowForFacing(Vec2{1.0f,  1.0f}) == 0);   // tie -> down
    CHECK(WalkRowForFacing(Vec2{1.0f, -1.0f}) == 3);   // tie -> up
    CHECK(WalkRowForFacing(Vec2{1.0f,  3.0f}) == 0);   // mostly down
    CHECK(WalkRowForFacing(Vec2{1.0f, -3.0f}) == 3);   // mostly up
}

TEST_CASE("WalkRowForFacing: a zero heading rests facing down (row 0)") {
    CHECK(WalkRowForFacing(Vec2{0.0f, 0.0f}) == 0);
}
