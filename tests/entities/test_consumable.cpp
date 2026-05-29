#include "doctest/doctest.h"
#include "game/entities/ConsumableItem.h"
#include "game/entities/HotPack.h"
#include "game/entities/WaterproofSpray.h"
#include "game/entities/EnergyDrink.h"
#include "game/controller/GameObjectFactory.h"
#include "game/quest/ItemCatalog.h"
#include "game/entities/Player.h"
#include "engine/events/EventBus.h"

#include <string>

namespace {

// Helper: subscribe a capture for ShowMessage and return the latest text.
//
// Phase 5 ASan finding: the pre-existing pattern captured `this` into a
// Subscribe lambda and relied on Attach() Clear()-ing the bus on the
// NEXT case to drop the stale handler. Between the destructor and the
// next Attach(), the lambda's `this` is dangling — any cross-case
// Publish (e.g. global EventBus::Instance() reuse from a sibling test)
// dereferences it (stack-use-after-scope). Switch to ScopedSubscribe
// so the handler lifetime is tied to the capture's own scope (the
// existing H1/B2 discipline used everywhere else in tests).
struct MessageCapture {
    int hits = 0;
    std::string lastText;
    EventBus::Subscription sub;

    void Attach() {
        EventBus::Instance().Clear();
        sub = EventBus::Instance().ScopedSubscribe(
            EventType::ShowMessage,
            [this](const Event& e) { hits++; lastText = e.text; });
    }
};

} // namespace

TEST_CASE("HotPack Consume: karma +5, rain -25 (G4), isActive false, ShowMessage") {
    Player p({0, 0});
    // G4: 暖暖包 is no longer a full reset — it sheds a FIXED -25 units. Dirty
    // the meter above 25 first so the -25 is observable (not floor-clamped).
    p.ApplyRain(12.0f, /*lethal=*/false);        // +60 (5 u/s * 12 s)
    REQUIRE(p.GetRainMeter() == doctest::Approx(60.0f));
    const int beforeKarma = p.GetKarma();

    MessageCapture cap;
    cap.Attach();

    HotPack pack({10, 20});
    CHECK(pack.IsActive() == true);
    CHECK(pack.GetPrice() == 30);

    pack.Consume(&p);

    CHECK(p.GetKarma() == beforeKarma + HotPack::kKarmaBonus);
    CHECK(p.GetRainMeter() == doctest::Approx(60.0f - HotPack::kRainRelief));  // 35
    CHECK(pack.IsActive() == false);
    CHECK(cap.hits == 1);
    CHECK(cap.lastText == "用了暖暖包，烘乾了大半的雨水，心情也好了一些。");
}

TEST_CASE("WaterproofSpray Consume: rain -35 (G4), no karma, isActive false") {
    Player p({0, 0});
    p.ApplyRain(16.0f, /*lethal=*/false);        // +80 (clamped <100)
    REQUIRE(p.GetRainMeter() == doctest::Approx(80.0f));
    const int beforeKarma = p.GetKarma();

    MessageCapture cap;
    cap.Attach();

    WaterproofSpray spray({5, 5});
    CHECK(spray.IsActive() == true);
    CHECK(spray.GetPrice() == 50);

    spray.Consume(&p);

    CHECK(p.GetKarma() == beforeKarma);          // gear — no karma delta
    CHECK(p.GetRainMeter() ==
          doctest::Approx(80.0f - WaterproofSpray::kRainRelief));  // 45
    CHECK(spray.IsActive() == false);
    CHECK(cap.hits == 1);
    CHECK(cap.lastText == "噴了防水噴霧，雨水大半都被彈開了。");
}

TEST_CASE("EnergyDrink Consume: karma +3, rain -15 (G4), isActive false") {
    Player p({0, 0});
    p.ApplyRain(8.0f, /*lethal=*/false);         // +40
    REQUIRE(p.GetRainMeter() == doctest::Approx(40.0f));
    const int beforeKarma = p.GetKarma();

    MessageCapture cap;
    cap.Attach();

    EnergyDrink drink({0, 0});
    CHECK(drink.IsActive() == true);
    CHECK(drink.GetPrice() == 40);

    drink.Consume(&p);

    CHECK(p.GetKarma() == beforeKarma + EnergyDrink::kKarmaBonus);
    CHECK(p.GetRainMeter() ==
          doctest::Approx(40.0f - EnergyDrink::kRainRelief));   // 25
    CHECK(drink.IsActive() == false);
    CHECK(cap.hits == 1);
    CHECK(cap.lastText == "喝完飲料，精神好多了，淋到的雨也擦乾了一些。");
}

TEST_CASE("Consume on null player is a no-op (no crash, item stays active)") {
    MessageCapture cap;
    cap.Attach();

    HotPack pack({0, 0});
    pack.Consume(nullptr);
    CHECK(pack.IsActive() == true);
    CHECK(cap.hits == 0);
}

TEST_CASE("Factory creates HotPack with correct dynamic type") {
    auto obj = GameObjectFactory::Create(ObjectType::HotPack, {1, 2});
    REQUIRE(obj != nullptr);
    auto* c = dynamic_cast<HotPack*>(obj.get());
    CHECK(c != nullptr);
}

TEST_CASE("Factory creates WaterproofSpray with correct dynamic type") {
    auto obj = GameObjectFactory::Create(ObjectType::WaterproofSpray, {1, 2});
    REQUIRE(obj != nullptr);
    auto* c = dynamic_cast<WaterproofSpray*>(obj.get());
    CHECK(c != nullptr);
}

TEST_CASE("Factory creates EnergyDrink with correct dynamic type") {
    auto obj = GameObjectFactory::Create(ObjectType::EnergyDrink, {1, 2});
    REQUIRE(obj != nullptr);
    auto* c = dynamic_cast<EnergyDrink*>(obj.get());
    CHECK(c != nullptr);
}

// Item 2(a) regression: picking a consumable up off the world now ADDS it
// to the bag (count map) under its itemId and applies NO effect — the
// effect is deferred to USE-from-bag. Pre-change OnPickup/Interact fired
// Consume() (karma + message) on the spot.
TEST_CASE("Item 2a: ConsumableItem pickup adds to the bag, applies NO effect") {
    Player p{nccu::gfx::Vec2{0, 0}};
    const int k0 = p.GetKarma();
    MessageCapture cap;
    cap.Attach();

    HotPack pack{nccu::gfx::Vec2{0, 0}};
    REQUIRE(p.ConsumableCount("HotPack") == 0);
    pack.OnPickup(&p);

    CHECK(p.ConsumableCount("HotPack") == 1);  // landed in the bag
    CHECK(p.GetKarma() == k0);                 // NO karma applied on pickup
    CHECK(p.GetRainMeter() == doctest::Approx(0.0f));
    CHECK(cap.hits == 0);                      // no flavour message on pickup
    CHECK_FALSE(pack.IsActive());              // object consumed (sweep removes)

    // Idempotency: a second pickup on the now-inert object does nothing.
    pack.OnPickup(&p);
    CHECK(p.ConsumableCount("HotPack") == 1);

    // Interact() routes through the same Collect path.
    EnergyDrink drink{nccu::gfx::Vec2{0, 0}};
    drink.Interact(&p);
    CHECK(p.ConsumableCount("EnergyDrink") == 1);
    CHECK(p.GetKarma() == k0);                 // still no effect on pickup
}

// Item 2(b) regression: using a held consumable from the bag applies the
// SAME effect Consume() did (identical karma + identical ShowMessage) and
// the caller decrements the count. ApplyConsumableEffect is the shared
// effect; ConsumeOne is the spend.
TEST_CASE("Item 2b: use-from-bag applies the effect + the catalog message") {
    Player p{nccu::gfx::Vec2{0, 0}};
    p.AddConsumable("HotPack").AddConsumable("HotPack");
    p.ApplyRain(12.0f, /*lethal=*/false);      // +60, above the -25 dry
    REQUIRE(p.GetRainMeter() == doctest::Approx(60.0f));
    const int k0 = p.GetKarma();

    MessageCapture cap;
    cap.Attach();

    // G4: ApplyConsumableEffect mirrors HotPack::Consume EXACTLY — +5 karma
    // and -25 rain (no longer a full reset).
    nccu::ApplyConsumableEffect(EventBus::Instance(), p, "HotPack");
    CHECK(p.GetKarma() == k0 + HotPack::kKarmaBonus);   // +5, same as Consume
    CHECK(p.GetRainMeter() ==
          doctest::Approx(60.0f - HotPack::kRainRelief));  // 35, same as Consume
    CHECK(cap.hits == 1);
    CHECK(cap.lastText == "用了暖暖包，烘乾了大半的雨水，心情也好了一些。");

    // The controller spends one after applying — model it here.
    CHECK(p.ConsumeOne("HotPack"));
    CHECK(p.ConsumableCount("HotPack") == 1);

    // EnergyDrink effect mirrors its Consume body too (+3 karma, -15 rain).
    Player q{nccu::gfx::Vec2{0, 0}};
    q.ApplyRain(8.0f, /*lethal=*/false);       // +40
    const int qk = q.GetKarma();
    MessageCapture cap2;
    cap2.Attach();
    nccu::ApplyConsumableEffect(EventBus::Instance(), q, "EnergyDrink");
    CHECK(q.GetKarma() == qk + EnergyDrink::kKarmaBonus);  // +3
    CHECK(q.GetRainMeter() ==
          doctest::Approx(40.0f - EnergyDrink::kRainRelief));  // 25
    CHECK(cap2.lastText == "喝完飲料，精神好多了，淋到的雨也擦乾了一些。");
}

// G4 — the dedicated rain-effect table for every usable consumable. Pins the
// EXACT -units per item (the owner-set table) + the generic 小吃 path that
// has no entity class, and the parity between the entity Consume body and
// ApplyConsumableEffect. Revert-verify: drop the DrainRainBy calls and the
// rain CHECKs fail; revert IsUsableConsumable's food set and the food CHECK
// fails (the effect no-ops on a non-usable id).
TEST_CASE("G4: consumable rain-relief table (use-from-bag)") {
    struct Row { const char* id; float relief; bool usable; };
    const Row rows[] = {
        {"WaterproofSpray", 35.0f, true},
        {"HotPack",         25.0f, true},
        {"EnergyDrink",     15.0f, true},
        {"EggCake",         15.0f, true},
        {"FlowerTea",       15.0f, true},
        {"Takoyaki",        15.0f, true},
        {"Donation",         0.0f, false},   // charity gift — not usable, no rain
    };
    for (const Row& r : rows) {
        CHECK(nccu::IsUsableConsumable(r.id) == r.usable);
        Player p{nccu::gfx::Vec2{0, 0}};
        p.ApplyRain(18.0f, /*lethal=*/false);          // +90 (clamped <100)
        REQUIRE(p.GetRainMeter() == doctest::Approx(90.0f));
        MessageCapture cap; cap.Attach();
        nccu::ApplyConsumableEffect(EventBus::Instance(), p, r.id);
        CHECK(p.GetRainMeter() == doctest::Approx(90.0f - r.relief));
    }

    // Floor clamp: a small meter cannot go below 0 (no negative rain).
    Player low{nccu::gfx::Vec2{0, 0}};
    low.ApplyRain(2.0f, /*lethal=*/false);             // +10
    nccu::ApplyConsumableEffect(EventBus::Instance(), low, "WaterproofSpray");  // -35 -> clamp 0
    CHECK(low.GetRainMeter() == doctest::Approx(0.0f));

    // DrainRainBy unit: fixed subtraction, clamped, no teleport.
    Player u{nccu::gfx::Vec2{0, 0}};
    u.ApplyRain(10.0f, /*lethal=*/false);              // +50
    u.DrainRainBy(20.0f);
    CHECK(u.GetRainMeter() == doctest::Approx(30.0f));
    u.DrainRainBy(100.0f);                             // over-drain -> clamp 0
    CHECK(u.GetRainMeter() == doctest::Approx(0.0f));
}

TEST_CASE("Item 2: IsUsableConsumable classifies the bag rows") {
    CHECK(nccu::IsUsableConsumable("HotPack"));
    CHECK(nccu::IsUsableConsumable("EnergyDrink"));
    CHECK(nccu::IsUsableConsumable("WaterproofSpray"));
    CHECK_FALSE(nccu::IsUsableConsumable(nccu::kItemMoney));
    CHECK_FALSE(nccu::IsUsableConsumable(nccu::kItemTrueUmbrella));
    CHECK_FALSE(nccu::IsUsableConsumable("NotAThing"));
}
