/**
 * @file test_ch4_ending_confession.cpp
 * @brief 驗證 Ch4 結局前的自白延後：自白播放時不轉場、關閉後才結算，且自白依優先序只觸發一次。
 */
#include "doctest/doctest.h"
#include "game/quest/Flags.h"
#include "game/controller/GameController.h"
#include "game/quest/Chapter4Quest.h"
#include "game/state/EndingGate.h"
#include "game/world/World.h"
#include "game/entities/Player.h"
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

using nccu::World;
using nccu::SemesterState;
using nccu::engine::input::Key;
using nccu::engine::math::Vec2;

// 結局不可突兀：每個 Ch4 結局的觸發都會延後到一段簡短的內心獨白（自白）之後。
// 以下案例驅動真正的 GameController::Update() 迴圈，端到端驗證這個延後：
// 當自白對話還在畫面上時結局不會觸發，等它關閉後才觸發。同時釘住
// TryOpenEndingConfession 的單次鍵冪等性，以及自白內容要與閘門最終落入的結局相符。

namespace {

// 完全不按鍵——玩家站著不動，只跑每幀的雨／自白／閘門邏輯。
class TestInput final : public nccu::engine::input::InputSource {
public:
    void EndFrame() {}
    bool IsDown(Key)     const noexcept override { return false; }
    bool IsPressed(Key)  const noexcept override { return false; }
    bool IsReleased(Key) const noexcept override { return false; }
};

void Frame(nccu::GameController& c, TestInput& in) {
    c.Update();
    in.EndFrame();
}

// 建立一個已處於 Ch4、且已接好 GameController 的 World；給玩家一把傘，
// 讓雨的計時只是扣血（而非會干擾多幀延後測試的致命傳送），並回傳參考。
struct Ch4Fixture {
    World world;
    nccu::GameController controller;
    TestInput in;
    explicit Ch4Fixture()
        : world("", /*loadSprites=*/false), controller(world, EventBus::Instance()) {
        nccu::dialog::SetContentDir(TEST_CONTENT_DIR);
        nccu::engine::platform::Time::SetFixedStep(1.0f / 60.0f);
        EventBus::Instance().Clear();
        world.Semester().Transition(SemesterState::Chapter4_Finals);
        nccu::engine::input::Input::SetSource(&in);
        // 先用一個安靜的幀讓 Ch4 進場的副作用沉澱（會清掉 HasUmbrella 與
        // Flag_HasTrueUmbrella），之後再武裝玩家。
        Frame(controller, in);
    }
    ~Ch4Fixture() {
        nccu::engine::input::Input::SetSource(nullptr);
        nccu::engine::platform::Time::SetFixedStep(0.0f);
        EventBus::Instance().Clear();
    }
    Player& P() { return *world.GetPlayer(); }
};

}  // namespace

// 買醜傘的結局會延後到自白之後：自白期間不轉場，關閉後才結算到 Ending C。
TEST_CASE("買醜傘結局延後到自白之後，關閉後才結算為 Ending C") {
    Ch4Fixture fx;
    Player& p = fx.P();
    p.SetHasUmbrella(true);                       // 維持雨只扣血、不致命
    REQUIRE(fx.world.Semester().Current() == SemesterState::Chapter4_Finals);
    REQUIRE_FALSE(fx.world.Dialog().Active());

    // 武裝 Ending C 的觸發（即 Vendor::TryBuy 在集英樓醜傘設下的旗標）。
    p.SetFlag(nccu::kFlagBoughtUglyUmbrella);

    // 下一幀：非對話輪詢開啟「務實」自白，結局閘門延後到它之後——玩家須先讀完獨白。
    Frame(fx.controller, fx.in);
    CHECK(fx.world.Dialog().Active());                                  // 自白出現
    CHECK(fx.world.Semester().Current() == SemesterState::Chapter4_Finals);  // 尚未轉場

    // 對話框開著時即使多跑幾幀也不會改變（對話凍結會提前返回，結局維持延後）。
    for (int i = 0; i < 10; ++i) Frame(fx.controller, fx.in);
    CHECK(fx.world.Semester().Current() == SemesterState::Chapter4_Finals);

    // 玩家讀完：關閉對話框。下一次非對話輪詢便把（持久的）旗標結算為 Ending C。
    fx.world.Dialog().Close();
    Frame(fx.controller, fx.in);
    CHECK(fx.world.Semester().Current() == SemesterState::Ending_C);
}

// 詛咒傘的結局會延後到自白之後：關閉後才結算到 Ending B。
TEST_CASE("詛咒傘結局延後到自白之後，關閉後才結算為 Ending B") {
    Ch4Fixture fx;
    Player& p = fx.P();
    p.SetHasUmbrella(true);
    p.SetFlag(nccu::kFlagTookCursedUmbrella);         // 自 Ch1 帶來 -> Ending B

    Frame(fx.controller, fx.in);
    CHECK(fx.world.Dialog().Active());                                  // 詛咒自白
    CHECK(fx.world.Semester().Current() == SemesterState::Chapter4_Finals);

    fx.world.Dialog().Close();
    Frame(fx.controller, fx.in);
    CHECK(fx.world.Semester().Current() == SemesterState::Ending_B);
}

// 自白具單次性（once-key）：讀過之後絕不會在前往結局的路上再次開啟。
TEST_CASE("自白具單次性（once-key）— 讀過後絕不重新開啟") {
    Ch4Fixture fx;
    Player& p = fx.P();
    p.SetHasUmbrella(true);
    p.SetFlag(nccu::kFlagBoughtUglyUmbrella);

    Frame(fx.controller, fx.in);
    REQUIRE(fx.world.Dialog().Active());
    CHECK(p.HasFlag(nccu::kFlagCh4ConfessedUgly));   // 單次鍵已設
    fx.world.Dialog().Close();

    // 結算到 C；前往途中自白不得再觸發第二次。
    Frame(fx.controller, fx.in);
    CHECK(fx.world.Semester().Current() == SemesterState::Ending_C);
}

// 直接測試輔助函式：依優先序（詛咒高於醜傘／真傘）恰好開啟一次對應自白，且對話開啟中或非 Ch4 時為無操作。
TEST_CASE("TryOpenEndingConfession 依優先序恰好開啟一段自白一次") {
    nccu::dialog::SetContentDir(TEST_CONTENT_DIR);
    EventBus::Instance().Clear();

    SUBCASE("非 Ch4 -> 無操作") {
        Player p{Vec2{0, 0}}; nccu::DialogState d;
        p.SetFlag(nccu::kFlagBoughtUglyUmbrella);
        CHECK_FALSE(nccu::TryOpenEndingConfession(
            p, d, SemesterState::Chapter1_AddDrop));
        CHECK_FALSE(d.Active());
    }
    SUBCASE("詛咒優先於醜傘（與 B 優先於 C 的閘門一致）") {
        Player p{Vec2{0, 0}}; nccu::DialogState d;
        p.SetFlag(nccu::kFlagTookCursedUmbrella);
        p.SetFlag(nccu::kFlagBoughtUglyUmbrella);
        CHECK(nccu::TryOpenEndingConfession(
            p, d, SemesterState::Chapter4_Finals));
        CHECK(d.Active());
        CHECK(p.HasFlag(nccu::kFlagCh4ConfessedCursed));   // 選中詛咒
        CHECK_FALSE(p.HasFlag(nccu::kFlagCh4ConfessedUgly));
    }
    SUBCASE("絕不打斷進行中的對話；once-key 擋住重新開啟") {
        Player p{Vec2{0, 0}}; nccu::DialogState d;
        p.SetFlag(nccu::kFlagBoughtUglyUmbrella);
        d.Open({"some other conversation"});
        CHECK_FALSE(nccu::TryOpenEndingConfession(
            p, d, SemesterState::Chapter4_Finals));        // 對話框開著 -> 無操作
        d.Close();
        CHECK(nccu::TryOpenEndingConfession(
            p, d, SemesterState::Chapter4_Finals));        // 現在會開啟
        d.Close();
        CHECK_FALSE(nccu::TryOpenEndingConfession(
            p, d, SemesterState::Chapter4_Finals));        // 單次鍵 -> 無操作
    }
    SUBCASE("重新取得真傘的自白只在結局前出現（不重複橋段）") {
        Player p{Vec2{0, 0}}; nccu::DialogState d;
        p.SetFlag(nccu::kFlagHasTrueUmbrella);
        p.SetFlag(nccu::kFlagTaFinaleChoiceMade);              // 溫柔結局路徑
        CHECK_FALSE(nccu::TryOpenEndingConfession(
            p, d, SemesterState::Chapter4_Finals));        // 被抑制
        Player q{Vec2{0, 0}}; nccu::DialogState d2;
        q.SetFlag(nccu::kFlagHasTrueUmbrella);                 // 從地面撿回、未走結局選擇
        CHECK(nccu::TryOpenEndingConfession(
            q, d2, SemesterState::Chapter4_Finals));
        CHECK(q.HasFlag(nccu::kFlagCh4ConfessedTrue));
    }
    EventBus::Instance().Clear();
}
