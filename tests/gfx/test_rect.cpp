#include "doctest/doctest.h"
#include "engine/math/Rect.h"
#include "engine/math/Vec2.h"

using namespace nccu::engine::math;

TEST_CASE("Rect: default-constructs to zero") {
    constexpr Rect r;
    CHECK(r.x == doctest::Approx(0.0f));
    CHECK(r.width == doctest::Approx(0.0f));
}

TEST_CASE("Rect: aggregate init") {
    constexpr Rect r{10.0f, 20.0f, 30.0f, 40.0f};
    CHECK(r.x == doctest::Approx(10.0f));
    CHECK(r.width == doctest::Approx(30.0f));
}

TEST_CASE("Rect::Contains true when point inside") {
    Rect r{10, 10, 20, 20};
    CHECK(r.Contains(Vec2{15, 15}));
    CHECK_FALSE(r.Contains(Vec2{5, 5}));
    CHECK_FALSE(r.Contains(Vec2{31, 15}));
}

TEST_CASE("Rect::Contains: edge inclusive on top-left, exclusive on bottom-right") {
    Rect r{0, 0, 10, 10};
    CHECK(r.Contains(Vec2{0, 0}));
    CHECK_FALSE(r.Contains(Vec2{10, 10}));
}

TEST_CASE("Rect::Intersects: overlapping rects") {
    Rect a{0, 0, 10, 10};
    Rect b{5, 5, 10, 10};
    CHECK(a.Intersects(b));
    CHECK(b.Intersects(a));
}

TEST_CASE("Rect::Intersects: disjoint rects") {
    Rect a{0, 0, 10, 10};
    Rect b{20, 20, 5, 5};
    CHECK_FALSE(a.Intersects(b));
}
