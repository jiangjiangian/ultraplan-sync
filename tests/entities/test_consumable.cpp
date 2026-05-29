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

/**
 * @file test_consumable.cpp
 * @brief 驗證可消耗道具（HotPack/WaterproofSpray/EnergyDrink 等）：直接 Consume 的
 *        karma 與雨量計效果、撿取改為放進背包不立即生效、由背包使用才套用效果，
 *        以及各道具的雨量緩解表與 IsUsableConsumable 分類。
 */

namespace {

// 訂閱 ShowMessage 並記錄命中次數與最後文字。
// 改用 ScopedSubscribe，讓 handler 生命週期綁在 capture 自身的 scope；
// 若沿用裸 Subscribe 並依賴下一案 Clear() 才移除舊 handler，析構後到下一次
// Attach() 之間 lambda 捕獲的 this 會成為懸空指標，跨案的 Publish 會誤觸。
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

// 暖暖包 Consume：karma +5、雨量計固定減 25、物件停用、發出對應訊息。
TEST_CASE("HotPack Consume: karma +5, rain -25 (G4), isActive false, ShowMessage") {
    Player p({0, 0});
    // 暖暖包不再是整條歸零，而是固定扣 25。先把雨量計灌到 25 以上，
    // 這樣 -25 才觀察得到（不會被地板裁切）。
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

// 防水噴霧 Consume：雨量計減 35、不影響 karma（裝備類）、物件停用。
TEST_CASE("WaterproofSpray Consume: rain -35 (G4), no karma, isActive false") {
    Player p({0, 0});
    p.ApplyRain(16.0f, /*lethal=*/false);        // +80（上限裁切於 <100）
    REQUIRE(p.GetRainMeter() == doctest::Approx(80.0f));
    const int beforeKarma = p.GetKarma();

    MessageCapture cap;
    cap.Attach();

    WaterproofSpray spray({5, 5});
    CHECK(spray.IsActive() == true);
    CHECK(spray.GetPrice() == 50);

    spray.Consume(&p);

    CHECK(p.GetKarma() == beforeKarma);          // 裝備類，karma 不變
    CHECK(p.GetRainMeter() ==
          doctest::Approx(80.0f - WaterproofSpray::kRainRelief));  // 45
    CHECK(spray.IsActive() == false);
    CHECK(cap.hits == 1);
    CHECK(cap.lastText == "噴了防水噴霧，雨水大半都被彈開了。");
}

// 能量飲料 Consume：karma +3、雨量計減 15、物件停用。
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

// 對 null 玩家 Consume 是空操作：不崩潰，道具維持啟用。
TEST_CASE("Consume on null player is a no-op (no crash, item stays active)") {
    MessageCapture cap;
    cap.Attach();

    HotPack pack({0, 0});
    pack.Consume(nullptr);
    CHECK(pack.IsActive() == true);
    CHECK(cap.hits == 0);
}

// Factory 產生的 HotPack 具備正確的動態型別。
TEST_CASE("Factory creates HotPack with correct dynamic type") {
    auto obj = GameObjectFactory::Create(ObjectType::HotPack, {1, 2});
    REQUIRE(obj != nullptr);
    auto* c = dynamic_cast<HotPack*>(obj.get());
    CHECK(c != nullptr);
}

// Factory 產生的 WaterproofSpray 具備正確的動態型別。
TEST_CASE("Factory creates WaterproofSpray with correct dynamic type") {
    auto obj = GameObjectFactory::Create(ObjectType::WaterproofSpray, {1, 2});
    REQUIRE(obj != nullptr);
    auto* c = dynamic_cast<WaterproofSpray*>(obj.get());
    CHECK(c != nullptr);
}

// Factory 產生的 EnergyDrink 具備正確的動態型別。
TEST_CASE("Factory creates EnergyDrink with correct dynamic type") {
    auto obj = GameObjectFactory::Create(ObjectType::EnergyDrink, {1, 2});
    REQUIRE(obj != nullptr);
    auto* c = dynamic_cast<EnergyDrink*>(obj.get());
    CHECK(c != nullptr);
}

// 從世界撿起可消耗道具只會以 itemId 加入背包（計數），當下不套用任何效果；
// 效果延後到「從背包使用」才發生。
TEST_CASE("Item 2a: ConsumableItem pickup adds to the bag, applies NO effect") {
    Player p{nccu::engine::math::Vec2{0, 0}};
    const int k0 = p.GetKarma();
    MessageCapture cap;
    cap.Attach();

    HotPack pack{nccu::engine::math::Vec2{0, 0}};
    REQUIRE(p.ConsumableCount("HotPack") == 0);
    pack.OnPickup(&p);

    CHECK(p.ConsumableCount("HotPack") == 1);  // 進到背包
    CHECK(p.GetKarma() == k0);                 // 撿取當下不套用 karma
    CHECK(p.GetRainMeter() == doctest::Approx(0.0f));
    CHECK(cap.hits == 0);                      // 撿取不發風味訊息
    CHECK_FALSE(pack.IsActive());              // 物件已被收取（之後掃描移除）

    // 冪等：對已失效的物件再撿一次不會有任何作用。
    pack.OnPickup(&p);
    CHECK(p.ConsumableCount("HotPack") == 1);

    // Interact() 走的是同一條收取路徑。
    EnergyDrink drink{nccu::engine::math::Vec2{0, 0}};
    drink.Interact(&p);
    CHECK(p.ConsumableCount("EnergyDrink") == 1);
    CHECK(p.GetKarma() == k0);                 // 撿取仍不套用效果
}

// 從背包使用持有的可消耗道具，會套用與 Consume() 完全相同的效果
//（相同 karma、相同 ShowMessage），並由呼叫方扣減數量。
// ApplyConsumableEffect 是共用的效果，ConsumeOne 才是消耗。
TEST_CASE("Item 2b: use-from-bag applies the effect + the catalog message") {
    Player p{nccu::engine::math::Vec2{0, 0}};
    p.AddConsumable("HotPack").AddConsumable("HotPack");
    p.ApplyRain(12.0f, /*lethal=*/false);      // +60, above the -25 dry
    REQUIRE(p.GetRainMeter() == doctest::Approx(60.0f));
    const int k0 = p.GetKarma();

    MessageCapture cap;
    cap.Attach();

    // ApplyConsumableEffect 與 HotPack::Consume 完全一致：karma +5、雨量 -25。
    nccu::ApplyConsumableEffect(EventBus::Instance(), p, "HotPack");
    CHECK(p.GetKarma() == k0 + HotPack::kKarmaBonus);   // +5，同 Consume
    CHECK(p.GetRainMeter() ==
          doctest::Approx(60.0f - HotPack::kRainRelief));  // 35，同 Consume
    CHECK(cap.hits == 1);
    CHECK(cap.lastText == "用了暖暖包，烘乾了大半的雨水，心情也好了一些。");

    // controller 在套用後會消耗一個，這裡模擬該行為。
    CHECK(p.ConsumeOne("HotPack"));
    CHECK(p.ConsumableCount("HotPack") == 1);

    // 能量飲料的效果同樣對應其 Consume 內容（karma +3、雨量 -15）。
    Player q{nccu::engine::math::Vec2{0, 0}};
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

// 每個可使用消耗品的雨量緩解對照表：固定各道具的扣減量、涵蓋沒有實體類別的
// 一般小吃路徑，並驗證實體 Consume 與 ApplyConsumableEffect 行為一致。
TEST_CASE("G4: consumable rain-relief table (use-from-bag)") {
    struct Row { const char* id; float relief; bool usable; };
    const Row rows[] = {
        {"WaterproofSpray", 35.0f, true},
        {"HotPack",         25.0f, true},
        {"EnergyDrink",     15.0f, true},
        {"EggCake",         15.0f, true},
        {"FlowerTea",       15.0f, true},
        {"Takoyaki",        15.0f, true},
        {"Donation",         0.0f, false},   // 捐贈：非可使用，無雨量效果
    };
    for (const Row& r : rows) {
        CHECK(nccu::IsUsableConsumable(r.id) == r.usable);
        Player p{nccu::engine::math::Vec2{0, 0}};
        p.ApplyRain(18.0f, /*lethal=*/false);          // +90 (clamped <100)
        REQUIRE(p.GetRainMeter() == doctest::Approx(90.0f));
        MessageCapture cap; cap.Attach();
        nccu::ApplyConsumableEffect(EventBus::Instance(), p, r.id);
        CHECK(p.GetRainMeter() == doctest::Approx(90.0f - r.relief));
    }

    // 地板裁切：雨量計很低時不會低於 0（沒有負雨量）。
    Player low{nccu::engine::math::Vec2{0, 0}};
    low.ApplyRain(2.0f, /*lethal=*/false);             // +10
    nccu::ApplyConsumableEffect(EventBus::Instance(), low, "WaterproofSpray");  // -35 → 裁切為 0
    CHECK(low.GetRainMeter() == doctest::Approx(0.0f));

    // DrainRainBy 單元：固定扣減、會裁切、不會傳送。
    Player u{nccu::engine::math::Vec2{0, 0}};
    u.ApplyRain(10.0f, /*lethal=*/false);              // +50
    u.DrainRainBy(20.0f);
    CHECK(u.GetRainMeter() == doctest::Approx(30.0f));
    u.DrainRainBy(100.0f);                             // 過量扣減 → 裁切為 0
    CHECK(u.GetRainMeter() == doctest::Approx(0.0f));
}

// IsUsableConsumable 對背包中各種道具的可使用分類。
TEST_CASE("Item 2: IsUsableConsumable classifies the bag rows") {
    CHECK(nccu::IsUsableConsumable("HotPack"));
    CHECK(nccu::IsUsableConsumable("EnergyDrink"));
    CHECK(nccu::IsUsableConsumable("WaterproofSpray"));
    CHECK_FALSE(nccu::IsUsableConsumable(nccu::kItemMoney));
    CHECK_FALSE(nccu::IsUsableConsumable(nccu::kItemTrueUmbrella));
    CHECK_FALSE(nccu::IsUsableConsumable("NotAThing"));
}
