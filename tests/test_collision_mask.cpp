#include "doctest/doctest.h"
#include "CollisionMask.h"
#include "Physics.h"

using nccu::CollisionMask;
using nccu::gfx::Rect;
using nccu::gfx::Vec2;
using nccu::physics::ResolveMove;

namespace {
// 64x64 mask with a solid vertical wall in columns [30,40) for every row
// (10 px thick — comfortably caught by the 4 px footprint scan).
CollisionMask MakeWallMask() {
    constexpr int W = 64, H = 64;
    std::vector<std::uint8_t> g(static_cast<std::size_t>(W) * H, 0);
    for (int y = 0; y < H; ++y)
        for (int x = 30; x < 40; ++x)
            g[static_cast<std::size_t>(y) * W + x] = 1;
    return CollisionMask{W, H, std::move(g)};
}
} // namespace

TEST_CASE("CollisionMask: empty mask is walkable everywhere") {
    const CollisionMask m;
    CHECK(m.Empty());
    CHECK_FALSE(m.Solid(0, 0));
    CHECK_FALSE(m.Solid(1000, 1000));
    CHECK_FALSE(m.BlockedBox(10.0f, 10.0f, 24.0f, 24.0f));
}

TEST_CASE("CollisionMask: Solid() reports the painted cells") {
    const CollisionMask m = MakeWallMask();
    CHECK_FALSE(m.Empty());
    CHECK_FALSE(m.Solid(29, 10));
    CHECK(m.Solid(30, 10));
    CHECK(m.Solid(39, 63));
    CHECK_FALSE(m.Solid(40, 10));
    // Out-of-bounds samples clamp to the edge (here: walkable border).
    CHECK_FALSE(m.Solid(-5, -5));
    CHECK_FALSE(m.Solid(99999, 0));
}

TEST_CASE("CollisionMask: BlockedBox catches a footprint over the wall") {
    const CollisionMask m = MakeWallMask();
    CHECK_FALSE(m.BlockedBox(0.0f, 0.0f, 24.0f, 24.0f));   // west of wall
    CHECK(m.BlockedBox(20.0f, 0.0f, 24.0f, 24.0f));        // straddles wall
    CHECK(m.BlockedBox(32.0f, 0.0f, 4.0f, 4.0f));          // fully inside
    CHECK_FALSE(m.BlockedBox(44.0f, 0.0f, 16.0f, 16.0f));  // east of wall
}

TEST_CASE("ResolveMove: nullptr mask preserves rect-only behaviour") {
    // The new mask parameter defaults to nullptr — existing call sites
    // and the 93 legacy physics tests must behave exactly as before.
    const Vec2 prev{0.0f, 0.0f};
    const Vec2 desired{10.0f, 0.0f};
    const Vec2 size{24.0f, 24.0f};
    const std::vector<Rect> none;
    const Vec2 out = ResolveMove(prev, desired, size, none, nullptr);
    CHECK(out.x == doctest::Approx(10.0f));
    CHECK(out.y == doctest::Approx(0.0f));
}

TEST_CASE("ResolveMove: terrain mask blocks the X step, Y still slides") {
    const CollisionMask m = MakeWallMask();
    const std::vector<Rect> none;
    const Vec2 size{24.0f, 24.0f};
    // Player right edge at prev.x=5 is x=29 (clear); stepping east to
    // x=10 puts the right edge at 34, inside the [30,40) wall -> X
    // blocked. The Y delta from the un-moved X is wall-free -> slides.
    const Vec2 prev{5.0f, 10.0f};
    const Vec2 desired{10.0f, 18.0f};
    const Vec2 out = ResolveMove(prev, desired, size, none, &m);
    CHECK(out.x == doctest::Approx(5.0f));    // blocked by the mask
    CHECK(out.y == doctest::Approx(18.0f));   // free axis slides
}

TEST_CASE("ResolveMove: a clear lane through the mask is walkable") {
    const CollisionMask m = MakeWallMask();
    const std::vector<Rect> none;
    const Vec2 size{20.0f, 20.0f};
    // Entirely east of the wall (x>=40): movement is unobstructed.
    const Vec2 prev{42.0f, 2.0f};
    const Vec2 desired{43.0f, 5.0f};
    const Vec2 out = ResolveMove(prev, desired, size, none, &m);
    CHECK(out.x == doctest::Approx(43.0f));
    CHECK(out.y == doctest::Approx(5.0f));
}

TEST_CASE("ResolveMove: dynamic rect and terrain mask both apply") {
    const CollisionMask m = MakeWallMask();
    const Vec2 size{24.0f, 24.0f};
    // Moving DOWN (y) toward a dynamic actor's hitbox: the mask leaves
    // this column clear but the rect blocks the Y step.
    const std::vector<Rect> dyn{Rect{0.0f, 30.0f, 24.0f, 10.0f}};
    const Vec2 prev{0.0f, 0.0f};
    const Vec2 desired{0.0f, 12.0f};
    const Vec2 out = ResolveMove(prev, desired, size, dyn, &m);
    CHECK(out.y == doctest::Approx(0.0f));   // blocked by the dynamic rect
}
