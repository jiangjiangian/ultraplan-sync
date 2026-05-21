#include "doctest/doctest.h"
#include "entities/CashPickup.h"
#include "controller/EventBus.h"
#include "controller/GameObjectFactory.h"
#include "entities/Player.h"

#include <string>

namespace {

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

TEST_CASE("CashPickup OnPickup: money grows by value, isActive flips, ShowMessage fired") {
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

TEST_CASE("CashPickup OnPickup on null player is a safe no-op") {
    MessageCapture cap;
    cap.Attach();

    CashPickup coin({0, 0}, 5);
    coin.OnPickup(nullptr);
    // Stays active, no event emitted.
    CHECK(coin.IsActive());
    CHECK(cap.hits == 0);
}

TEST_CASE("Factory::Create(CashPickup5) yields a CashPickup that grants 5 on pickup") {
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

TEST_CASE("Factory::Create(CashPickup10) grants 10 on pickup") {
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

TEST_CASE("Factory::Create(CashPickup20) grants 20 on pickup") {
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
