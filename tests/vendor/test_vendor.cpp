#include "doctest/doctest.h"
#include "engine/events/EventBus.h"
#include "game/controller/GameObjectFactory.h"
#include "game/entities/Player.h"
#include "game/vendor/Vendor.h"
#include "game/vendor/VendorConfig.h"

#include <string>
#include <vector>

/**
 * @file test_vendor.cpp
 * @brief 驗證 Vendor 的核心契約：由 greeting + 庫存建構對話列、TryBuy 成功時扣錢並發出
 *        ShowMessage + PickupAcquired、金錢不足／索引越界／null player 等情況的安全行為，
 *        以及 Factory 能建出 Vendor。
 */

namespace {

// 同時捕捉 Vendor 發出的兩種事件——購買訊息與背包側的 PickupAcquired。每個測試重新建立，
// 避免 EventBus 狀態跨案例外洩。
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

// ctor 由 greeting + 庫存項建出 NPC 對話列。
TEST_CASE("Vendor: ctor seeds NPC dialog from greeting + stock entries") {
    Vendor v({0, 0}, MakeOneItemStall());
    // greeting + 一個庫存項 = 兩條可循環的對話列。
    CHECK(v.DialogLineCount() == 2);
    CHECK(v.CurrentLineText() == "歡迎光臨");
    CHECK(v.Config().name == "市集攤主");
    CHECK(v.Config().stock.size() == 1);
    CHECK(v.Config().stock[0].itemId == "HotPack");
    CHECK(v.Config().stock[0].price  == 30);
}

// TryBuy 成功：扣錢，並發出 ShowMessage + PickupAcquired。
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

    // 兩個事件都應恰好觸發一次。
    CHECK(cap.msgHits    == 1);
    // 提示顯示中文 catalog 名稱 + 花費 + 剩餘餘額（起始 100 - 30 = 70），而非原始 itemId。
    CHECK(cap.lastMsg    == "買了暖暖包，花了 30 元（剩 70 元）");
    CHECK(cap.pickupHits == 1);
    CHECK(cap.lastPickup == "HotPack");   // 背包事件仍用 id
}

// TryBuy 金錢不足：不扣錢、發出「錢不夠」訊息、無 pickup 事件。
TEST_CASE("Vendor::TryBuy insufficient funds: no deduction, ShowMessage 錢不夠, no pickup event") {
    Player p({0, 0});
    // 把錢包清空，使連最便宜的商品都買不起。
    while (p.DeductMoney(1)) { /* noop */ }
    const int startMoney = p.GetMoney();
    CHECK(startMoney == 0);

    VendorCapture cap;
    cap.Attach();

    Vendor v({0, 0}, MakeOneItemStall());
    const bool ok = v.TryBuy(&p, 0);
    CHECK_FALSE(ok);
    CHECK(p.GetMoney() == startMoney);    // 不變
    CHECK(cap.msgHits    == 1);
    CHECK(cap.lastMsg    == "你錢不夠");
    CHECK(cap.pickupHits == 0);
}

// TryBuy 索引越界：回傳 false 且不發任何事件。
TEST_CASE("Vendor::TryBuy out-of-bounds index returns false and emits no events") {
    Player p({0, 0});
    const int startMoney = p.GetMoney();

    VendorCapture cap;
    cap.Attach();

    Vendor v({0, 0}, MakeOneItemStall());
    // 庫存大小為 1，有效索引只有 0——1 與 99 都越界。
    CHECK_FALSE(v.TryBuy(&p, 1));
    CHECK_FALSE(v.TryBuy(&p, 99));
    CHECK(p.GetMoney()   == startMoney);
    CHECK(cap.msgHits    == 0);
    CHECK(cap.pickupHits == 0);
}

// TryBuy 對 null player 為安全的 no-op。
TEST_CASE("Vendor::TryBuy on null player is a safe no-op") {
    VendorCapture cap;
    cap.Attach();

    Vendor v({0, 0}, MakeOneItemStall());
    CHECK_FALSE(v.TryBuy(nullptr, 0));
    CHECK(cap.msgHits    == 0);
    CHECK(cap.pickupHits == 0);
}

// Factory::Create(ObjectType::Vendor) 回傳 Vendor*。
TEST_CASE("Factory::Create(ObjectType::Vendor) returns a Vendor*") {
    auto obj = GameObjectFactory::Create(ObjectType::Vendor, {1, 2});
    REQUIRE(obj != nullptr);
    auto* v = dynamic_cast<Vendor*>(obj.get());
    CHECK(v != nullptr);
    // 預留的 config 與 GameObjectFactory.cpp 寫死的內容一致。
    CHECK(v->Config().name == "市集攤主");
    CHECK(v->Config().stock.size() == 1);
}
