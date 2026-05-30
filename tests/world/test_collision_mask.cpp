#include "doctest/doctest.h"
#include "game/world/CollisionMask.h"
#include "game/world/Physics.h"

/**
 * @file test_collision_mask.cpp
 * @brief 驗證 CollisionMask 像素級地形遮罩：空遮罩處處可走、Solid 正確回報已塗格、
 *        BlockedBox 偵測踩在牆上的腳印，以及 ResolveMove 在地形遮罩下逐軸阻擋／滑動，
 *        並能與動態 collider 同時作用。
 */

using nccu::CollisionMask;
using nccu::engine::math::Rect;
using nccu::engine::math::Vec2;
using nccu::physics::ResolveMove;

namespace {
// 64x64 遮罩，在欄 [30,40) 對每一列都有一道實心垂直牆（10 px 厚，足以被 4 px 腳印掃描捕捉）。
CollisionMask MakeWallMask() {
    constexpr int W = 64, H = 64;
    std::vector<std::uint8_t> g(static_cast<std::size_t>(W) * H, 0);
    for (int y = 0; y < H; ++y)
        for (int x = 30; x < 40; ++x)
            g[static_cast<std::size_t>(y) * W + x] = 1;
    return CollisionMask{W, H, std::move(g)};
}
} // namespace

// 空遮罩處處可走。
TEST_CASE("CollisionMask：空遮罩處處可走") {
    const CollisionMask m;
    CHECK(m.Empty());
    CHECK_FALSE(m.Solid(0, 0));
    CHECK_FALSE(m.Solid(1000, 1000));
    CHECK_FALSE(m.BlockedBox(10.0f, 10.0f, 24.0f, 24.0f));
}

// Solid() 回報已塗的格。
TEST_CASE("CollisionMask：Solid() 回報已塗的格") {
    const CollisionMask m = MakeWallMask();
    CHECK_FALSE(m.Empty());
    CHECK_FALSE(m.Solid(29, 10));
    CHECK(m.Solid(30, 10));
    CHECK(m.Solid(39, 63));
    CHECK_FALSE(m.Solid(40, 10));
    // 出界取樣會夾到邊緣（此處：可走的邊界）。
    CHECK_FALSE(m.Solid(-5, -5));
    CHECK_FALSE(m.Solid(99999, 0));
}

// BlockedBox 偵測壓在牆上的腳印。
TEST_CASE("CollisionMask：BlockedBox 偵測壓在牆上的腳印") {
    const CollisionMask m = MakeWallMask();
    CHECK_FALSE(m.BlockedBox(0.0f, 0.0f, 24.0f, 24.0f));   // 牆的西側
    CHECK(m.BlockedBox(20.0f, 0.0f, 24.0f, 24.0f));        // 跨在牆上
    CHECK(m.BlockedBox(32.0f, 0.0f, 4.0f, 4.0f));          // 完全在牆內
    CHECK_FALSE(m.BlockedBox(44.0f, 0.0f, 16.0f, 16.0f));  // 牆的東側
}

// ResolveMove：mask 為 nullptr 時保持「只看矩形」的舊行為。
TEST_CASE("ResolveMove：mask 為 nullptr 時維持只看矩形的行為") {
    // 新的 mask 參數預設為 nullptr——既有呼叫端與舊有的 physics 測試行為須與先前完全相同。
    const Vec2 prev{0.0f, 0.0f};
    const Vec2 desired{10.0f, 0.0f};
    const Vec2 size{24.0f, 24.0f};
    const std::vector<Rect> none;
    const Vec2 out = ResolveMove(prev, desired, size, none, nullptr);
    CHECK(out.x == doctest::Approx(10.0f));
    CHECK(out.y == doctest::Approx(0.0f));
}

// 地形遮罩擋住 X 步，Y 仍可滑動。
TEST_CASE("ResolveMove：地形遮罩擋住 X 步，Y 仍可滑動") {
    const CollisionMask m = MakeWallMask();
    const std::vector<Rect> none;
    const Vec2 size{24.0f, 24.0f};
    // prev.x=5 時玩家右緣在 x=29（淨空）；往東走到 x=10 使右緣到 34，落入 [30,40) 牆內 ->
    // X 被擋。從未移動的 X 算出的 Y 增量無牆 -> 可滑動。
    const Vec2 prev{5.0f, 10.0f};
    const Vec2 desired{10.0f, 18.0f};
    const Vec2 out = ResolveMove(prev, desired, size, none, &m);
    CHECK(out.x == doctest::Approx(5.0f));    // 被遮罩擋住
    CHECK(out.y == doctest::Approx(18.0f));   // 自由軸滑動
}

// 穿過遮罩的淨空通道可行走。
TEST_CASE("ResolveMove：穿過遮罩的淨空通道可行走") {
    const CollisionMask m = MakeWallMask();
    const std::vector<Rect> none;
    const Vec2 size{20.0f, 20.0f};
    // 完全位於牆的東側（x>=40）：移動暢通無阻。
    const Vec2 prev{42.0f, 2.0f};
    const Vec2 desired{43.0f, 5.0f};
    const Vec2 out = ResolveMove(prev, desired, size, none, &m);
    CHECK(out.x == doctest::Approx(43.0f));
    CHECK(out.y == doctest::Approx(5.0f));
}

// 動態矩形與地形遮罩同時作用。
TEST_CASE("ResolveMove：動態矩形與地形遮罩同時作用") {
    const CollisionMask m = MakeWallMask();
    const Vec2 size{24.0f, 24.0f};
    // 往下（y）朝某動態角色的碰撞箱移動：此欄遮罩淨空，但矩形擋住 Y 步。
    const std::vector<Rect> dyn{Rect{0.0f, 30.0f, 24.0f, 10.0f}};
    const Vec2 prev{0.0f, 0.0f};
    const Vec2 desired{0.0f, 12.0f};
    const Vec2 out = ResolveMove(prev, desired, size, dyn, &m);
    CHECK(out.y == doctest::Approx(0.0f));   // 被動態矩形擋住
}
