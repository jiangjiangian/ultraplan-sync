#include "doctest/doctest.h"
#include "entities/ConsumableItem.h"
#include "entities/HotPack.h"
#include "entities/WaterproofSpray.h"
#include "entities/EnergyDrink.h"
#include "controller/GameObjectFactory.h"
#include "quest/ItemCatalog.h"
#include "entities/Player.h"
#include "controller/EventBus.h"

#include <string>

namespace {

// Helper: subscribe a capture for ShowMessage and return the latest text.
struct MessageCapture {
    int hits = 0;
    std::string lastText;

    void Attach() {
        EventBus::Instance().Clear();
        EventBus::Instance().Subscribe(EventType::ShowMessage,
            [this](const Event& e) { hits++; lastText = e.text; });
    }
};

} // namespace

TEST_CASE("HotPack Consume: karma +5, rainMeter reset, isActive false, ShowMessage text") {
    Player p({0, 0});
    // Player starts with rainMeter=0; resetRainMeter() must leave it at 0.
    // Until a public setter exists this test only confirms the call is safe
    // and post-condition still holds.
    const int beforeKarma = p.GetKarma();

    MessageCapture cap;
    cap.Attach();

    HotPack pack({10, 20});
    CHECK(pack.IsActive() == true);
    CHECK(pack.GetPrice() == 30);

    pack.Consume(&p);

    CHECK(p.GetKarma() == beforeKarma + HotPack::kKarmaBonus);
    CHECK(p.GetRainMeter() == doctest::Approx(0.0f));
    CHECK(pack.IsActive() == false);
    CHECK(cap.hits == 1);
    CHECK(cap.lastText == "用了暖暖包，雨水蒸發了，心情也好了一些。");
}

TEST_CASE("WaterproofSpray Consume: no karma change, isActive false, ShowMessage text") {
    Player p({0, 0});
    const int beforeKarma = p.GetKarma();

    MessageCapture cap;
    cap.Attach();

    WaterproofSpray spray({5, 5});
    CHECK(spray.IsActive() == true);
    CHECK(spray.GetPrice() == 50);

    spray.Consume(&p);

    CHECK(p.GetKarma() == beforeKarma);          // mood-only — no karma delta
    CHECK(spray.IsActive() == false);
    CHECK(cap.hits == 1);
    CHECK(cap.lastText == "噴了防水噴霧，接下來這場雨就無感了。");
}

TEST_CASE("EnergyDrink Consume: karma +3, isActive false, ShowMessage text") {
    Player p({0, 0});
    const int beforeKarma = p.GetKarma();

    MessageCapture cap;
    cap.Attach();

    EnergyDrink drink({0, 0});
    CHECK(drink.IsActive() == true);
    CHECK(drink.GetPrice() == 40);

    drink.Consume(&p);

    CHECK(p.GetKarma() == beforeKarma + EnergyDrink::kKarmaBonus);
    CHECK(drink.IsActive() == false);
    CHECK(cap.hits == 1);
    CHECK(cap.lastText == "喝完飲料，精神好多了。下次小考應該能撐住。");
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
    p.ApplyRain(2.0f, /*lethal=*/false);       // dirty the rain meter
    REQUIRE(p.GetRainMeter() > 0.0f);
    const int k0 = p.GetKarma();

    MessageCapture cap;
    cap.Attach();

    nccu::ApplyConsumableEffect(p, "HotPack");
    CHECK(p.GetKarma() == k0 + HotPack::kKarmaBonus);   // +5, same as Consume
    CHECK(p.GetRainMeter() == doctest::Approx(0.0f));   // rain reset, same
    CHECK(cap.hits == 1);
    CHECK(cap.lastText == "用了暖暖包，雨水蒸發了，心情也好了一些。");

    // The controller spends one after applying — model it here.
    CHECK(p.ConsumeOne("HotPack"));
    CHECK(p.ConsumableCount("HotPack") == 1);

    // EnergyDrink effect mirrors its Consume body too.
    Player q{nccu::gfx::Vec2{0, 0}};
    const int qk = q.GetKarma();
    MessageCapture cap2;
    cap2.Attach();
    nccu::ApplyConsumableEffect(q, "EnergyDrink");
    CHECK(q.GetKarma() == qk + EnergyDrink::kKarmaBonus);  // +3
    CHECK(cap2.lastText == "喝完飲料，精神好多了。下次小考應該能撐住。");
}

TEST_CASE("Item 2: IsUsableConsumable classifies the bag rows") {
    CHECK(nccu::IsUsableConsumable("HotPack"));
    CHECK(nccu::IsUsableConsumable("EnergyDrink"));
    CHECK(nccu::IsUsableConsumable("WaterproofSpray"));
    CHECK_FALSE(nccu::IsUsableConsumable(nccu::kItemMoney));
    CHECK_FALSE(nccu::IsUsableConsumable(nccu::kItemTrueUmbrella));
    CHECK_FALSE(nccu::IsUsableConsumable("NotAThing"));
}
