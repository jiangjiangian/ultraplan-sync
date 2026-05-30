#include "doctest/doctest.h"
#include "engine/render/Camera2D.h"
#include "engine/math/Vec2.h"

/**
 * @file test_camera2d_clamp.cpp
 * @brief 驗證 Camera2D::ClampToWorld：將相機 target 夾限，使視口不超出世界邊界（世界小於視口時置中）。
 */

using namespace nccu::engine::render;
using namespace nccu::engine::math;

// target 接近世界中央時不被夾限。
TEST_CASE("Camera2D::ClampToWorld：target 接近世界中央時不被夾限") {
    Camera2D c;
    c.Follow(Vec2{1000.0f, 1000.0f}, Vec2{400.0f, 225.0f});
    c.ClampToWorld(Vec2{2048.0f, 2048.0f}, Vec2{800.0f, 450.0f});
    CHECK(c.target.x == doctest::Approx(1000.0f));
    CHECK(c.target.y == doctest::Approx(1000.0f));
}

// target 在世界原點時夾限至半個視口處（避免露出邊界外）。
TEST_CASE("Camera2D::ClampToWorld：target 在世界原點時夾限至半個視口處") {
    Camera2D c;
    c.Follow(Vec2{0.0f, 0.0f}, Vec2{400.0f, 225.0f});
    c.ClampToWorld(Vec2{2048.0f, 2048.0f}, Vec2{800.0f, 450.0f});
    CHECK(c.target.x == doctest::Approx(400.0f));
    CHECK(c.target.y == doctest::Approx(225.0f));
}

// target 超出右下角時夾限至「世界 - 半視口」處。
TEST_CASE("Camera2D::ClampToWorld：target 超出右下角時夾限至「世界 - 半視口」處") {
    Camera2D c;
    c.Follow(Vec2{5000.0f, 5000.0f}, Vec2{400.0f, 225.0f});
    c.ClampToWorld(Vec2{2048.0f, 2048.0f}, Vec2{800.0f, 450.0f});
    CHECK(c.target.x == doctest::Approx(2048.0f - 400.0f));
    CHECK(c.target.y == doctest::Approx(2048.0f - 225.0f));
}

// 世界比視口還小時，target 夾限至世界中點。
TEST_CASE("Camera2D::ClampToWorld：世界比視口還小時夾限至世界中點") {
    Camera2D c;
    c.Follow(Vec2{500.0f, 100.0f}, Vec2{400.0f, 225.0f});
    c.ClampToWorld(Vec2{200.0f, 100.0f}, Vec2{800.0f, 450.0f});
    CHECK(c.target.x == doctest::Approx(100.0f));   // 200 / 2
    CHECK(c.target.y == doctest::Approx(50.0f));    // 100 / 2
}

// ClampToWorld 回傳 *this 以支援流暢式串接。
TEST_CASE("Camera2D::ClampToWorld 回傳 *this 以支援流暢式串接") {
    Camera2D c;
    auto& ret = c.Follow(Vec2{0.0f, 0.0f}, Vec2{400.0f, 225.0f})
                 .ClampToWorld(Vec2{2048.0f, 2048.0f}, Vec2{800.0f, 450.0f});
    CHECK(&ret == &c);
}
