#include "doctest/doctest.h"
#include "engine/math/Color.h"

/**
 * @file test_color.cpp
 * @brief 驗證 Color 的預設值、聚合初始化、WithAlpha 不可變語意、預設調色盤與相等比較。
 */

using namespace nccu::engine::math;

// 預設建構為不透明黑色。
TEST_CASE("Color：預設建構為不透明黑色") {
    constexpr Color c;
    CHECK(c.r == 0);
    CHECK(c.g == 0);
    CHECK(c.b == 0);
    CHECK(c.a == 255);
}

// 以 {r,g,b,a} 聚合初始化。
TEST_CASE("Color：以 {r,g,b,a} 聚合初始化") {
    constexpr Color c{10, 20, 30, 200};
    CHECK(c.r == 10);
    CHECK(c.g == 20);
    CHECK(c.b == 30);
    CHECK(c.a == 200);
}

// WithAlpha 回傳覆寫 alpha 的新 Color，原物件不變。
TEST_CASE("Color::WithAlpha 回傳覆寫 alpha 的新 Color，原物件不變") {
    constexpr Color base{255, 0, 0, 255};
    constexpr Color faded = base.WithAlpha(128);
    CHECK(faded.r == 255);
    CHECK(faded.a == 128);
    CHECK(base.a == 255); // 原物件不可變
}

// 預設調色盤含預期的常見顏色。
TEST_CASE("Colors:: 調色盤含預期的常見顏色") {
    CHECK(Colors::Black.r == 0);
    CHECK(Colors::White.r == 255);
    CHECK(Colors::White.a == 255);
    CHECK(Colors::Blue.b > 200);
}

// 相等運算子逐分量比較。
TEST_CASE("Color 相等運算子逐分量比較") {
    constexpr Color a{1, 2, 3, 4};
    constexpr Color b{1, 2, 3, 4};
    constexpr Color c{1, 2, 3, 5};
    CHECK(a == b);
    CHECK_FALSE(a == c);
}
