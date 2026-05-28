#include "doctest/doctest.h"
#include "gfx/Camera2D.h"
#include "engine/math/Vec2.h"

using namespace nccu::gfx;

TEST_CASE("Camera2D::ClampToWorld: target near centre is unchanged") {
    Camera2D c;
    c.Follow(Vec2{1000.0f, 1000.0f}, Vec2{400.0f, 225.0f});
    c.ClampToWorld(Vec2{2048.0f, 2048.0f}, Vec2{800.0f, 450.0f});
    CHECK(c.target.x == doctest::Approx(1000.0f));
    CHECK(c.target.y == doctest::Approx(1000.0f));
}

TEST_CASE("Camera2D::ClampToWorld: target at world origin clamps to half-viewport") {
    Camera2D c;
    c.Follow(Vec2{0.0f, 0.0f}, Vec2{400.0f, 225.0f});
    c.ClampToWorld(Vec2{2048.0f, 2048.0f}, Vec2{800.0f, 450.0f});
    CHECK(c.target.x == doctest::Approx(400.0f));
    CHECK(c.target.y == doctest::Approx(225.0f));
}

TEST_CASE("Camera2D::ClampToWorld: target past lower-right clamps to world - half-viewport") {
    Camera2D c;
    c.Follow(Vec2{5000.0f, 5000.0f}, Vec2{400.0f, 225.0f});
    c.ClampToWorld(Vec2{2048.0f, 2048.0f}, Vec2{800.0f, 450.0f});
    CHECK(c.target.x == doctest::Approx(2048.0f - 400.0f));
    CHECK(c.target.y == doctest::Approx(2048.0f - 225.0f));
}

TEST_CASE("Camera2D::ClampToWorld: world smaller than viewport pins target to world midpoint") {
    Camera2D c;
    c.Follow(Vec2{500.0f, 100.0f}, Vec2{400.0f, 225.0f});
    c.ClampToWorld(Vec2{200.0f, 100.0f}, Vec2{800.0f, 450.0f});
    CHECK(c.target.x == doctest::Approx(100.0f));   // 200 / 2
    CHECK(c.target.y == doctest::Approx(50.0f));    // 100 / 2
}

TEST_CASE("Camera2D::ClampToWorld returns *this for fluent chaining") {
    Camera2D c;
    auto& ret = c.Follow(Vec2{0.0f, 0.0f}, Vec2{400.0f, 225.0f})
                 .ClampToWorld(Vec2{2048.0f, 2048.0f}, Vec2{800.0f, 450.0f});
    CHECK(&ret == &c);
}
