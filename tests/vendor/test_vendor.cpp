#include "doctest/doctest.h"
#include "engine/events/EventBus.h"
#include "game/controller/GameObjectFactory.h"
#include "game/entities/Player.h"
#include "game/vendor/Vendor.h"
#include "game/vendor/VendorConfig.h"

#include <string>
#include <vector>

namespace {

// Capture both flavours of event Vendor emits — purchase chatter and the
// inventory-side PickupAcquired payload. Built fresh per test so EventBus
// state doesn't leak across cases.
struct VendorCapture {
    int         msgHits     = 0;
    std::string lastMsg;
    int         pickupHits  = 0;
    std::string lastPickup;

    void Attach() {
        EventBus::Instance().Clear();
        EventBus::Instance().Subscribe(EventType::ShowMessage,
            [this](const Event& e) { msgHits++; lastMsg = e.text; });
        EventBus::Instance().Subscribe(EventType::PickupAcquired,
            [this](const Event& e) { pickupHits++; lastPickup = e.text; });
    }
};

VendorConfig MakeOneItemStall() {
    return VendorConfig{
        "市集攤主",
        "歡迎光臨",
        {{"HotPack", 30}}
    };
}

} // namespace

TEST_CASE("Vendor: ctor seeds NPC dialog from greeting + stock entries") {
    Vendor v({0, 0}, MakeOneItemStall());
    // Greeting + one stock entry = two cyclable lines.
    CHECK(v.DialogLineCount() == 2);
    CHECK(v.CurrentLineText() == "歡迎光臨");
    CHECK(v.Config().name == "市集攤主");
    CHECK(v.Config().stock.size() == 1);
    CHECK(v.Config().stock[0].itemId == "HotPack");
    CHECK(v.Config().stock[0].price  == 30);
}

TEST_CASE("Vendor::TryBuy success: deducts money, emits ShowMessage + PickupAcquired") {
    Player p({0, 0});
    const int startMoney = p.GetMoney();
    REQUIRE(startMoney >= 30);

    VendorCapture cap;
    cap.Attach();

    Vendor v({100, 100}, MakeOneItemStall());
    const bool ok = v.TryBuy(&p, 0);
    CHECK(ok);
    CHECK(p.GetMoney() == startMoney - 30);

    // Both events should have fired exactly once.
    CHECK(cap.msgHits    == 1);
    // Item 5b: the toast shows the 中文 catalog name + price spent +
    // remaining balance (start 100 - 30 = 70), not the raw itemId.
    CHECK(cap.lastMsg    == "買了暖暖包，花了 30 元（剩 70 元）");
    CHECK(cap.pickupHits == 1);
    CHECK(cap.lastPickup == "HotPack");   // inventory event still the id
}

TEST_CASE("Vendor::TryBuy insufficient funds: no deduction, ShowMessage 錢不夠, no pickup event") {
    Player p({0, 0});
    // Drain the wallet so even the cheapest item is unreachable.
    while (p.DeductMoney(1)) { /* noop */ }
    const int startMoney = p.GetMoney();
    CHECK(startMoney == 0);

    VendorCapture cap;
    cap.Attach();

    Vendor v({0, 0}, MakeOneItemStall());
    const bool ok = v.TryBuy(&p, 0);
    CHECK_FALSE(ok);
    CHECK(p.GetMoney() == startMoney);    // unchanged
    CHECK(cap.msgHits    == 1);
    CHECK(cap.lastMsg    == "你錢不夠");
    CHECK(cap.pickupHits == 0);
}

TEST_CASE("Vendor::TryBuy out-of-bounds index returns false and emits no events") {
    Player p({0, 0});
    const int startMoney = p.GetMoney();

    VendorCapture cap;
    cap.Attach();

    Vendor v({0, 0}, MakeOneItemStall());
    // Stock has size 1, valid index is 0 — 1 and 99 both overflow.
    CHECK_FALSE(v.TryBuy(&p, 1));
    CHECK_FALSE(v.TryBuy(&p, 99));
    CHECK(p.GetMoney()   == startMoney);
    CHECK(cap.msgHits    == 0);
    CHECK(cap.pickupHits == 0);
}

TEST_CASE("Vendor::TryBuy on null player is a safe no-op") {
    VendorCapture cap;
    cap.Attach();

    Vendor v({0, 0}, MakeOneItemStall());
    CHECK_FALSE(v.TryBuy(nullptr, 0));
    CHECK(cap.msgHits    == 0);
    CHECK(cap.pickupHits == 0);
}

TEST_CASE("Factory::Create(ObjectType::Vendor) returns a Vendor*") {
    auto obj = GameObjectFactory::Create(ObjectType::Vendor, {1, 2});
    REQUIRE(obj != nullptr);
    auto* v = dynamic_cast<Vendor*>(obj.get());
    CHECK(v != nullptr);
    // Placeholder config matches what GameObjectFactory.cpp hard-codes.
    CHECK(v->Config().name == "市集攤主");
    CHECK(v->Config().stock.size() == 1);
}
