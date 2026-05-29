#include "doctest/doctest.h"
#include "game/quest/Flags.h"
#include "engine/events/EventBus.h"
#include "game/entities/Player.h"
#include "game/vendor/Vendor.h"
#include "game/vendor/VendorConfig.h"
#include "game/vendor/VendorMessages.h"
#include "game/quest/ItemCatalog.h"

#include <string>

// S5b-3: the buy now actually lands in the Player count-inventory, the
// stall karma hook fires, and a finite stock is enforced. test_vendor.cpp
// (the pinned base contract) is untouched — these are additive cases on
// the extended fields, all defaulted so the old stall behaves identically.

namespace {

struct MsgCapture {
    int msgHits = 0;
    std::string lastMsg;
    void Attach() {
        EventBus::Instance().Clear();
        EventBus::Instance().Subscribe(EventType::ShowMessage,
            [this](const Event& e) { msgHits++; lastMsg = e.text; });
    }
};

}  // namespace

TEST_CASE("Player: count inventory add / query / consume") {
    Player p{nccu::gfx::Vec2{0, 0}};
    CHECK(p.ConsumableCount("HotPack") == 0);

    p.AddConsumable("HotPack").AddConsumable("HotPack");
    p.AddConsumable("EnergyDrink");
    CHECK(p.ConsumableCount("HotPack")     == 2);
    CHECK(p.ConsumableCount("EnergyDrink") == 1);

    CHECK(p.ConsumeOne("HotPack"));
    CHECK(p.ConsumableCount("HotPack") == 1);
    CHECK(p.ConsumeOne("HotPack"));
    CHECK(p.ConsumableCount("HotPack") == 0);
    CHECK_FALSE(p.ConsumeOne("HotPack"));          // none left -> false
    CHECK_FALSE(p.ConsumeOne("NeverHad"));         // unknown -> false
}

TEST_CASE("Vendor::TryBuy lands the item in the count inventory") {
    Player p{nccu::gfx::Vec2{0, 0}};
    EventBus::Instance().Clear();

    VendorConfig cfg;
    cfg.name  = "測試攤";
    cfg.stock = {{"HotPack", 10}};                 // stockLeft -1 (unlimited)

    Vendor v({0, 0}, cfg);
    REQUIRE(p.GetMoney() >= 20);
    CHECK(v.TryBuy(&p, 0));
    CHECK(v.TryBuy(&p, 0));
    CHECK(p.ConsumableCount("HotPack") == 2);
}

TEST_CASE("Vendor::TryBuy applies karmaOnInteract (募款箱 valve)") {
    Player p{nccu::gfx::Vec2{0, 0}};
    EventBus::Instance().Clear();
    const int karma0 = p.GetKarma();

    VendorConfig donate;
    donate.name            = "學生會募款箱";
    donate.stock           = {{"Donation", 10}};
    donate.karmaOnInteract = 1;

    Vendor v({0, 0}, donate);
    CHECK(v.TryBuy(&p, 0));
    CHECK(p.GetKarma() == karma0 + 1);             // karma valve fired

    // A default stall (karmaOnInteract 0) must NOT move karma.
    Player q{nccu::gfx::Vec2{0, 0}};
    const int qk = q.GetKarma();
    VendorConfig plain;
    plain.name  = "一般攤";
    plain.stock = {{"HotPack", 10}};
    Vendor w({0, 0}, plain);
    CHECK(w.TryBuy(&q, 0));
    CHECK(q.GetKarma() == qk);
}

TEST_CASE("Vendor::TryBuy enforces finite stockLeft, then sold out") {
    Player p{nccu::gfx::Vec2{0, 0}};
    MsgCapture cap;
    cap.Attach();

    VendorConfig cfg;
    cfg.name = "限量攤";
    VendorItem ring;
    ring.itemId    = "Ring";
    ring.price     = 5;
    ring.stockLeft = 2;
    cfg.stock = {ring};

    Vendor v({0, 0}, cfg);
    const int m0 = p.GetMoney();
    CHECK(v.TryBuy(&p, 0));                         // 1 -> stockLeft 1
    CHECK(v.TryBuy(&p, 0));                         // 2 -> stockLeft 0
    CHECK(p.ConsumableCount("Ring") == 2);
    CHECK(p.GetMoney() == m0 - 10);

    // Third buy: sold out — no charge, no inventory gain, kSoldOut toast.
    CHECK_FALSE(v.TryBuy(&p, 0));
    CHECK(p.ConsumableCount("Ring") == 2);
    CHECK(p.GetMoney() == m0 - 10);
    CHECK(cap.lastMsg == std::string(nccu::vendor::msg::kSoldOut));
}

// Item 5b: the Ch4 集英樓 ugly-umbrella stall (the exact item the owner
// "didn't see deduct money") now shows the 中文 catalog name + price spent
// + remaining balance, via the SAME TryBuy path the market uses.
TEST_CASE("Item 5b: ugly-umbrella buy toast shows 中文 name + spend + balance") {
    Player p{nccu::gfx::Vec2{0, 0}};            // starts with 100 元
    MsgCapture cap;
    cap.Attach();

    VendorConfig cfg;
    cfg.name  = "集英樓便利商店";
    cfg.stock = {VendorItem{"UglyUmbrella", 100, -1, nccu::kFlagBoughtUglyUmbrella}};

    Vendor v({0, 0}, cfg);
    REQUIRE(p.GetMoney() == 100);
    CHECK(v.TryBuy(&p, 0));
    CHECK(p.GetMoney() == 0);                   // 100 - 100
    // The 中文 name (not "UglyUmbrella"), the price, and the 0 balance.
    CHECK(cap.lastMsg == "買了螢光綠醜傘，花了 100 元（剩 0 元）");
    CHECK(p.HasFlag(nccu::kFlagBoughtUglyUmbrella)); // Ending C seed still set
}

// Every itemId a vendor sells must resolve to a 中文 catalog name so no
// purchase toast / bag row ever prints a raw English id.
TEST_CASE("Item 2d/5b: every market itemId has a 中文 catalog name") {
    const char* ids[] = {"HotPack", "EnergyDrink", "WaterproofSpray",
                         "EggCake", "FlowerTea", "Takoyaki", "Donation",
                         "UglyUmbrella", "CursedUmbrella",
                         "TransparentUmbrella"};
    for (const char* id : ids) {
        const std::string name{nccu::ItemInfoFor(id).displayName};
        CHECK(name != id);                      // not the raw English id
        CHECK_FALSE(name.empty());
    }
}
