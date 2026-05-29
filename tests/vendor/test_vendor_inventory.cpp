#include "doctest/doctest.h"
#include "game/quest/Flags.h"
#include "engine/events/EventBus.h"
#include "game/entities/Player.h"
#include "game/vendor/Vendor.h"
#include "game/vendor/VendorConfig.h"
#include "game/vendor/VendorMessages.h"
#include "game/quest/ItemCatalog.h"

#include <string>

/**
 * @file test_vendor_inventory.cpp
 * @brief 驗證 Vendor 購買的延伸欄位：商品確實落入 Player 的計數背包、攤位的業力鉤子會觸發、
 *        有限庫存（stockLeft）會被強制執行並售罄，以及購買提示顯示中文名稱 + 花費 + 餘額。
 *        既有的基本契約（test_vendor.cpp）不受影響——這些是新增、且全部有預設值的案例。
 */

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

// Player 計數背包的加入 / 查詢 / 消耗。
TEST_CASE("Player: count inventory add / query / consume") {
    Player p{nccu::engine::math::Vec2{0, 0}};
    CHECK(p.ConsumableCount("HotPack") == 0);

    p.AddConsumable("HotPack").AddConsumable("HotPack");
    p.AddConsumable("EnergyDrink");
    CHECK(p.ConsumableCount("HotPack")     == 2);
    CHECK(p.ConsumableCount("EnergyDrink") == 1);

    CHECK(p.ConsumeOne("HotPack"));
    CHECK(p.ConsumableCount("HotPack") == 1);
    CHECK(p.ConsumeOne("HotPack"));
    CHECK(p.ConsumableCount("HotPack") == 0);
    CHECK_FALSE(p.ConsumeOne("HotPack"));          // 已無剩餘 -> false
    CHECK_FALSE(p.ConsumeOne("NeverHad"));         // 未知 -> false
}

// TryBuy 會把商品放進計數背包。
TEST_CASE("Vendor::TryBuy lands the item in the count inventory") {
    Player p{nccu::engine::math::Vec2{0, 0}};
    EventBus::Instance().Clear();

    VendorConfig cfg;
    cfg.name  = "測試攤";
    cfg.stock = {{"HotPack", 10}};                 // stockLeft -1（無限）

    Vendor v({0, 0}, cfg);
    REQUIRE(p.GetMoney() >= 20);
    CHECK(v.TryBuy(&p, 0));
    CHECK(v.TryBuy(&p, 0));
    CHECK(p.ConsumableCount("HotPack") == 2);
}

// TryBuy 會套用 karmaOnInteract（募款箱的業力閥）。
TEST_CASE("Vendor::TryBuy applies karmaOnInteract (募款箱 valve)") {
    Player p{nccu::engine::math::Vec2{0, 0}};
    EventBus::Instance().Clear();
    const int karma0 = p.GetKarma();

    VendorConfig donate;
    donate.name            = "學生會募款箱";
    donate.stock           = {{"Donation", 10}};
    donate.karmaOnInteract = 1;

    Vendor v({0, 0}, donate);
    CHECK(v.TryBuy(&p, 0));
    CHECK(p.GetKarma() == karma0 + 1);             // 業力閥觸發

    // 一般攤位（karmaOnInteract 0）不得改變業力。
    Player q{nccu::engine::math::Vec2{0, 0}};
    const int qk = q.GetKarma();
    VendorConfig plain;
    plain.name  = "一般攤";
    plain.stock = {{"HotPack", 10}};
    Vendor w({0, 0}, plain);
    CHECK(w.TryBuy(&q, 0));
    CHECK(q.GetKarma() == qk);
}

// TryBuy 會強制有限的 stockLeft，售完後售罄。
TEST_CASE("Vendor::TryBuy enforces finite stockLeft, then sold out") {
    Player p{nccu::engine::math::Vec2{0, 0}};
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

    // 第三次購買：售罄——不扣錢、不增背包、顯示 kSoldOut 提示。
    CHECK_FALSE(v.TryBuy(&p, 0));
    CHECK(p.ConsumableCount("Ring") == 2);
    CHECK(p.GetMoney() == m0 - 10);
    CHECK(cap.lastMsg == std::string(nccu::vendor::msg::kSoldOut));
}

// Ch4 集英樓醜傘攤位的購買提示顯示中文 catalog 名稱 + 花費 + 餘額，走的是與市集相同的
// TryBuy 路徑。
TEST_CASE("Item 5b: ugly-umbrella buy toast shows 中文 name + spend + balance") {
    Player p{nccu::engine::math::Vec2{0, 0}};            // 起始有 100 元
    MsgCapture cap;
    cap.Attach();

    VendorConfig cfg;
    cfg.name  = "集英樓便利商店";
    cfg.stock = {VendorItem{"UglyUmbrella", 100, -1, nccu::kFlagBoughtUglyUmbrella}};

    Vendor v({0, 0}, cfg);
    REQUIRE(p.GetMoney() == 100);
    CHECK(v.TryBuy(&p, 0));
    CHECK(p.GetMoney() == 0);                   // 100 - 100
    // 中文名稱（非 "UglyUmbrella"）、售價、以及 0 餘額。
    CHECK(cap.lastMsg == "買了螢光綠醜傘，花了 100 元（剩 0 元）");
    CHECK(p.HasFlag(nccu::kFlagBoughtUglyUmbrella)); // 結局 C 的種子旗標仍設定
}

// 攤位販售的每個 itemId 都必須能解析到中文 catalog 名稱，使購買提示／背包列絕不印出原始
// 英文 id。
TEST_CASE("Item 2d/5b: every market itemId has a 中文 catalog name") {
    const char* ids[] = {"HotPack", "EnergyDrink", "WaterproofSpray",
                         "EggCake", "FlowerTea", "Takoyaki", "Donation",
                         "UglyUmbrella", "CursedUmbrella",
                         "TransparentUmbrella"};
    for (const char* id : ids) {
        const std::string name{nccu::ItemInfoFor(id).displayName};
        CHECK(name != id);                      // 不是原始英文 id
        CHECK_FALSE(name.empty());
    }
}
