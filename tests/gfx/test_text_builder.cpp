#include "doctest/doctest.h"
#include "engine/render/TextBuilder.h"
#include "engine/math/Color.h"
#include "engine/math/Vec2.h"

/**
 * @file test_text_builder.cpp
 * @brief 驗證 TextBuilder 的流暢式設定器會保存狀態、預設值正確，且串接時回傳自身參考。
 */

using namespace nccu::engine::render;
using namespace nccu::engine::math;

// 流暢式設定器 At/Size/Color 設定後狀態應被保存。
TEST_CASE("TextBuilder fluent setters preserve state") {
    TextBuilder t{"hi"};
    t.At(Vec2{10, 20}).Size(16).Color(Colors::Red);
    CHECK(t.GetPosition().x == doctest::Approx(10.0f));
    CHECK(t.GetPosition().y == doctest::Approx(20.0f));
    CHECK(t.GetSize() == 16);
    CHECK(t.GetColor() == Colors::Red);
}

// 預設值：位置 (0,0)、字級 10、顏色黑。
TEST_CASE("TextBuilder defaults: pos (0,0), size 10, color black") {
    TextBuilder t{"x"};
    CHECK(t.GetSize() == 10);
    CHECK(t.GetColor() == Colors::Black);
}

// 串接時各設定器回傳自身參考。
TEST_CASE("TextBuilder chaining returns self by reference") {
    TextBuilder t{"x"};
    auto& ref = t.At(Vec2{1, 1}).Size(5).Color(Colors::Blue);
    CHECK(&ref == &t);
}
