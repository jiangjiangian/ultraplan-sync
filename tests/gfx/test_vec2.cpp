#include "doctest/doctest.h"
#include "engine/math/Vec2.h"
#include <cmath>

using namespace nccu::gfx;

TEST_CASE("Vec2: default-constructs to (0,0)") {
    constexpr Vec2 v;
    CHECK(v.x == doctest::Approx(0.0f));
    CHECK(v.y == doctest::Approx(0.0f));
}

TEST_CASE("Vec2: aggregate init {x,y}") {
    constexpr Vec2 v{3.0f, 4.0f};
    CHECK(v.x == doctest::Approx(3.0f));
    CHECK(v.y == doctest::Approx(4.0f));
}

TEST_CASE("Vec2 arithmetic operators") {
    constexpr Vec2 a{1, 2};
    constexpr Vec2 b{3, 5};
    CHECK((a + b).x == doctest::Approx(4.0f));
    CHECK((a + b).y == doctest::Approx(7.0f));
    CHECK((b - a).x == doctest::Approx(2.0f));
    CHECK((a * 2.0f).x == doctest::Approx(2.0f));
    CHECK((a * 2.0f).y == doctest::Approx(4.0f));
}

TEST_CASE("Vec2::Length returns Euclidean magnitude") {
    Vec2 v{3.0f, 4.0f};
    CHECK(v.Length() == doctest::Approx(5.0f));
}

TEST_CASE("Vec2::Normalized returns unit vector or zero for zero input") {
    Vec2 v{0.0f, 0.0f};
    auto n = v.Normalized();
    CHECK(n.x == doctest::Approx(0.0f));
    CHECK(n.y == doctest::Approx(0.0f));

    Vec2 u{3.0f, 4.0f};
    auto un = u.Normalized();
    CHECK(un.x == doctest::Approx(0.6f));
    CHECK(un.y == doctest::Approx(0.8f));
}
