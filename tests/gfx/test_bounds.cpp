#include "doctest/doctest.h"
#include "game/gfx/Bounds.h"
#include "engine/math/Vec2.h"

using namespace nccu::gfx;
using namespace nccu::engine::math;

TEST_CASE("ClampToWorld: point inside world is unchanged") {
    auto v = ClampToWorld(Vec2{1000.0f, 1000.0f}, Vec2{24.0f, 24.0f}, Vec2{2048.0f, 2048.0f});
    CHECK(v.x == doctest::Approx(1000.0f));
    CHECK(v.y == doctest::Approx(1000.0f));
}

TEST_CASE("ClampToWorld: point past upper-left pins to 0") {
    auto v = ClampToWorld(Vec2{-50.0f, -10.0f}, Vec2{24.0f, 24.0f}, Vec2{2048.0f, 2048.0f});
    CHECK(v.x == doctest::Approx(0.0f));
    CHECK(v.y == doctest::Approx(0.0f));
}

TEST_CASE("ClampToWorld: point past lower-right pins so size stays inside") {
    auto v = ClampToWorld(Vec2{3000.0f, 3000.0f}, Vec2{24.0f, 24.0f}, Vec2{2048.0f, 2048.0f});
    CHECK(v.x == doctest::Approx(2048.0f - 24.0f));
    CHECK(v.y == doctest::Approx(2048.0f - 24.0f));
}

TEST_CASE("ClampToWorld: size > world pins pos to 0 on that axis") {
    auto v = ClampToWorld(Vec2{500.0f, 500.0f}, Vec2{4096.0f, 24.0f}, Vec2{2048.0f, 2048.0f});
    CHECK(v.x == doctest::Approx(0.0f));
    CHECK(v.y == doctest::Approx(500.0f));
}
