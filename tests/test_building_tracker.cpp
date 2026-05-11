#include "doctest/doctest.h"
#include "BuildingTracker.h"
#include "Buildings.h"
#include "EventBus.h"
#include "gfx/Vec2.h"

using nccu::BuildingTracker;
using nccu::buildings::kAll;
using nccu::gfx::Vec2;

namespace {
struct EventCapture {
    int hits{0};
    std::string lastName;
};

void SubscribeBuilding(EventCapture& cap) {
    EventBus::Instance().Subscribe(EventType::EnteredBuilding,
        [&](const Event& e) { cap.hits++; cap.lastName = e.text; });
}
} // namespace

TEST_CASE("BuildingTracker: initial Current() is null") {
    EventBus::Instance().Clear();
    BuildingTracker t;
    CHECK(t.Current() == nullptr);
}

TEST_CASE("BuildingTracker: entering 操場 fires one event") {
    EventBus::Instance().Clear();
    EventCapture cap;
    SubscribeBuilding(cap);

    BuildingTracker t;
    // 操場 trigger rect: {720, 180, 360, 360} — centre ≈ (900, 360)
    t.Update(Vec2{900.0f, 360.0f});

    CHECK(t.Current() != nullptr);
    CHECK(t.Current()->name == "操場");
    CHECK(cap.hits == 1);
    CHECK(cap.lastName == "操場");
}

TEST_CASE("BuildingTracker: staying inside same building fires no further events") {
    EventBus::Instance().Clear();
    EventCapture cap;
    SubscribeBuilding(cap);

    BuildingTracker t;
    t.Update(Vec2{900.0f, 360.0f});  // enter 操場
    t.Update(Vec2{905.0f, 365.0f});  // still inside
    t.Update(Vec2{910.0f, 370.0f});  // still inside

    CHECK(cap.hits == 1);
}

TEST_CASE("BuildingTracker: walking from 操場 into 體育館 fires a new event") {
    EventBus::Instance().Clear();
    EventCapture cap;
    SubscribeBuilding(cap);

    BuildingTracker t;
    t.Update(Vec2{900.0f, 360.0f});      // 操場
    t.Update(Vec2{1450.0f, 360.0f});     // 體育館 centre

    CHECK(cap.hits == 2);
    CHECK(cap.lastName == "體育館");
    CHECK(t.Current()->name == "體育館");
}

TEST_CASE("BuildingTracker: leaving into empty space clears Current() with no spurious event") {
    EventBus::Instance().Clear();
    EventCapture cap;
    SubscribeBuilding(cap);

    BuildingTracker t;
    t.Update(Vec2{900.0f, 360.0f});      // 操場
    t.Update(Vec2{50.0f, 50.0f});        // top-left corner, no building

    CHECK(t.Current() == nullptr);
    CHECK(cap.hits == 1);                // only the entry into 操場
}

// Regression: when trigger rects overlap, BuildingTracker::Update must
// resolve to the building whose CENTRE is nearest to the player —
// independent of array order in Buildings.h.

TEST_CASE("BuildingTracker overlap: 行政大樓 / 新聞館 — nearer centre wins") {
    EventBus::Instance().Clear();
    BuildingTracker t;
    // (475, 1475) lies inside 行政大樓 (200,1200,280,280) AND
    // 新聞館 (410,1410,240,240). Centre distances:
    //   行政大樓 centre (340,1340): d ≈ 191
    //   新聞館   centre (530,1530): d ≈ 78  ← nearer
    auto* b = t.Update(Vec2{475.0f, 1475.0f});
    REQUIRE(b != nullptr);
    CHECK(b->name == "新聞館");
}

TEST_CASE("BuildingTracker overlap: 風雩樓 / 風雩走廊 — closer point wins each way") {
    EventBus::Instance().Clear();
    BuildingTracker t1;
    // (1190, 1580) inside both; 風雩走廊 centre (1280,1620) closer than
    // 風雩樓 centre (1080,1490).
    auto* b1 = t1.Update(Vec2{1190.0f, 1580.0f});
    REQUIRE(b1 != nullptr);
    CHECK(b1->name == "風雩走廊");

    EventBus::Instance().Clear();
    BuildingTracker t2;
    // (1190, 1525) inside both; this time 風雩樓 centre is closer.
    auto* b2 = t2.Update(Vec2{1190.0f, 1525.0f});
    REQUIRE(b2 != nullptr);
    CHECK(b2->name == "風雩樓");
}

TEST_CASE("BuildingTracker overlap: 集英樓 / 果夫樓 — nearer centre wins") {
    EventBus::Instance().Clear();
    BuildingTracker t;
    // (1180, 1760) inside both. 集英樓 centre (1270,1760) closer than
    // 果夫樓 centre (1080,1740).
    auto* b = t.Update(Vec2{1180.0f, 1760.0f});
    REQUIRE(b != nullptr);
    CHECK(b->name == "集英樓");
}
