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
    // 操場 trigger rect: {1240, 630, 280, 280} — centre (1380, 770).
    t.Update(Vec2{1380.0f, 770.0f});

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
    t.Update(Vec2{1380.0f, 770.0f});  // enter 操場
    t.Update(Vec2{1385.0f, 775.0f});  // still inside
    t.Update(Vec2{1390.0f, 780.0f});  // still inside

    CHECK(cap.hits == 1);
}

TEST_CASE("BuildingTracker: walking from 操場 into 體育館 fires a new event") {
    EventBus::Instance().Clear();
    EventCapture cap;
    SubscribeBuilding(cap);

    BuildingTracker t;
    t.Update(Vec2{1380.0f, 770.0f});     // 操場 centre
    t.Update(Vec2{1750.0f, 670.0f});     // 體育館 centre

    CHECK(cap.hits == 2);
    CHECK(cap.lastName == "體育館");
    CHECK(t.Current()->name == "體育館");
}

TEST_CASE("BuildingTracker: leaving into empty space clears Current() with no spurious event") {
    EventBus::Instance().Clear();
    EventCapture cap;
    SubscribeBuilding(cap);

    BuildingTracker t;
    t.Update(Vec2{1380.0f, 770.0f});     // 操場
    t.Update(Vec2{50.0f, 50.0f});        // far NW corner, no trigger rect there

    CHECK(t.Current() == nullptr);
    CHECK(cap.hits == 1);                // only the entry into 操場
}

// Regression: when trigger rects overlap, BuildingTracker::Update must
// resolve to the building whose CENTRE is nearest to the player —
// independent of array order in Buildings.h.

TEST_CASE("BuildingTracker overlap: 行政大樓 / 法學院 — nearer centre wins") {
    EventBus::Instance().Clear();
    BuildingTracker t;
    // 行政大樓 (69, 1330, 222, 200) occupies x [69,291], y [1330,1530].
    // 法學院   (37, 1520, 146, 200) occupies x [37,183], y [1520,1720].
    // Their rects overlap in x [69,183], y [1520,1530].
    // (130, 1525) is in both. Centre distances:
    //   行政大樓 centre (180,1430): d ≈ 103
    //   法學院   centre (110,1620): d ≈ 97  ← nearer
    auto* b = t.Update(Vec2{130.0f, 1525.0f});
    REQUIRE(b != nullptr);
    CHECK(b->name == "法學院");
}

TEST_CASE("BuildingTracker overlap: 大勇樓 / 志希樓 — closer point wins each way") {
    EventBus::Instance().Clear();
    BuildingTracker t1;
    // 大勇樓 (1269, 1530, 322, 180) occupies x [1269,1591], y [1530,1710].
    // 志希樓 (1218, 1700, 224, 140) occupies x [1218,1442], y [1700,1840].
    // Overlap region: x [1269,1442], y [1700,1710].
    // (1300, 1705) inside both; closer to 志希樓 centre (1330,1770).
    auto* b1 = t1.Update(Vec2{1300.0f, 1705.0f});
    REQUIRE(b1 != nullptr);
    CHECK(b1->name == "志希樓");

    EventBus::Instance().Clear();
    BuildingTracker t2;
    // (1400, 1705) inside both; closer to 大勇樓 centre (1430,1620).
    auto* b2 = t2.Update(Vec2{1400.0f, 1705.0f});
    REQUIRE(b2 != nullptr);
    CHECK(b2->name == "大勇樓");
}

TEST_CASE("BuildingTracker overlap: exact-tie midpoint resolves lexicographically") {
    EventBus::Instance().Clear();
    BuildingTracker t;
    // 行政大樓 centre = (69+222/2, 1330+200/2) = (180, 1430).
    // 法學院   centre = (37+146/2, 1520+200/2) = (110, 1620).
    // Exact midpoint = (145, 1525). Both rects contain that point and both
    // centres are exactly equidistant (√10250 ≈ 101.24). Tie-break by
    // string_view ordering: bytes 0xE6 0xB3 0x95 (法) < 0xE8 0xA1 0x8C
    // (行), so 法學院 < 行政大樓 — 法學院 wins regardless of array order.
    auto* b = t.Update(Vec2{145.0f, 1525.0f});
    REQUIRE(b != nullptr);
    CHECK(b->name == "法學院");
}
