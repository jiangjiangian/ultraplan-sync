#include "doctest/doctest.h"
#include "Physics.h"

using nccu::gfx::Rect;
using nccu::gfx::Vec2;
using nccu::physics::ResolveMove;

TEST_CASE("ResolveMove: no colliders -> desired position accepted") {
    const Vec2 prev{100.0f, 100.0f};
    const Vec2 desired{120.0f, 130.0f};
    const Vec2 size{24.0f, 24.0f};
    const std::vector<Rect> colliders;

    const Vec2 out = ResolveMove(prev, desired, size, colliders);
    CHECK(out.x == doctest::Approx(120.0f));
    CHECK(out.y == doctest::Approx(130.0f));
}

TEST_CASE("ResolveMove: collider far away -> desired position accepted") {
    const Vec2 prev{100.0f, 100.0f};
    const Vec2 desired{110.0f, 110.0f};
    const Vec2 size{24.0f, 24.0f};
    const std::vector<Rect> colliders{Rect{500.0f, 500.0f, 50.0f, 50.0f}};

    const Vec2 out = ResolveMove(prev, desired, size, colliders);
    CHECK(out.x == doctest::Approx(110.0f));
    CHECK(out.y == doctest::Approx(110.0f));
}

TEST_CASE("ResolveMove: head-on wall east -> x blocked, y stays") {
    // Wall starts at x=130 so player at prev.x=100 (right edge x=124) is
    // clear of it; stepping east to desired.x=110 (right edge x=134)
    // clips the wall and the X move is blocked.
    const Vec2 prev{100.0f, 100.0f};
    const Vec2 desired{110.0f, 100.0f};
    const Vec2 size{24.0f, 24.0f};
    const std::vector<Rect> colliders{Rect{130.0f, 50.0f, 100.0f, 200.0f}};

    const Vec2 out = ResolveMove(prev, desired, size, colliders);
    CHECK(out.x == doctest::Approx(100.0f));   // X blocked
    CHECK(out.y == doctest::Approx(100.0f));   // Y unchanged (zero delta)
}

TEST_CASE("ResolveMove: diagonal into wall corner -> slides on free axis") {
    // Wall covers x in [120,220], y in [200,400]. Player at (100,100) walks
    // SE to (110,110). The X step alone overlaps the wall (110..134 horiz,
    // but 100..124 vert doesn't intersect wall vert range 200..400), so
    // actually X is free. Let me pick a stricter case below.
    SUBCASE("X axis blocked, Y free (slide downward along east wall)") {
        // Wall starts at x=130 so player at prev.x=100 (right edge 124) is
        // clear of it; stepping to desired.x=110 (right edge 134) clips
        // the wall and is blocked. Y-step from prev.x leaves player still
        // west of the wall, so the slide downward should succeed.
        const Vec2 prev{100.0f, 250.0f};
        const Vec2 desired{110.0f, 260.0f};
        const Vec2 size{24.0f, 24.0f};
        const std::vector<Rect> colliders{Rect{130.0f, 200.0f, 100.0f, 200.0f}};

        const Vec2 out = ResolveMove(prev, desired, size, colliders);
        CHECK(out.x == doctest::Approx(100.0f));   // blocked
        CHECK(out.y == doctest::Approx(260.0f));   // slides
    }
    SUBCASE("Y axis blocked, X free (slide east along south wall)") {
        const Vec2 prev{100.0f, 100.0f};
        const Vec2 desired{110.0f, 110.0f};
        const Vec2 size{24.0f, 24.0f};
        // Wall directly below the player covering full x range we move through.
        const std::vector<Rect> colliders{Rect{50.0f, 130.0f, 200.0f, 50.0f}};

        const Vec2 out = ResolveMove(prev, desired, size, colliders);
        CHECK(out.x == doctest::Approx(110.0f));   // slides
        CHECK(out.y == doctest::Approx(100.0f));   // blocked
    }
}

TEST_CASE("ResolveMove: multiple colliders -> blocked by any") {
    const Vec2 prev{100.0f, 100.0f};
    const Vec2 desired{110.0f, 100.0f};
    const Vec2 size{24.0f, 24.0f};
    const std::vector<Rect> colliders{
        Rect{500.0f, 500.0f, 10.0f, 10.0f},
        Rect{130.0f,  50.0f, 100.0f, 200.0f},  // this one blocks the X step
        Rect{800.0f, 800.0f, 10.0f, 10.0f},
    };

    const Vec2 out = ResolveMove(prev, desired, size, colliders);
    CHECK(out.x == doctest::Approx(100.0f));
}

TEST_CASE("ResolveMove: NPC-sized collider blocks player too") {
    const Vec2 prev{100.0f, 100.0f};
    const Vec2 desired{110.0f, 100.0f};
    const Vec2 size{24.0f, 24.0f};
    // NPC at (130, 100), 24x24 — same shape as player. Player's right edge
    // at desired.x=110 is x=134, which clips the NPC starting at x=130.
    const std::vector<Rect> colliders{Rect{130.0f, 100.0f, 24.0f, 24.0f}};

    const Vec2 out = ResolveMove(prev, desired, size, colliders);
    CHECK(out.x == doctest::Approx(100.0f));
}

TEST_CASE("ResolveMove: escape when prev already overlaps a collider") {
    // Spawned on top of a wall (e.g., NPC hitbox overlapped Player at
    // start-of-frame). Without an escape clause both axis tests would
    // fail and the player would soft-lock; we want movement to proceed.
    const Vec2 prev{100.0f, 100.0f};
    const Vec2 desired{120.0f, 130.0f};
    const Vec2 size{24.0f, 24.0f};
    // Wall surrounds prev — every axis-test from prev fails.
    const std::vector<Rect> colliders{Rect{80.0f, 80.0f, 60.0f, 60.0f}};

    const Vec2 out = ResolveMove(prev, desired, size, colliders);
    CHECK(out.x == doctest::Approx(120.0f));
    CHECK(out.y == doctest::Approx(130.0f));
}
