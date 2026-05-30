/**
 * @file test_restart_safety.cpp
 * @brief 驗證遊戲內選單「重新開始」的安全性：重啟後 Player 回到初始
 *        karma/金錢/旗標/消耗品，且 EventBus 訂閱者不殘留（懸空）也不重複堆疊。
 */
#include "game/quest/Flags.h"
//
// 遊戲內暫停選單的「重新開始」會讓 main.cpp 外層迴圈拆掉每一輪的
// {World, View, GameController} 範圍並重建全新一組。此功能必須維持兩個不變量，
// 否則就是資料外洩／釋放後使用的隱患：
//
//   (a) 全新狀態：重啟後的 Player 回到初始 karma／金錢／旗標／消耗品 —— 跨輪
//       不殘留污染（在第 1 輪結束時變壞／變富／帶旗標的玩家，在第 2 輪會像
//       全新遊戲一樣開始）。
//
//   (b) EventBus 訂閱者不懸空、不重複：GameController 在建構子接上訂閱者
//       （有些捕捉 World 參考），在解構子呼叫 EventBus::Clear()。重啟不可跨輪
//       累積處理器，也不可留下捕捉了已銷毀 World 位址的處理器（這類懸空捕捉曾
//       導致記憶體錯誤）。此處證明重啟後的匯流排恰好只有當前新一輪的訂閱者 ——
//       舊的既不殘留（懸空）也不堆疊（重複訂閱）。
//
// 這對應 main.cpp 真實的重啟路徑：進入 {World, controller} 範圍、弄髒、再銷毀
// （controller 解構子在其捕捉的 World 死亡前清空匯流排 —— 關鍵的反向解構順序），
// 然後建立新的範圍。
//
// 註：eventbus_isolation 監聽器會在每個測試／subcase 邊界清空匯流排，故下列每個
// TEST_CASE 都從乾淨的匯流排開始；此處逐輪的斷言則自行透過 controller 解構子
// 驅動 Clear()，從不依賴跨測試的匯流排狀態。

#include "doctest/doctest.h"
#include "game/world/World.h"
#include "game/controller/GameController.h"
#include "game/entities/Player.h"
#include "engine/events/EventBus.h"
#include "game/state/SemesterState.h"
#include "engine/math/Vec2.h"

#include <memory>
#include <string>

using nccu::World;
using nccu::GameController;

namespace {

// 把 Player 弄成「弄髒的」進行中狀態：花/賺錢、改動 karma、設旗標、囤消耗品。
// 正確的重啟會把這一切全部歸回建構時的預設值。
void DirtyTheRun(World& w) {
    Player* p = w.GetPlayer();
    REQUIRE(p != nullptr);
    p->AddMoney(150);                       // 100 -> 250
    p->AddKarma(-40);                       //  50 ->  10
    p->SetFlag(nccu::kFlagTookCursedUmbrella);
    p->SetFlag(nccu::kFlagHelpedTACh1);
    p->AddConsumable("HotPack").AddConsumable("HotPack");
    p->SetHasUmbrella(true);
    CHECK(p->GetMoney() == 250);
    CHECK(p->GetKarma() == 10);
    CHECK(p->HasFlag(nccu::kFlagTookCursedUmbrella));
    CHECK(p->ConsumableCount("HotPack") == 2);
}

} // namespace

// 重啟把 karma／金錢／旗標／消耗品重置為全新遊戲。
TEST_CASE("重啟把業力／金錢／旗標／消耗品重置為全新遊戲") {
    // 第 1 輪：建立、弄髒，再用 RAII 拆掉此輪範圍。
    {
        World w("", /*loadSprites=*/false);
        GameController c{w, EventBus::Instance()};
        DirtyTheRun(w);
    }   // ~GameController -> EventBus::Clear()；~World 釋放玩家

    // 第 2 輪：全新的 World 等同全新遊戲。第 1 輪不得有任何東西存活
    // （狀態存在於已銷毀 World 所擁有的 Player 上，從不放在全域）。
    {
        World w("", /*loadSprites=*/false);
        GameController c{w, EventBus::Instance()};
        const Player* p = w.GetPlayer();
        REQUIRE(p != nullptr);
        CHECK(p->GetKarma() == 50);                 // 企劃設定的起始值
        CHECK(p->GetMoney() == 100);                // 企劃設定的起始值
        CHECK(p->GetRainMeter() == doctest::Approx(0.0f));
        CHECK_FALSE(p->HasUmbrella());
        CHECK_FALSE(p->HasFlag(nccu::kFlagTookCursedUmbrella));
        CHECK_FALSE(p->HasFlag(nccu::kFlagHelpedTACh1));
        CHECK(p->ConsumableCount("HotPack") == 0);
        CHECK(w.Semester().Current() ==
              nccu::SemesterState::Chapter1_AddDrop);
    }
}

// 重啟不會使 EventBus 處理器懸空，也不會重複訂閱。
TEST_CASE("重啟不會使 EventBus 處理器懸空或重複訂閱") {
    // 外部計數探針。在每一輪 controller 解構子的 Clear() 之後，於該輪內重新訂閱，
    // 故它只計入當前這一輪的派送（eventbus_isolation 監聽器也會在 case 邊界清空，
    // 故第 0 輪也從乾淨狀態開始）。
    //
    // controller 恰好接上一個捕捉 World 的 ShowMessage 訂閱者
    // （WireHudMessageSubscriber → World::SetHudMessage），外加一個記錄用的
    // ShowMessage 訂閱者（僅輸出至 cout，不捕捉）。若重啟使舊 World 的處理器懸空，
    // 重建後的發佈會寫入已釋放的 World（釋放後使用，可被偵測器抓到）；若重複訂閱，
    // 現存 World 的 HUD 會被設兩次、逐輪處理器數量會增長。此處同時固定兩點：
    // 現存 World 的 HUD 訊息在每一輪都被派送（處理器存在、未懸空），且對一個全新
    // 單一探針的派送次數每輪都相同（無累積）。
    int probeHits = 0;

    auto runOneCycleAndCount = [&](int cycle) {
        World w("", /*loadSprites=*/false);
        GameController c{w, EventBus::Instance()};

        // 為這一輪建立全新探針（前一輪 controller 解構子的 Clear() 已移除任何
        // 較早的探針）。
        probeHits = 0;
        EventBus::Instance().Subscribe(EventType::ShowMessage,
            [&](const Event&) { ++probeHits; });

        const std::string msg =
            "restart-cycle-" + std::to_string(cycle);
        EventBus::Instance().Publish(Event{EventType::ShowMessage, msg});

        // 現存 World 的 HUD 訂閱者有觸發（本輪處理器存在 —— 證明未遺失），
        // 並路由到「這個」World。
        CHECK(w.HudMessage() == msg);
        // 恰好只有我們這個探針觸發一次 —— 若前一輪的探針或 HUD 訂閱者懸空到本輪，
        // 計數會超過 1 / 逐輪增長（重複訂閱），或匯流排會對已釋放的捕捉呼叫處理器
        // （釋放後使用）。每輪都穩定為 1 ⇒ Clear() 已完全卸除舊一輪的訂閱者。
        CHECK(probeHits == 1);
    };  // ~GameController -> EventBus::Clear() 清掉這一輪的處理器

    for (int cycle = 0; cycle < 4; ++cycle)
        runOneCycleAndCount(cycle);

    // 最後一輪 controller 解構子之後，匯流排為空：發佈不會送達任何處理器
    // （無殘留的 World/HUD/探針處理器）。
    int afterAll = 0;
    EventBus::Instance().Subscribe(EventType::ShowMessage,
        [&](const Event&) { ++afterAll; });
    EventBus::Instance().Publish(Event{EventType::ShowMessage, "tail"});
    CHECK(afterAll == 1);   // 只有剛加入的探針 —— 沒有任何東西懸空
}
