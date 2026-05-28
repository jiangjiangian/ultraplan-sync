#include "doctest/doctest.h"
#include "engine/math/Color.h"

using namespace nccu::gfx;

TEST_CASE("Color: default-constructs to opaque black") {
    constexpr Color c;
    CHECK(c.r == 0);
    CHECK(c.g == 0);
    CHECK(c.b == 0);
    CHECK(c.a == 255);
}

TEST_CASE("Color: aggregate-init {r,g,b,a}") {
    constexpr Color c{10, 20, 30, 200};
    CHECK(c.r == 10);
    CHECK(c.g == 20);
    CHECK(c.b == 30);
    CHECK(c.a == 200);
}

TEST_CASE("Color::WithAlpha returns a new color with overridden alpha") {
    constexpr Color base{255, 0, 0, 255};
    constexpr Color faded = base.WithAlpha(128);
    CHECK(faded.r == 255);
    CHECK(faded.a == 128);
    CHECK(base.a == 255); // immutable
}

TEST_CASE("Colors:: palette has expected presets") {
    CHECK(Colors::Black.r == 0);
    CHECK(Colors::White.r == 255);
    CHECK(Colors::White.a == 255);
    CHECK(Colors::Blue.b > 200);
}

TEST_CASE("Color equality") {
    constexpr Color a{1, 2, 3, 4};
    constexpr Color b{1, 2, 3, 4};
    constexpr Color c{1, 2, 3, 5};
    CHECK(a == b);
    CHECK_FALSE(a == c);
}
