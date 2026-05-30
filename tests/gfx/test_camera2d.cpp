#include "doctest/doctest.h"
#include "engine/render/Camera2D.h"
#include "engine/math/Vec2.h"

/**
 * @file test_camera2d.cpp
 * @brief 驗證 Camera2D 的預設值、Follow 對位，以及 WithZoom/WithRotation 的流暢式設定器。
 */

using namespace nccu::engine::render;
using namespace nccu::engine::math;

// 預設值：offset/target/rotation 皆為零，zoom 為 1.0。
TEST_CASE("Camera2D 預設值：offset/target/rotation 皆為零，zoom 為 1.0") {
    Camera2D c;
    CHECK(c.offset.x   == doctest::Approx(0.0f));
    CHECK(c.offset.y   == doctest::Approx(0.0f));
    CHECK(c.target.x   == doctest::Approx(0.0f));
    CHECK(c.target.y   == doctest::Approx(0.0f));
    CHECK(c.rotation   == doctest::Approx(0.0f));
    CHECK(c.zoom       == doctest::Approx(1.0f));
}

// Follow 將 target 設為世界目標、offset 設為螢幕中心，並回傳 *this。
TEST_CASE("Camera2D::Follow 將 target 設為世界目標、offset 設為螢幕中心") {
    Camera2D c;
    auto& ret = c.Follow(Vec2{1000.0f, 500.0f}, Vec2{400.0f, 225.0f});
    CHECK(&ret == &c);
    CHECK(c.target.x == doctest::Approx(1000.0f));
    CHECK(c.target.y == doctest::Approx(500.0f));
    CHECK(c.offset.x == doctest::Approx(400.0f));
    CHECK(c.offset.y == doctest::Approx(225.0f));
}

// WithZoom 與 WithRotation 為可串接的流暢式設定器。
TEST_CASE("Camera2D::WithZoom 與 WithRotation 為可串接的流暢式設定器") {
    Camera2D c;
    auto& ret = c.WithZoom(2.0f).WithRotation(45.0f);
    CHECK(&ret == &c);
    CHECK(c.zoom     == doctest::Approx(2.0f));
    CHECK(c.rotation == doctest::Approx(45.0f));
}
