#include "doctest/doctest.h"
#include "game/world/BuildingTracker.h"
#include "engine/events/EventBus.h"

#include <array>
#include <string>
#include <string_view>

using nccu::BuildingTracker;
using nccu::buildings::Building;
using nccu::buildings::kAll;
using nccu::detail::NearestContaining;
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

// Centre of the named building's trigger rect, looked up live in kAll so
// the transition tests track Tiled re-placements instead of hardcoding
// coordinates. Feeding a building's OWN centre also guarantees distance 0,
// so it wins any overlap deterministically regardless of layout. Returns
// {-1,-1} if absent — callers REQUIRE presence so a rename fails loudly.
Vec2 CentreOf(std::string_view name) {
    for (const auto& b : kAll)
        if (b.name == name)
            return Vec2{b.triggerRect.x + b.triggerRect.width  * 0.5f,
                        b.triggerRect.y + b.triggerRect.height * 0.5f};
    return Vec2{-1.0f, -1.0f};
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

    const Vec2 c = CentreOf("操場");
    REQUIRE(c.x >= 0.0f);            // 操場 must exist in kAll

    BuildingTracker t;
    t.Update(c);

    CHECK(t.Current() != nullptr);
    CHECK(t.Current()->name == "操場");
    CHECK(cap.hits == 1);
    CHECK(cap.lastName == "操場");
}

TEST_CASE("BuildingTracker: staying inside same building fires no further events") {
    EventBus::Instance().Clear();
    EventCapture cap;
    SubscribeBuilding(cap);

    const Vec2 c = CentreOf("操場");
    REQUIRE(c.x >= 0.0f);

    BuildingTracker t;
    t.Update(c);                              // enter 操場
    t.Update(Vec2{c.x + 5.0f, c.y + 5.0f});   // still deep inside
    t.Update(Vec2{c.x - 5.0f, c.y - 5.0f});   // still deep inside

    CHECK(cap.hits == 1);
}

TEST_CASE("BuildingTracker: walking from 操場 into 體育館 fires a new event") {
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

TEST_CASE("BuildingTracker: leaving into empty space clears Current() with no spurious event") {
    EventBus::Instance().Clear();
    EventCapture cap;
    SubscribeBuilding(cap);

    const Vec2 c = CentreOf("操場");
    REQUIRE(c.x >= 0.0f);

    // The map's top-left corner holds no trigger rect in any sane layout;
    // assert that precondition so a future regression fails loudly here
    // rather than as a confusing spurious-event miscount below.
    const Vec2 empty{50.0f, 50.0f};
    for (const auto& bld : kAll) REQUIRE_FALSE(bld.triggerRect.Contains(empty));

    BuildingTracker t;
    t.Update(c);          // enter 操場
    t.Update(empty);      // walk off into nothing

    CHECK(t.Current() == nullptr);
    CHECK(cap.hits == 1); // only the entry into 操場
}

// The overlap / tie-break disambiguation is a property of the algorithm,
// not of the campus layout, so it is exercised against fixed synthetic
// fixtures via the extracted nccu::detail::NearestContaining helper —
// independent of Buildings.h and immune to Tiled regeneration.

TEST_CASE("NearestContaining: overlapping rects — nearer centre wins") {
    const std::array<Building, 2> fix = {{
        {"A", {  0.0f,   0.0f, 200.0f, 200.0f}, false, false}, // centre (100,100)
        {"B", {100.0f, 100.0f, 200.0f, 200.0f}, false, false}, // centre (200,200)
    }};
    // (140,140) is in both rects; nearer A's centre (≈56.6 vs ≈84.9).
    const Building* b = NearestContaining(Vec2{140.0f, 140.0f}, fix);
    REQUIRE(b != nullptr);
    CHECK(b->name == "A");
}

TEST_CASE("NearestContaining: same overlap, opposite points pick opposite buildings") {
    const std::array<Building, 2> fix = {{
        {"A", {  0.0f,   0.0f, 200.0f, 200.0f}, false, false}, // centre (100,100)
        {"B", {100.0f, 100.0f, 200.0f, 200.0f}, false, false}, // centre (200,200)
    }};
    const Building* near_a = NearestContaining(Vec2{120.0f, 120.0f}, fix);
    const Building* near_b = NearestContaining(Vec2{180.0f, 180.0f}, fix);
    REQUIRE(near_a != nullptr);
    REQUIRE(near_b != nullptr);
    CHECK(near_a->name == "A");
    CHECK(near_b->name == "B");
}

TEST_CASE("NearestContaining: exact equidistant tie breaks lexicographically by name") {
    // Centres (50,50) and (150,50); (100,50) sits on their perpendicular
    // bisector → identical squared distance (2500). Rects are widened so
    // (100,50) is strictly interior to BOTH (Rect::Contains is half-open,
    // so a shared edge would not count). UTF-8 first byte: 法 0xE6 < 行
    // 0xE8, so 法學院 must win regardless of array order.
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
