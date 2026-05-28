#include "doctest/doctest.h"
#include "engine/render/TextBuilder.h"
#include "engine/math/Color.h"
#include "engine/math/Vec2.h"

using namespace nccu::gfx;

TEST_CASE("TextBuilder fluent setters preserve state") {
    TextBuilder t{"hi"};
    t.At(Vec2{10, 20}).Size(16).Color(Colors::Red);
    CHECK(t.GetPosition().x == doctest::Approx(10.0f));
    CHECK(t.GetPosition().y == doctest::Approx(20.0f));
    CHECK(t.GetSize() == 16);
    CHECK(t.GetColor() == Colors::Red);
}

TEST_CASE("TextBuilder defaults: pos (0,0), size 10, color black") {
    TextBuilder t{"x"};
    CHECK(t.GetSize() == 10);
    CHECK(t.GetColor() == Colors::Black);
}

TEST_CASE("TextBuilder chaining returns self by reference") {
    TextBuilder t{"x"};
    auto& ref = t.At(Vec2{1, 1}).Size(5).Color(Colors::Blue);
    CHECK(&ref == &t);
}
