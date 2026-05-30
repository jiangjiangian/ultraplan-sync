// 企劃核心的「淋雨求生」迴圈。
//
// 機制：GameController::Update() 每幀依玩家處境二擇一：
//   * 排水（Player::DrainRain，-10 u/s）：當玩家受庇護（撐傘或在建築內）。
//   * 累積（Player::ApplyRain lethal=true，+5 u/s）：當玩家曝露（戶外且未持傘）；
//     達 100 時 RespawnAtGate 把玩家傳送回正門、重設雨量計、發出落湯雞訊息。
//（只有在市集過場與結局會略過。）雨量計滿是「沒管理好曝露」的失敗，而非隱藏的
// 一次性計時器；有了排水機制，既有的結局腳本仍可通關且具決定性。
//
// 走 GameController 的測試會驅動真正的 Update() 迴圈，並透過 harness 所用的同一個
// nccu::engine::input::Input 節點注入輸入，藉此演練實際的生產路徑。

#include "doctest/doctest.h"
#include "game/controller/GameController.h"
#include "game/world/World.h"
#include "game/entities/Player.h"
#include "game/world/Buildings.h"
#include "game/dialog/DialogState.h"
#include "game/dialog/DialogSource.h"
#include "engine/events/EventBus.h"
#include "game/state/SemesterState.h"
#include "engine/input/Input.h"
#include "engine/input/Key.h"
#include "engine/platform/Time.h"

#include <set>
#include <string>

#ifndef TEST_CONTENT_DIR
#error "TEST_CONTENT_DIR must be defined by the build system"
#endif

/**
 * @file test_rain_survival.cpp
 * @brief 驗證淋雨求生迴圈：DrainRain 純恢復、撐傘戶外的慢速累積、戶外無傘累積後
 *        觸發致命傳送，以及撐傘只是減速、唯有進入建築才會排乾；市集過場不計雨。
 */

using nccu::World;
using nccu::SemesterState;
using nccu::engine::input::Key;
using nccu::engine::math::Vec2;

namespace {

// 最小的腳本化 InputSource，採 raylib 的邊緣語意，驅動方式與 harness 驅動
// ScriptInput 完全相同。這裡從不按下任何鍵——玩家站著不動，只有雨量 tick 在跑
// ——但 GameController 仍需要一個可輪詢的 InputSource。
class TestInput final : public nccu::engine::input::InputSource {
public:
    void EndFrame() { pressed_.clear(); released_.clear(); }
    bool IsDown(Key)     const noexcept override { return false; }
    bool IsPressed(Key)  const noexcept override { return false; }
    bool IsReleased(Key) const noexcept override { return false; }
private:
    std::set<int> pressed_, released_;
};

void Frame(nccu::GameController& c, TestInput& in) {
    c.Update();
    in.EndFrame();
}

}  // namespace

// 單元測試——DrainRain 是純恢復：-10 u/s、裁切到 [0,100]、絕不傳送。先用
// ApplyRain(lethal=false)（無傘累積但抑制傳送）把雨量計灌高，再排乾到地板。
TEST_CASE("淋雨：DrainRain 以 -10 u/s 恢復、裁切於 0，無副作用") {
    Player p{Vec2{1234.0f, 5678.0f}};
    REQUIRE(p.GetRainMeter() == doctest::Approx(0.0f));

    // +5 u/s 跑 4 秒 => 20（遠離上限與地板）。
    for (int i = 0; i < 240; ++i) p.ApplyRain(1.0f / 60.0f, /*lethal=*/false);
    const float raised = p.GetRainMeter();
    CHECK(raised == doctest::Approx(20.0f).epsilon(0.05));

    // -10 u/s 跑 1 秒 => -10（約 20 -> 約 10），嚴格遞減。
    float prev = raised;
    for (int i = 0; i < 60; ++i) {
        p.DrainRain(1.0f / 60.0f);
        const float now = p.GetRainMeter();
        CHECK(now < prev);
        prev = now;
    }
    CHECK(p.GetRainMeter() == doctest::Approx(raised - 10.0f).epsilon(0.05));

    // 排水裁切於 0——不會變負，且位置不受影響（DrainRain 不得像致命累積那樣傳送）。
    for (int i = 0; i < 600; ++i) p.DrainRain(1.0f / 60.0f);
    CHECK(p.GetRainMeter() == doctest::Approx(0.0f));
    CHECK(p.GetPosition().x == doctest::Approx(1234.0f));
    CHECK(p.GetPosition().y == doctest::Approx(5678.0f));
}

// 單元測試——ApplyRainSheltered（撐傘但在戶外）是慢速累積：+1.5 u/s，約曝露
// +5 u/s 的三成，裁切於 [0,100]，且帶致命機制（滿值時傳送並重設，同 ApplyRain）。
// 它在相同時間內必須遠慢於 ApplyRain，且（與 ApplyRain 不同）不理會持傘旗標
//（因為這正是撐傘的情況）。
TEST_CASE("ApplyRainSheltered 慢速累積（約 1.5 u/s）且帶致命機制") {
    Player p{Vec2{100.0f, 200.0f}};
    p.SetHasUmbrella(true);                  // 不論持傘與否都必須累積
    REQUIRE(p.GetRainMeter() == doctest::Approx(0.0f));

    // 撐傘戶外 4 秒 => 約 6（1.5 u/s），嚴格遞增。
    float prev = 0.0f;
    for (int i = 0; i < 240; ++i) {
        p.ApplyRainSheltered(1.0f / 60.0f, /*lethal=*/false);
        const float now = p.GetRainMeter();
        CHECK(now >= prev);
        prev = now;
    }
    CHECK(p.GetRainMeter() == doctest::Approx(6.0f).epsilon(0.05));

    // 遠慢於曝露：全新玩家以 ApplyRain 累積相同的 4 秒會到約 20（超過 3 倍）。
    // 慢 ≠ 零，慢 ≠ 快。
    Player q{Vec2{0.0f, 0.0f}};
    for (int i = 0; i < 240; ++i) q.ApplyRain(1.0f / 60.0f, /*lethal=*/false);
    CHECK(q.GetRainMeter() > p.GetRainMeter() * 2.5f);

    // 帶致命機制：開啟傳送把它推過上限——雨量計會重設（不會釘在 100），
    // 玩家被移到正門。
    Player r{Vec2{1500.0f, 1500.0f}};
    r.SetHasUmbrella(true);
    for (int i = 0; i < 60 * 80; ++i)        // 80 秒 * 1.5 ≈ 120 >> 100
        r.ApplyRainSheltered(1.0f / 60.0f, /*lethal=*/true);
    CHECK(r.GetRainMeter() < 100.0f);        // 已重設，不會卡在上限
    CHECK(r.GetPosition().x == doctest::Approx(500.0f));   // 正門重生
    CHECK(r.GetPosition().y == doctest::Approx(1860.0f));
}

// 戶外無傘玩家（Ch1 出生點在所有建築矩形以南，故 CurrentBuildingName() 為空）
// 透過真正的 GC tick 以 +5 u/s 累積，滿值時觸發致命傳送：發出落湯雞 ShowMessage
// 並重設雨量計（不會釘在 100）。
TEST_CASE("淋雨：戶外無傘玩家累積後觸發致命傳送") {
    nccu::dialog::SetContentDir(TEST_CONTENT_DIR);
    nccu::engine::platform::Time::SetFixedStep(1.0f / 60.0f);
    EventBus::Instance().Clear();

    int soakMsgHits = 0;
    EventBus::Instance().Subscribe(EventType::ShowMessage,
        [&](const Event& e) {
            if (e.text.find("落湯雞") != std::string::npos) ++soakMsgHits;
        });

    World world("", /*loadSprites=*/false);
    nccu::GameController controller{world, EventBus::Instance()};
    Player* p = world.GetPlayer();
    REQUIRE(p != nullptr);
    REQUIRE(world.Semester().Current() == SemesterState::Chapter1_AddDrop);
    REQUIRE_FALSE(p->HasUmbrella());
    REQUIRE(p->GetRainMeter() == doctest::Approx(0.0f));

    TestInput in;
    nccu::engine::input::Input::SetSource(&in);

    Frame(controller, in);
    CHECK(world.CurrentBuildingName().empty());      // 確認在戶外
    CHECK(p->GetRainMeter() > 0.0f);                  // 累積中（非排水）

    // 朝上限上升：在低於 100 且第一次傳送之前嚴格遞增（RespawnAtGate 一觸發，
    // 雨量計就會呈鋸齒下降）。
    float prev = p->GetRainMeter();
    bool sawHighThenDrop = false;
    for (int f = 0; f < 60 && soakMsgHits == 0; ++f) {
        Frame(controller, in);
        const float now = p->GetRainMeter();
        if (now < 100.0f && soakMsgHits == 0) CHECK(now >= prev);
        prev = now;
    }

    // 驅動到遠超飽和（100/5 = 20 秒；1800 幀 = 30 秒）。致命傳送必須至少觸發一次，
    // 且每次觸發都重設雨量計（因此不會釘在 100，而是鋸齒下降）。
    float maxSeen = 0.0f;
    for (int f = 0; f < 1800; ++f) {
        const float before = p->GetRainMeter();
        Frame(controller, in);
        const float after = p->GetRainMeter();
        if (before > maxSeen) maxSeen = before;
        if (before > 80.0f && after < 20.0f) sawHighThenDrop = true; // 重設
    }
    CHECK(soakMsgHits >= 1);            // 落湯雞 ShowMessage（只來自傳送門）
    CHECK(maxSeen >= 99.0f);            // 確實到達上限
    CHECK(sawHighThenDrop);            // 並且被重設（致命，非釘住）
    CHECK(p->GetRainMeter() < 100.0f); // 沒卡在上限

    nccu::engine::input::Input::SetSource(nullptr);
    nccu::engine::platform::Time::SetFixedStep(0.0f);
    EventBus::Instance().Clear();
}

// 每個章節都必須有淋雨壓力，因此撐傘不再等於免疫雨水：GC 的 tick 是三向的，
// 這個測試透過真正的迴圈驗證三個分支。
//   * 戶外、無傘     -> 快速累積（+5 u/s，ApplyRain）
//   * 戶外、有傘     -> 仍會累積，但很慢（+1.5 u/s，ApplyRainSheltered）——
//                       撐傘只是爭取時間，不是免疫
//   * 在建築物內     -> 排乾到 0（-10 u/s，真正的避難所）
TEST_CASE("撐傘只減緩淋雨（每章皆然）；唯有進入建築才會排乾") {
    nccu::dialog::SetContentDir(TEST_CONTENT_DIR);
    nccu::engine::platform::Time::SetFixedStep(1.0f / 60.0f);
    EventBus::Instance().Clear();

    int soakMsgHits = 0;
    EventBus::Instance().Subscribe(EventType::ShowMessage,
        [&](const Event& e) {
            if (e.text.find("落湯雞") != std::string::npos) ++soakMsgHits;
        });

    World world("", /*loadSprites=*/false);
    nccu::GameController controller{world, EventBus::Instance()};
    Player* p = world.GetPlayer();
    REQUIRE(p != nullptr);
    REQUIRE(world.Semester().Current() == SemesterState::Chapter1_AddDrop);

    TestInput in;
    nccu::engine::input::Input::SetSource(&in);

    // (1) 戶外、無傘約 3 秒 => 快速累積（+5 u/s ⇒ 約 15）。
    for (int f = 0; f < 180; ++f) Frame(controller, in);
    const float wet = p->GetRainMeter();
    CHECK(wet > 12.0f);                       // 3 秒 +5 u/s 約 15
    CHECK(soakMsgHits == 0);

    // (2) 裝備雨傘但留在戶外（Ch1 出生點在所有建築矩形以南 ⇒
    //     CurrentBuildingName() 為空）。雨量計必須持續上升——但很慢
    //     （撐傘非免疫）——嚴格遞增，且遠慢於曝露速率。
    p->SetHasUmbrella(true);
    float prev = p->GetRainMeter();
    for (int f = 0; f < 120; ++f) {           // 撐傘戶外 2 秒
        Frame(controller, in);
        const float now = p->GetRainMeter();
        CHECK(now >= prev);                   // 仍在累積，非排水
        prev = now;
    }
    const float slowGain = p->GetRainMeter() - wet;
    CHECK(slowGain > 0.0f);                   // 確實上升（非免疫）
    // 2 秒內：撐傘約 +1.5 u/s ⇒ 約 3；曝露則會是 +5 u/s ⇒ 約 10。
    // 驗證它落在慢速區間（遠低於快速速率）。
    CHECK(slowGain < 5.0f);
    CHECK(soakMsgHits == 0);

    // (3) 走進建築物內（其觸發矩形中心）——只有這裡雨量計才會排水、嚴格遞減
    //     到 0、不傳送。
    const auto& b = nccu::buildings::kAll[0];   // 大勇樓
    p->SetPosition(nccu::engine::math::Vec2{
        b.triggerRect.x + b.triggerRect.width  * 0.5f,
        b.triggerRect.y + b.triggerRect.height * 0.5f});
    Frame(controller, in);                      // BuildingTracker 鎖定
    float dprev = p->GetRainMeter();
    for (int f = 0; f < 60; ++f) {
        Frame(controller, in);
        const float now = p->GetRainMeter();
        CHECK(now <= dprev);                    // 室內排水
        dprev = now;
    }
    CHECK(p->GetRainMeter() < slowGain + wet);  // 已恢復
    for (int f = 0; f < 600; ++f) Frame(controller, in);
    CHECK(p->GetRainMeter() == doctest::Approx(0.0f));   // 完全乾了
    CHECK(soakMsgHits == 0);                              // 從未傳送

    nccu::engine::input::Input::SetSource(nullptr);
    nccu::engine::platform::Time::SetFixedStep(0.0f);
    EventBus::Instance().Clear();
}

// 淋雨只屬於玩法——市集過場是安全狀態，GC 完全略過 tick，因此即使無傘，
// 玩家的雨量計也維持不變。
TEST_CASE("淋雨：在 Interlude_Market（安全狀態）不累積也不排水") {
    nccu::dialog::SetContentDir(TEST_CONTENT_DIR);
    nccu::engine::platform::Time::SetFixedStep(1.0f / 60.0f);
    EventBus::Instance().Clear();

    World world("", /*loadSprites=*/false);
    nccu::GameController controller{world, EventBus::Instance()};
    world.Semester().Transition(SemesterState::Interlude_Market);

    TestInput in;
    nccu::engine::input::Input::SetSource(&in);
    Frame(controller, in);                       // 名冊 -> 過場
    Player* p = world.GetPlayer();
    REQUIRE(p != nullptr);
    REQUIRE_FALSE(p->HasUmbrella());
    REQUIRE(world.Semester().Current() == SemesterState::Interlude_Market);

    for (int f = 0; f < 600; ++f) Frame(controller, in);
    CHECK(p->GetRainMeter() == doctest::Approx(0.0f));  // 市集 => 不 tick

    nccu::engine::input::Input::SetSource(nullptr);
    nccu::engine::platform::Time::SetFixedStep(0.0f);
    EventBus::Instance().Clear();
}
