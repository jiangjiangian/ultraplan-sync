#include "game/quest/Flags.h"
#include "doctest/doctest.h"
#include "game/controller/GameController.h"
#include "game/world/World.h"
#include "game/entities/Player.h"
#include "game/dialog/DialogState.h"
#include "game/dialog/DialogSource.h"
#include "game/quest/ChapterVendors.h"
#include "engine/events/EventBus.h"
#include "engine/core/GameObject.h"
#include "game/state/SemesterState.h"
#include "engine/input/Input.h"
#include "engine/input/Key.h"
#include "engine/platform/Time.h"

#include <set>
#include <string>

/**
 * @file test_vendor_decline.cpp
 * @brief 驗證向 Vendor 購買「絕不會被強迫」：每個攤位選單都有結尾的「不買」選項，挑選它會
 *        關閉對話且不改變任何狀態（不扣錢、不加消耗品、不設旗標、不發 PickupAcquired）。
 *
 * 本測試經由 harness／互動測試所用的同一個 Input 入口驅動真正的 GameController::Update()，
 *        因此走的是正式產品的「不買」路徑，而非單元替身。
 */

#ifndef TEST_CONTENT_DIR
#error "TEST_CONTENT_DIR must be defined by the build system"
#endif

using nccu::World;
using nccu::SemesterState;
using nccu::engine::input::Key;

namespace {

class TestInput final : public nccu::engine::input::InputSource {
public:
    void Hold(Key k)    { if (down_.insert(static_cast<int>(k)).second) pressed_.insert(static_cast<int>(k)); }
    void Release(Key k) { if (down_.erase(static_cast<int>(k)))         released_.insert(static_cast<int>(k)); }
    void Tap(Key k)     { Hold(k); autoUp_.insert(static_cast<int>(k)); }
    void EndFrame() {
        pressed_.clear();
        released_.clear();
        for (int k : autoUp_) { if (down_.erase(k)) released_.insert(k); }
        autoUp_.clear();
    }
    bool IsDown(Key k)     const noexcept override { return down_.count(static_cast<int>(k)) != 0; }
    bool IsPressed(Key k)  const noexcept override { return pressed_.count(static_cast<int>(k)) != 0; }
    bool IsReleased(Key k) const noexcept override { return released_.count(static_cast<int>(k)) != 0; }
private:
    std::set<int> down_, pressed_, released_, autoUp_;
};

void Frame(nccu::GameController& c, TestInput& in) {
    c.Update();
    in.EndFrame();
}

const GameObject* FindVendor(const World& w) {
    for (const auto& u : w.Objects())
        if (u && u->IsActive() && u->IsVendor()) return u.get();
    return nullptr;
}

}  // namespace

// Ch4 集英樓攤位販售醜傘（售價 100，設定旗標 kFlagBoughtUglyUmbrella → 結局 C）。開啟選單、
// 把游標移到結尾的「不買」項、確認：對話關閉且一切不變——錢包、背包、旗標、EventBus 皆不動。
// 這是「不強迫購買」的保證。
TEST_CASE("REQ#4: declining a vendor purchase mutates nothing") {
    nccu::dialog::SetContentDir(TEST_CONTENT_DIR);
    nccu::engine::platform::Time::SetFixedStep(1.0f / 60.0f);
    EventBus::Instance().Clear();

    int pickupHits = 0;
    EventBus::Instance().Subscribe(EventType::PickupAcquired,
        [&](const Event&) { ++pickupHits; });

    World world("", /*loadSprites=*/false);
    nccu::GameController controller{world, EventBus::Instance()};

    world.Semester().Transition(SemesterState::Chapter4_Finals);
    TestInput in;
    nccu::engine::input::Input::SetSource(&in);
    Frame(controller, in);                              // roster -> Ch4

    const GameObject* vend = FindVendor(world);
    REQUIRE(vend != nullptr);
    Player* p = world.GetPlayer();
    REQUIRE(p != nullptr);
    p->AddMoney(300);
    const int money0 = p->GetMoney();
    REQUIRE_FALSE(p->HasFlag(nccu::kFlagBoughtUglyUmbrella));
    REQUIRE(p->ConsumableCount("UglyUmbrella") == 0);

    p->SetPosition(nccu::engine::math::Vec2{vend->GetPosition().x - 8.0f,
                                   vend->GetPosition().y});

    in.Tap(Key::E);                                     // 開啟購買選單
    Frame(controller, in);
    REQUIRE(world.Dialog().Active());
    for (int f = 0; f < 16 && !world.Dialog().AtChoice(); ++f) {
        in.Tap(Key::E);
        Frame(controller, in);
    }
    REQUIRE(world.Dialog().AtChoice());

    // 選單為庫存(1) + 結尾的不買。最後一個選項是「不買」。
    const std::size_t n = world.Dialog().Choices().size();
    REQUIRE(n == 2);
    CHECK(world.Dialog().Choices().back().label == "先不買，謝謝");
    CHECK(world.Dialog().Choices().back().setsFlag.empty());
    CHECK(world.Dialog().Choices().back().karmaDelta == 0);

    // 把游標往下移到最後一項（不買）並確認。
    for (std::size_t i = 0; i + 1 < n; ++i) {
        in.Tap(Key::Down);
        Frame(controller, in);
    }
    CHECK(world.Dialog().ChoiceCursor() == static_cast<int>(n) - 1);

    const int karma0 = p->GetKarma();
    in.Tap(Key::E);                                     // 確認「不買」
    Frame(controller, in);

    // 什麼都沒發生：對話關閉，錢包／旗標／背包／事件／業力全與先前完全相同——購買未被強迫。
    CHECK_FALSE(world.Dialog().Active());
    CHECK(p->GetMoney() == money0);
    CHECK(p->GetKarma() == karma0);
    CHECK_FALSE(p->HasFlag(nccu::kFlagBoughtUglyUmbrella));
    CHECK(p->ConsumableCount("UglyUmbrella") == 0);
    CHECK(pickupHits == 0);

    // 且攤位之後仍可使用：重新開啟並這次真的購買（游標預設回到庫存項）——不買並未弄壞攤位。
    p->SetPosition(nccu::engine::math::Vec2{vend->GetPosition().x - 8.0f,
                                   vend->GetPosition().y});
    in.Tap(Key::E);
    Frame(controller, in);
    REQUIRE(world.Dialog().Active());
    for (int f = 0; f < 16 && !world.Dialog().AtChoice(); ++f) {
        in.Tap(Key::E);
        Frame(controller, in);
    }
    REQUIRE(world.Dialog().AtChoice());
    CHECK(world.Dialog().ChoiceCursor() == 0);          // 回到購買那列
    in.Tap(Key::E);                                     // 確認購買
    Frame(controller, in);
    CHECK(p->GetMoney() == money0 - 100);
    CHECK(p->HasFlag(nccu::kFlagBoughtUglyUmbrella));
    CHECK(pickupHits == 1);

    nccu::engine::input::Input::SetSource(nullptr);
    nccu::engine::platform::Time::SetFixedStep(0.0f);
    EventBus::Instance().Clear();
}
