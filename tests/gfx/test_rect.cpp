#include "doctest/doctest.h"
#include "engine/math/Rect.h"
#include "engine/math/Vec2.h"

/**
 * @file test_rect.cpp
 * @brief 驗證 Rect 的建構、聚合初始化、Contains 半開區間語意與 Intersects 重疊判定。
 */

using namespace nccu::engine::math;

// 預設建構各欄位為零。
TEST_CASE("Rect：預設建構各欄位為零") {
    constexpr Rect r;
    CHECK(r.x == doctest::Approx(0.0f));
    CHECK(r.width == doctest::Approx(0.0f));
}

// 以 {x,y,w,h} 聚合初始化。
TEST_CASE("Rect：以 {x,y,w,h} 聚合初始化") {
    constexpr Rect r{10.0f, 20.0f, 30.0f, 40.0f};
    CHECK(r.x == doctest::Approx(10.0f));
    CHECK(r.width == doctest::Approx(30.0f));
}

// 點落在矩形內時 Contains 為真。
TEST_CASE("Rect::Contains 在點落在矩形內時為真") {
    Rect r{10, 10, 20, 20};
    CHECK(r.Contains(Vec2{15, 15}));
    CHECK_FALSE(r.Contains(Vec2{5, 5}));
    CHECK_FALSE(r.Contains(Vec2{31, 15}));
}

// Contains 採半開區間：含左上邊界、不含右下邊界。
TEST_CASE("Rect::Contains：含左上邊界、不含右下邊界（半開區間）") {
    Rect r{0, 0, 10, 10};
    CHECK(r.Contains(Vec2{0, 0}));
    CHECK_FALSE(r.Contains(Vec2{10, 10}));
}

// 兩矩形重疊時 Intersects 為真（對稱）。
TEST_CASE("Rect::Intersects：兩矩形重疊時為真（對稱）") {
    Rect a{0, 0, 10, 10};
    Rect b{5, 5, 10, 10};
    CHECK(a.Intersects(b));
    CHECK(b.Intersects(a));
}

// 兩矩形不相交時 Intersects 為偽。
TEST_CASE("Rect::Intersects：兩矩形不相交時為偽") {
    Rect a{0, 0, 10, 10};
    Rect b{20, 20, 5, 5};
    CHECK_FALSE(a.Intersects(b));
}
