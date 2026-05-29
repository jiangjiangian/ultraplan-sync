#include "doctest/doctest.h"
#include "game/world/Physics.h"

/**
 * @file test_physics.cpp
 * @brief 驗證 physics::ResolveMove 的逐軸碰撞解析：無 collider 時接受目標位置、遠處
 *        collider 不影響、正面撞牆時該軸被擋而另一軸維持、撞角時沿自由軸滑動、多個
 *        collider 任一擋住即擋住，以及起點已重疊時的脫困條款。
 */

using nccu::engine::math::Rect;
using nccu::engine::math::Vec2;
using nccu::physics::ResolveMove;

// 無 collider -> 接受目標位置。
TEST_CASE("ResolveMove: no colliders -> desired position accepted") {
    const Vec2 prev{100.0f, 100.0f};
    const Vec2 desired{120.0f, 130.0f};
    const Vec2 size{24.0f, 24.0f};
    const std::vector<Rect> colliders;

    const Vec2 out = ResolveMove(prev, desired, size, colliders);
    CHECK(out.x == doctest::Approx(120.0f));
    CHECK(out.y == doctest::Approx(130.0f));
}

// collider 在遠處 -> 接受目標位置。
TEST_CASE("ResolveMove: collider far away -> desired position accepted") {
    const Vec2 prev{100.0f, 100.0f};
    const Vec2 desired{110.0f, 110.0f};
    const Vec2 size{24.0f, 24.0f};
    const std::vector<Rect> colliders{Rect{500.0f, 500.0f, 50.0f, 50.0f}};

    const Vec2 out = ResolveMove(prev, desired, size, colliders);
    CHECK(out.x == doctest::Approx(110.0f));
    CHECK(out.y == doctest::Approx(110.0f));
}

// 正面朝東撞牆 -> X 被擋，Y 維持。
TEST_CASE("ResolveMove: head-on wall east -> x blocked, y stays") {
    // 牆起於 x=130，故 prev.x=100 的玩家（右緣 x=124）淨空；往東走到 desired.x=110
    //（右緣 x=134）便切到牆，X 移動被擋。
    const Vec2 prev{100.0f, 100.0f};
    const Vec2 desired{110.0f, 100.0f};
    const Vec2 size{24.0f, 24.0f};
    const std::vector<Rect> colliders{Rect{130.0f, 50.0f, 100.0f, 200.0f}};

    const Vec2 out = ResolveMove(prev, desired, size, colliders);
    CHECK(out.x == doctest::Approx(100.0f));   // X 被擋
    CHECK(out.y == doctest::Approx(100.0f));   // Y 不變（增量為零）
}

// 對角撞牆角 -> 沿自由軸滑動。
TEST_CASE("ResolveMove: diagonal into wall corner -> slides on free axis") {
    SUBCASE("X axis blocked, Y free (slide downward along east wall)") {
        // 牆起於 x=130，故 prev.x=100 的玩家（右緣 124）淨空；走到 desired.x=110
        //（右緣 134）切到牆而被擋。從 prev.x 算出的 Y 步使玩家仍在牆西側，故往下的滑動
        // 應成功。
        const Vec2 prev{100.0f, 250.0f};
        const Vec2 desired{110.0f, 260.0f};
        const Vec2 size{24.0f, 24.0f};
        const std::vector<Rect> colliders{Rect{130.0f, 200.0f, 100.0f, 200.0f}};

        const Vec2 out = ResolveMove(prev, desired, size, colliders);
        CHECK(out.x == doctest::Approx(100.0f));   // 被擋
        CHECK(out.y == doctest::Approx(260.0f));   // 滑動
    }
    SUBCASE("Y axis blocked, X free (slide east along south wall)") {
        const Vec2 prev{100.0f, 100.0f};
        const Vec2 desired{110.0f, 110.0f};
        const Vec2 size{24.0f, 24.0f};
        // 牆正在玩家下方，涵蓋我們移動經過的整個 x 範圍。
        const std::vector<Rect> colliders{Rect{50.0f, 130.0f, 200.0f, 50.0f}};

        const Vec2 out = ResolveMove(prev, desired, size, colliders);
        CHECK(out.x == doctest::Approx(110.0f));   // 滑動
        CHECK(out.y == doctest::Approx(100.0f));   // 被擋
    }
}

// 多個 collider -> 任一擋住即擋住。
TEST_CASE("ResolveMove: multiple colliders -> blocked by any") {
    const Vec2 prev{100.0f, 100.0f};
    const Vec2 desired{110.0f, 100.0f};
    const Vec2 size{24.0f, 24.0f};
    const std::vector<Rect> colliders{
        Rect{500.0f, 500.0f, 10.0f, 10.0f},
        Rect{130.0f,  50.0f, 100.0f, 200.0f},  // 由這個擋住 X 步
        Rect{800.0f, 800.0f, 10.0f, 10.0f},
    };

    const Vec2 out = ResolveMove(prev, desired, size, colliders);
    CHECK(out.x == doctest::Approx(100.0f));
}

// NPC 大小的 collider 也會擋住玩家。
TEST_CASE("ResolveMove: NPC-sized collider blocks player too") {
    const Vec2 prev{100.0f, 100.0f};
    const Vec2 desired{110.0f, 100.0f};
    const Vec2 size{24.0f, 24.0f};
    // NPC 在 (130, 100)，24x24——與玩家同形。玩家在 desired.x=110 的右緣為 x=134，切到
    // 起於 x=130 的 NPC。
    const std::vector<Rect> colliders{Rect{130.0f, 100.0f, 24.0f, 24.0f}};

    const Vec2 out = ResolveMove(prev, desired, size, colliders);
    CHECK(out.x == doctest::Approx(100.0f));
}

// 起點已與 collider 重疊時可脫困。
TEST_CASE("ResolveMove: escape when prev already overlaps a collider") {
    // 生成時就疊在牆上（例如開格時 NPC 碰撞箱與玩家重疊）。若無脫困條款，兩軸測試都會失敗、
    // 玩家會卡死；我們希望移動仍能進行。
    const Vec2 prev{100.0f, 100.0f};
    const Vec2 desired{120.0f, 130.0f};
    const Vec2 size{24.0f, 24.0f};
    // 牆包住 prev——從 prev 出發的每個軸測試都失敗。
    const std::vector<Rect> colliders{Rect{80.0f, 80.0f, 60.0f, 60.0f}};

    const Vec2 out = ResolveMove(prev, desired, size, colliders);
    CHECK(out.x == doctest::Approx(120.0f));
    CHECK(out.y == doctest::Approx(130.0f));
}
