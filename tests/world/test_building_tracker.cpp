#include "doctest/doctest.h"
#include "game/world/BuildingTracker.h"
#include "engine/events/EventBus.h"

#include <array>
#include <string>
#include <string_view>

/**
 * @file test_building_tracker.cpp
 * @brief 驗證 BuildingTracker 的進出建築偵測：初始為 null、進入時發一次事件、停留同棟不再發、
 *        走到另一棟發新事件、走進空地清除 Current() 且不發多餘事件；並以合成 fixture 驗證
 *        NearestContaining 在重疊／平手時的消歧義（距離近者勝、平手依名稱字典序）。
 */

using nccu::BuildingTracker;
using nccu::buildings::Building;
using nccu::buildings::kAll;
using nccu::detail::NearestContaining;
using nccu::engine::math::Vec2;

namespace {
struct EventCapture {
    int hits{0};
    std::string lastName;
};

void SubscribeBuilding(EventCapture& cap) {
    EventBus::Instance().Subscribe(EventType::EnteredBuilding,
        [&](const Event& e) { cap.hits++; cap.lastName = e.text; });
}

// 取得指定建築觸發矩形的中心，直接從 kAll 即時查找，使轉場測試能追蹤 Tiled 重新擺放而非寫死
// 座標。餵入建築自己的中心也保證距離為 0，故不論版面如何，它都確定性地贏得任何重疊。若不存在
// 回傳 {-1,-1}——呼叫端以 REQUIRE 要求其存在，使改名時大聲失敗。
Vec2 CentreOf(std::string_view name) {
    for (const auto& b : kAll)
        if (b.name == name)
            return Vec2{b.triggerRect.x + b.triggerRect.width  * 0.5f,
                        b.triggerRect.y + b.triggerRect.height * 0.5f};
    return Vec2{-1.0f, -1.0f};
}
} // namespace

// 初始的 Current() 為 null。
TEST_CASE("BuildingTracker：初始的 Current() 為 null") {
    EventBus::Instance().Clear();
    BuildingTracker t;
    CHECK(t.Current() == nullptr);
}

// 進入操場發出一次事件。
TEST_CASE("BuildingTracker：進入操場發出一次事件") {
    EventBus::Instance().Clear();
    EventCapture cap;
    SubscribeBuilding(cap);

    const Vec2 c = CentreOf("操場");
    REQUIRE(c.x >= 0.0f);            // 操場必須存在於 kAll

    BuildingTracker t;
    t.Update(c);

    CHECK(t.Current() != nullptr);
    CHECK(t.Current()->name == "操場");
    CHECK(cap.hits == 1);
    CHECK(cap.lastName == "操場");
}

// 停留同一棟內不再發事件。
TEST_CASE("BuildingTracker：停留同一棟建築內不再發事件") {
    EventBus::Instance().Clear();
    EventCapture cap;
    SubscribeBuilding(cap);

    const Vec2 c = CentreOf("操場");
    REQUIRE(c.x >= 0.0f);

    BuildingTracker t;
    t.Update(c);                              // 進入操場
    t.Update(Vec2{c.x + 5.0f, c.y + 5.0f});   // 仍在深處
    t.Update(Vec2{c.x - 5.0f, c.y - 5.0f});   // 仍在深處

    CHECK(cap.hits == 1);
}

// 從操場走進體育館發出新事件。
TEST_CASE("BuildingTracker：從操場走進體育館發出新事件") {
    EventBus::Instance().Clear();
    EventCapture cap;
    SubscribeBuilding(cap);

    const Vec2 a = CentreOf("操場");
    const Vec2 b = CentreOf("體育館");
    REQUIRE(a.x >= 0.0f);
    REQUIRE(b.x >= 0.0f);

    BuildingTracker t;
    t.Update(a);
    t.Update(b);

    CHECK(cap.hits == 2);
    CHECK(cap.lastName == "體育館");
    CHECK(t.Current()->name == "體育館");
}

// 走進空地會清除 Current() 且不發多餘事件。
TEST_CASE("BuildingTracker：走進空地會清除 Current() 且不發多餘事件") {
    EventBus::Instance().Clear();
    EventCapture cap;
    SubscribeBuilding(cap);

    const Vec2 c = CentreOf("操場");
    REQUIRE(c.x >= 0.0f);

    // 地圖左上角在任何合理版面下都不含觸發矩形；先斷言此前提，使未來的迴歸在此大聲失敗，而非
    // 在下方化為令人困惑的多餘事件計數錯誤。
    const Vec2 empty{50.0f, 50.0f};
    for (const auto& bld : kAll) REQUIRE_FALSE(bld.triggerRect.Contains(empty));

    BuildingTracker t;
    t.Update(c);          // 進入操場
    t.Update(empty);      // 走進空無一物之處

    CHECK(t.Current() == nullptr);
    CHECK(cap.hits == 1); // 只有進入操場那次
}

// 重疊／平手的消歧義是演算法（而非校園版面）的性質，故對固定的合成 fixture，透過抽出的
// nccu::detail::NearestContaining helper 演練——與 Buildings.h 無關，且不受 Tiled 重新產生影響。

// NearestContaining：矩形重疊時，中心較近者勝。
TEST_CASE("NearestContaining：矩形重疊時中心較近者勝") {
    const std::array<Building, 2> fix = {{
        {"A", {  0.0f,   0.0f, 200.0f, 200.0f}, false, false}, // 中心 (100,100)
        {"B", {100.0f, 100.0f, 200.0f, 200.0f}, false, false}, // 中心 (200,200)
    }};
    // (140,140) 同時落在兩矩形內；離 A 中心較近（約 56.6 對 約 84.9）。
    const Building* b = NearestContaining(Vec2{140.0f, 140.0f}, fix);
    REQUIRE(b != nullptr);
    CHECK(b->name == "A");
}

// 同樣的重疊，相反的點選到相反的建築。
TEST_CASE("NearestContaining：同樣的重疊下相反的點選到相反的建築") {
    const std::array<Building, 2> fix = {{
        {"A", {  0.0f,   0.0f, 200.0f, 200.0f}, false, false}, // 中心 (100,100)
        {"B", {100.0f, 100.0f, 200.0f, 200.0f}, false, false}, // 中心 (200,200)
    }};
    const Building* near_a = NearestContaining(Vec2{120.0f, 120.0f}, fix);
    const Building* near_b = NearestContaining(Vec2{180.0f, 180.0f}, fix);
    REQUIRE(near_a != nullptr);
    REQUIRE(near_b != nullptr);
    CHECK(near_a->name == "A");
    CHECK(near_b->name == "B");
}

// 完全等距的平手依名稱字典序決定。
TEST_CASE("NearestContaining：完全等距的平手依名稱字典序決定") {
    // 兩中心 (50,50) 與 (150,50)；(100,50) 落在其垂直平分線上 -> 距離平方相同 (2500)。矩形
    // 加寬，使 (100,50) 嚴格落在兩者內部（Rect::Contains 為半開，共用邊不算）。UTF-8 首位
    // 元組：法 0xE6 < 行 0xE8，故不論陣列順序，法學院都應勝出。
    const Building admin{"行政大樓", {-20.0f, 0.0f, 140.0f, 100.0f}, false, false};
    const Building law  {"法學院",   { 80.0f, 0.0f, 140.0f, 100.0f}, false, false};
    const Vec2 mid{100.0f, 50.0f};

    const std::array<Building, 2> fwd = {{admin, law}};
    const std::array<Building, 2> rev = {{law, admin}};

    const Building* a = NearestContaining(mid, fwd);
    const Building* b = NearestContaining(mid, rev);
    REQUIRE(a != nullptr);
    REQUIRE(b != nullptr);
    CHECK(a->name == "法學院");
    CHECK(b->name == "法學院");
}
