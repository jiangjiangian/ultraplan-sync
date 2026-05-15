#include "doctest/doctest.h"
#include "gfx/Camera2D.h"
#include "gfx/Vec2.h"

using namespace nccu::gfx;

TEST_CASE("Camera2D defaults: zero offset/target/rotation, zoom 1.0") {
    Camera2D c;
    CHECK(c.offset.x   == doctest::Approx(0.0f));
    CHECK(c.offset.y   == doctest::Approx(0.0f));
    CHECK(c.target.x   == doctest::Approx(0.0f));
    CHECK(c.target.y   == doctest::Approx(0.0f));
    CHECK(c.rotation   == doctest::Approx(0.0f));
    CHECK(c.zoom       == doctest::Approx(1.0f));
}

TEST_CASE("Camera2D::Follow sets target = world target and offset = screen center") {
    Camera2D c;
    auto& ret = c.Follow(Vec2{1000.0f, 500.0f}, Vec2{400.0f, 225.0f});
    CHECK(&ret == &c);
    CHECK(c.target.x == doctest::Approx(1000.0f));
    CHECK(c.target.y == doctest::Approx(500.0f));
    CHECK(c.offset.x == doctest::Approx(400.0f));
    CHECK(c.offset.y == doctest::Approx(225.0f));
}

TEST_CASE("Camera2D::WithZoom + WithRotation are fluent setters") {
    Camera2D c;
    auto& ret = c.WithZoom(2.0f).WithRotation(45.0f);
    CHECK(&ret == &c);
    CHECK(c.zoom     == doctest::Approx(2.0f));
    CHECK(c.rotation == doctest::Approx(45.0f));
}
