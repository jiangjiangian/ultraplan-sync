#include "doctest/doctest.h"
#include "ConsumableItem.h"
#include "HotPack.h"
#include "WaterproofSpray.h"
#include "EnergyDrink.h"
#include "GameObjectFactory.h"
#include "Player.h"
#include "EventBus.h"

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
