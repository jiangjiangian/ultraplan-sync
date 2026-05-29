#include "game/quest/Flags.h"
#include "doctest/doctest.h"
#include "game/controller/GameController.h"
#include "game/world/World.h"
#include "game/entities/Player.h"
#include "game/dialog/DialogState.h"
#include "game/dialog/DialogSource.h"
#include "game/quest/ChapterVendors.h"
#include "game/quest/ItemCatalog.h"
#include "game/quest/Chapter2Quest.h"
#include "engine/events/EventBus.h"
#include "engine/core/GameObject.h"
#include "game/state/SemesterState.h"
#include "engine/input/Input.h"
#include "engine/input/Key.h"
#include "engine/platform/Time.h"

#include <cmath>
#include <set>
#include <string>
#include <vector>

/**
 * @file test_i35_interact_vendor.cpp
 * @brief 透過真正的 GameController::Update() 迴圈驗證兩條互動主線：
 *        (1) 走近會擋路的 NPC 後按 E 能開啟對話（互動觸及範圍 kInteractReach 充氣），
 *        (2) 與 Vendor 互動會經由選單路由到 Vendor::TryBuy（含 Ch2 進度線）。
 *
 * 這些測試都經由 harness 所用的同一個 Input 入口來驅動，因此走的是正式產品路徑，而非
 *        單元層的替身。
 */

#ifndef TEST_CONTENT_DIR
#error "TEST_CONTENT_DIR must be defined by the build system"
#endif

using nccu::World;
using nccu::SemesterState;
using nccu::engine::input::Key;

namespace {

// 最小的腳本化 InputSource，採 raylib 的 edge 語意，並以 harness 驅動 ScriptInput 的
// 完全相同方式驅動：每格「設定本格按鍵；controller.Update(); src.EndFrame();」，使被
// Tap() 的鍵恰好在其後的那一個 Update 期間為「pressed」，並在 EndFrame() 自動放開——
// edge 契約與 LiveInput/ScriptInput 相同，GameController 分不出差異。
class TestInput final : public nccu::engine::input::InputSource {
public:
    void Hold(Key k)    { if (down_.insert(static_cast<int>(k)).second) pressed_.insert(static_cast<int>(k)); }
    void Release(Key k) { if (down_.erase(static_cast<int>(k)))         released_.insert(static_cast<int>(k)); }
    void Tap(Key k)     { Hold(k); autoUp_.insert(static_cast<int>(k)); }

    // 在 controller.Update() 「之後」呼叫：使本格的 press/release edge 過期，並自動放開
    // 任何被 tap 的鍵（使該按下恰好一個 Update 寬）。Hold（未 Tap）的鍵則跨格維持。
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

// 一個模擬格：本格按鍵已設好 -> Update -> 使每格 edge 過期（對應 Harness::EndFrame）。
void Frame(nccu::GameController& c, TestInput& in) {
    c.Update();
    in.EndFrame();
}

const GameObject* FindNpc(const World& w, const char* id) {
    for (const auto& u : w.Objects())
        if (u && u->IsActive() && u->NpcId() == id) return u.get();
    return nullptr;
}

const GameObject* FindVendor(const World& w) {
    for (const auto& u : w.Objects())
        if (u && u->IsActive() && u->IsVendor()) return u.get();
    return nullptr;
}

// 把玩家走到貼齊停住，然後每格 tap 一次 E 直到對話開啟（或預算用盡）。回傳是否開啟對話。
bool WalkUpAndTalk(nccu::GameController& c, TestInput& in, World& w,
                   Key approach, float wantX) {
    in.Hold(approach);
    Player* p = w.GetPlayer();
    float lastX = p->GetPosition().x, lastY = p->GetPosition().y;
    for (int f = 0; f < 800; ++f) {
        Frame(c, in);
        const auto pos = w.GetPlayer()->GetPosition();
        if (f > 5 && pos.x == lastX && pos.y == lastY) break;  // 貼齊停住
        lastX = pos.x; lastY = pos.y;
    }
    in.Release(approach);
    (void)wantX;
    for (int f = 0; f < 16; ++f) {
        in.Tap(Key::E);
        Frame(c, in);
        if (w.Dialog().Active()) return true;
    }
    return w.Dialog().Active();
}

}  // namespace

// 玩家「走」（按住 Key::A/W）靠近 Ch1 苦主，被其移動碰撞箱貼齊擋住後按 E，對話必須開啟。
// 若 E 探測框等同移動箱，貼齊停住時不會與 NPC 重疊，對話便永不開啟（整條主線在此卡死）。
TEST_CASE("I3: walking up to the Ch1 victim + E opens the dialog") {
    nccu::dialog::SetContentDir(TEST_CONTENT_DIR);
    nccu::engine::platform::Time::SetFixedStep(1.0f / 60.0f);
    EventBus::Instance().Clear();

    World world("", /*loadSprites=*/false);
    nccu::GameController controller{world, EventBus::Instance()};

    const GameObject* v = FindNpc(world, "victim");
    REQUIRE(v != nullptr);
    const float vx = v->GetPosition().x;             // {1660,1010}
    const float vy = v->GetPosition().y;

    // 苦主位於綜合院館 (1660,1010)，不在生成列上、且在南側校園牆後。互動觸及範圍的
    // 保證與位置無關，故把玩家放在苦主正南方、淨空的 x=1660 列上，往北走向他——以同樣的
    // 貼齊擋住 + E 探測幾何，改在 Y 軸而非 X 軸上演練。
    world.GetPlayer()->SetPosition(nccu::engine::math::Vec2{vx, vy + 90.0f});

    TestInput in;
    nccu::engine::input::Input::SetSource(&in);

    const bool opened = WalkUpAndTalk(controller, in, world, Key::W, vx);

    // 貼齊苦主：玩家可能被「碰到但未重疊」，故位於玩家原點的 24x24 E 探測框不會碰撞。
    // 互動觸及範圍的修正讓充氣後的探測框與 NPC 碰撞箱重疊。由南往北靠近，玩家最終在苦主
    // 下方（py >= vy），X 對齊。
    const float px = world.GetPlayer()->GetPosition().x;
    const float py = world.GetPlayer()->GetPosition().y;
    CHECK(py >= vy);                                   // 從未穿過
    CHECK(py <= vy + 40.0f);                            // 確實緊鄰
    CHECK(std::fabs(px - vx) < 1.0f);                   // Y 軸靠近的列
    CHECK(opened);                                      // <-- 互動鎖定點
    CHECK(world.Dialog().Active());
    CHECK(world.Dialog().NpcId() == "victim");

    nccu::engine::input::Input::SetSource(nullptr);
    nccu::engine::platform::Time::SetFixedStep(0.0f);
    EventBus::Instance().Clear();
}

// 互動觸及範圍的修正不得開出「可穿越」的漏洞。玩家持續數格往苦主擠；碰撞箱必須讓玩家的
// 箱永不嚴格重疊 NPC 的箱（貼齊可以，穿過不行）。
TEST_CASE("I3: player still cannot walk through a static NPC") {
    nccu::dialog::SetContentDir(TEST_CONTENT_DIR);
    nccu::engine::platform::Time::SetFixedStep(1.0f / 60.0f);
    EventBus::Instance().Clear();

    World world("", /*loadSprites=*/false);
    nccu::GameController controller{world, EventBus::Instance()};
    const GameObject* v = FindNpc(world, "victim");
    REQUIRE(v != nullptr);
    const float vx = v->GetPosition().x;
    const float vy = v->GetPosition().y;

    // 把玩家放在苦主 (1660,1010) 正南方的淨空列上，往北用力擠——在 Y 軸上演練同樣的
    // 不可穿越保證（苦主已不在生成列上）。
    world.GetPlayer()->SetPosition(nccu::engine::math::Vec2{vx, vy + 90.0f});

    TestInput in;
    nccu::engine::input::Input::SetSource(&in);
    in.Hold(Key::W);                                   // 用力往上擠
    for (int f = 0; f < 1200; ++f) {
        Frame(controller, in);
        const auto pos = world.GetPlayer()->GetPosition();
        // 在共用的列上，玩家的 24x24 箱絕不得嚴格重疊苦主的 24x24 箱：其上緣須維持
        // >= vy（貼齊），絕不越過 NPC 的另一側。
        const bool sameCol = !(pos.x >= vx + 24.0f || pos.x + 24.0f <= vx);
        if (sameCol) CHECK(pos.y >= vy);               // 不可穿越
    }
    const float endY = world.GetPlayer()->GetPosition().y;
    CHECK(endY >= vy);                                  // 最終貼齊，未越過

    nccu::engine::input::Input::SetSource(nullptr);
    nccu::engine::platform::Time::SetFixedStep(0.0f);
    EventBus::Instance().Clear();
}

// 與 Vendor 互動必須經由選擇 UI 路由到 TryBuy。驅動 Ch4 集英樓攤位（販售醜傘，設定旗標
// kFlagBoughtUglyUmbrella → 結局 C）。E 開啟購買選單、E 確認唯一的庫存項；購買事件觸發、
// 扣錢、背包與結局旗標更新——全在 Vendor::TryBuy 內完成。
TEST_CASE("I5: Vendor interaction routes to TryBuy (Ch4 ugly umbrella)") {
    nccu::dialog::SetContentDir(TEST_CONTENT_DIR);
    nccu::engine::platform::Time::SetFixedStep(1.0f / 60.0f);
    EventBus::Instance().Clear();

    int pickupHits = 0;
    std::string lastPickup;
    EventBus::Instance().Subscribe(EventType::PickupAcquired,
        [&](const Event& e) { ++pickupHits; lastPickup = e.text; });

    World world("", /*loadSprites=*/false);
    nccu::GameController controller{world, EventBus::Instance()};

    // 驅動 FSM 到 Ch4；GameController 會在下一次 Update() 重生 roster（含集英樓 Vendor）。
    world.Semester().Transition(SemesterState::Chapter4_Finals);
    TestInput in;
    nccu::engine::input::Input::SetSource(&in);
    Frame(controller, in);                             // roster -> Ch4

    const GameObject* vend = FindVendor(world);
    REQUIRE(vend != nullptr);
    const float vx = vend->GetPosition().x;
    const float vy = vend->GetPosition().y;
    Player* p = world.GetPlayer();
    REQUIRE(p != nullptr);
    p->AddMoney(300);                                  // 足夠買 100 元的傘
    const int money0 = p->GetMoney();
    CHECK_FALSE(p->HasFlag(nccu::kFlagBoughtUglyUmbrella));

    // 傳送到緊鄰處（本測試針對購買的接線，非尋路；I3 案例已證明走近 + E 能抵達對話）。
    p->SetPosition(nccu::engine::math::Vec2{vx - 8.0f, vy});

    in.Tap(Key::E);                                    // 開啟購買選單
    Frame(controller, in);
    REQUIRE(world.Dialog().Active());

    // 翻過招呼語到庫存選項。
    for (int f = 0; f < 16 && !world.Dialog().AtChoice(); ++f) {
        in.Tap(Key::E);
        Frame(controller, in);
    }
    REQUIRE(world.Dialog().AtChoice());
    // 選單現為一個庫存項「加上」一個結尾的「不買」選項（強迫購買是缺陷）。choiceCursor_
    // 預設為 0（庫存項），故按一次 E 仍如舊地確認購買；接線不變，僅選單因新增不買項而多
    // 一列。挑選不買時不會改變任何狀態的證明由 test_vendor_decline.cpp 負責。
    REQUIRE(world.Dialog().Choices().size() == 2);     // 庫存 + 不買
    CHECK(world.Dialog().Choices().back().label == "先不買，謝謝");
    CHECK(world.Dialog().ChoiceCursor() == 0);         // 預設停在購買

    in.Tap(Key::E);                                    // 確認購買
    Frame(controller, in);

    CHECK(p->GetMoney() == money0 - 100);              // 經濟（soft-cap）不變
    CHECK(p->HasFlag(nccu::kFlagBoughtUglyUmbrella));      // 結局 C 的關鍵旗標
    // 醜傘的購買現為「手持傘」，而非可計數的消耗品——玩家持有它（自動遮蔽），且背包只顯示
    // 單獨一列醜傘，「沒有」幽靈般的 "UglyUmbrella" 消耗品項。
    CHECK(p->ConsumableCount("UglyUmbrella") == 0);    // 非消耗品
    CHECK(p->HeldUmbrellaKind() == HeldUmbrella::Ugly);
    CHECK(p->HasUmbrella());                           // 現已遮蔽
    CHECK_FALSE(p->HasFlag(nccu::kFlagHasTrueUmbrella));   // 不是真傘
    {
        const auto rows = nccu::BuildInventoryRows(*p);
        int umbRows = 0;
        for (const auto& r : rows)
            if (r.itemId == nccu::kItemUglyUmbrella) ++umbRows;
        CHECK(umbRows == 1);                           // 恰好一列，不重複
    }
    CHECK(pickupHits == 1);                            // EventBus 購買事件
    CHECK(lastPickup == "UglyUmbrella");

    nccu::engine::input::Input::SetSource(nullptr);
    nccu::engine::platform::Time::SetFixedStep(0.0f);
    EventBus::Instance().Clear();
}

// Ch2 主線的兩階段流程。清關需要背包裡有 EnergyDrink 來喚醒學霸（只有 Vendor 購買能供給）。
// 在 Ch2 自動販賣機買到；與學霸對話一次 -> 第一階段消耗飲料並喚醒他（kFlagBookworm，啟動
// 撿筆記任務）；湊齊 3 份筆記後第二次對話 -> 第二階段交換（kFlagBookwormRecovered）；關閉
// 道謝對話後，LiftChapter2Clear 設定 kFlagCh2Cleared。
TEST_CASE("I5: Ch2 progression — buy EnergyDrink, wake 學霸, Flag_Ch2Cleared") {
    nccu::dialog::SetContentDir(TEST_CONTENT_DIR);
    nccu::engine::platform::Time::SetFixedStep(1.0f / 60.0f);
    EventBus::Instance().Clear();

    World world("", /*loadSprites=*/false);
    nccu::GameController controller{world, EventBus::Instance()};

    world.Semester().Transition(SemesterState::Chapter2_Midterms);
    TestInput in;
    nccu::engine::input::Input::SetSource(&in);
    Frame(controller, in);                             // roster -> Ch2

    Player* p = world.GetPlayer();
    REQUIRE(p != nullptr);
    // 3 份散落筆記是另一條正交的撿取任務；先預設它們，讓本測試專注隔離 EnergyDrink 的供給
    //（救援需要筆記齊全「且」有飲料——這裡演練飲料路徑）。
    p->SetFlag(nccu::kFlagFoundNote1);
    p->SetFlag(nccu::kFlagFoundNote2);
    p->SetFlag(nccu::kFlagFoundNote3);
    CHECK(p->ConsumableCount("EnergyDrink") == 0);     // 買到前皆無

    // --- 在 Ch2 自動販賣機購買 EnergyDrink。 ---
    const GameObject* vend = FindVendor(world);
    REQUIRE(vend != nullptr);
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
    in.Tap(Key::E);                                    // 確認購買
    Frame(controller, in);
    CHECK(p->ConsumableCount("EnergyDrink") == 1);     // <-- EnergyDrink 供給已修

    // --- 硬性前置：先見圖書館管理員。她是 Ch2 鏈的起點——在 kFlagMetLibrarian 設定前
    // 學霸無法被喚醒（喚醒步驟會拒絕，opener 改導向櫃台）。走向她並按 E 觸發真正的
    // TryMeetLibrarian 鉤子。 ---
    const GameObject* lib = FindNpc(world, "librarian");
    REQUIRE(lib != nullptr);
    p->SetPosition(nccu::engine::math::Vec2{lib->GetPosition().x - 8.0f,
                                   lib->GetPosition().y});
    in.Tap(Key::E);
    Frame(controller, in);
    CHECK(p->HasFlag(nccu::kFlagMetLibrarian));         // 鏈起點已會面
    for (int f = 0; f < 16 && world.Dialog().Active(); ++f) {
        in.Tap(Key::E);
        Frame(controller, in);
    }

    // --- 與學霸對話，第一階段：消耗飲料 -> 喚醒他（kFlagBookworm）。新的兩階段流程不再
    // 一步完成；喚醒才是啟動撿筆記任務的動作。 ---
    const GameObject* bw = FindNpc(world, "bookworm");
    REQUIRE(bw != nullptr);
    p->SetPosition(nccu::engine::math::Vec2{bw->GetPosition().x - 8.0f,
                                   bw->GetPosition().y});
    in.Tap(Key::E);
    Frame(controller, in);
    CHECK(p->HasFlag(nccu::kFlagBookworm));                // 已喚醒
    CHECK(p->ConsumableCount("EnergyDrink") == 0);     // 飲料在喚醒時消耗
    CHECK_FALSE(p->HasFlag(nccu::kFlagBookwormRecovered)); // 尚未——需要筆記

    // 關閉喚醒對話，使下一次 E 是全新的互動。
    for (int f = 0; f < 16 && world.Dialog().Active(); ++f) {
        in.Tap(Key::E);
        Frame(controller, in);
    }

    // --- 與學霸對話，第二階段：筆記已齊（上面預設）-> 交換：kFlagBookwormRecovered。
    // 不再消耗飲料。 ---
    p->SetPosition(nccu::engine::math::Vec2{bw->GetPosition().x - 8.0f,
                                   bw->GetPosition().y});
    in.Tap(Key::E);
    Frame(controller, in);
    CHECK(p->HasFlag(nccu::kFlagBookwormRecovered));

    // 關閉學霸的道謝對話；LiftChapter2Clear 會在下一個非對話格設定 kFlagCh2Cleared，即
    // 主線的 Ch2 清關。
    for (int f = 0; f < 32 && world.Dialog().Active(); ++f) {
        in.Tap(Key::E);
        Frame(controller, in);
    }
    CHECK_FALSE(world.Dialog().Active());
    Frame(controller, in);                             // LiftChapter2Clear
    CHECK(p->HasFlag(nccu::kFlagCh2Cleared));              // <-- Ch2 主線清關

    nccu::engine::input::Input::SetSource(nullptr);
    nccu::engine::platform::Time::SetFixedStep(0.0f);
    EventBus::Instance().Clear();
}
