#include "doctest/doctest.h"
#include "game/entities/CashPickup.h"
#include "engine/events/EventBus.h"
#include "game/controller/GameObjectFactory.h"
#include "game/entities/Player.h"

#include <string>

/**
 * @file test_cashpickup.cpp
 * @brief 驗證 CashPickup（金錢拾取物）：撿取後增加玩家金錢、自我停用、
 *        發出 ShowMessage，並驗證 GameObjectFactory 產生各面額硬幣。
 */

namespace {

// 訂閱 ShowMessage 事件並記錄命中次數與最後一段文字，供各案驗證。
struct MessageCapture {
    int         hits = 0;
    std::string lastText;
    void Attach() {
        EventBus::Instance().Clear();
        EventBus::Instance().Subscribe(EventType::ShowMessage,
            [this](const Event& e) { hits++; lastText = e.text; });
    }
};

} // namespace

// 撿取硬幣後玩家金錢增加其面額、物件停用、並發出一則撿錢訊息。
TEST_CASE("CashPickup OnPickup：money 增加其 value、isActive 翻轉、發出 ShowMessage") {
    Player p({0, 0});
    const int before = p.GetMoney();

    MessageCapture cap;
    cap.Attach();

    CashPickup coin({10, 20}, 7);
    CHECK(coin.IsActive());
    CHECK(coin.Value() == 7);
    CHECK(coin.GetName() == "Cash");

    coin.OnPickup(&p);

    CHECK(p.GetMoney() == before + 7);
    CHECK_FALSE(coin.IsActive());
    CHECK(cap.hits     == 1);
    CHECK(cap.lastText == "撿到 7 元");
}

// 對 null 玩家撿取是安全的空操作：物件維持啟用、不發事件。
TEST_CASE("CashPickup OnPickup 對 null 玩家是安全的空操作") {
    MessageCapture cap;
    cap.Attach();

    CashPickup coin({0, 0}, 5);
    coin.OnPickup(nullptr);
    // 維持啟用，不發出任何事件。
    CHECK(coin.IsActive());
    CHECK(cap.hits == 0);
}

// Factory 產生面額 5 的 CashPickup，撿取後玩家金錢加 5。
TEST_CASE("Factory::Create(CashPickup5) 產生撿取後加 5 元的 CashPickup") {
    Player p({0, 0});
    const int before = p.GetMoney();

    auto obj = GameObjectFactory::Create(ObjectType::CashPickup5, {0, 0});
    REQUIRE(obj != nullptr);
    auto* coin = dynamic_cast<CashPickup*>(obj.get());
    REQUIRE(coin != nullptr);
    CHECK(coin->Value() == 5);

    coin->OnPickup(&p);
    CHECK(p.GetMoney() == before + 5);
}

// Factory 產生面額 10 的 CashPickup，撿取後玩家金錢加 10。
TEST_CASE("Factory::Create(CashPickup10) 撿取後加 10 元") {
    Player p({0, 0});
    const int before = p.GetMoney();

    auto obj = GameObjectFactory::Create(ObjectType::CashPickup10, {0, 0});
    REQUIRE(obj != nullptr);
    auto* coin = dynamic_cast<CashPickup*>(obj.get());
    REQUIRE(coin != nullptr);
    CHECK(coin->Value() == 10);

    coin->OnPickup(&p);
    CHECK(p.GetMoney() == before + 10);
}

// Factory 產生面額 20 的 CashPickup，撿取後玩家金錢加 20。
TEST_CASE("Factory::Create(CashPickup20) 撿取後加 20 元") {
    Player p({0, 0});
    const int before = p.GetMoney();

    auto obj = GameObjectFactory::Create(ObjectType::CashPickup20, {0, 0});
    REQUIRE(obj != nullptr);
    auto* coin = dynamic_cast<CashPickup*>(obj.get());
    REQUIRE(coin != nullptr);
    CHECK(coin->Value() == 20);

    coin->OnPickup(&p);
    CHECK(p.GetMoney() == before + 20);
}
