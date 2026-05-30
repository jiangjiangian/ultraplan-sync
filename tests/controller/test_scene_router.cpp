#include "doctest/doctest.h"
#include "game/quest/Flags.h"
#include "game/state/ChapterToast.h"
#include "engine/events/EventBus.h"
#include "game/controller/EventWiring.h"
#include "game/state/InterludeExit.h"
#include "game/entities/Player.h"
#include "game/controller/SceneRouter.h"
#include "game/quest/ItemCatalog.h"
#include "game/world/World.h"
#include "engine/math/Vec2.h"

#include <algorithm>
#include <string>
#include <string_view>
#include <vector>

using nccu::SceneRouter;
using nccu::SemesterState;
using nccu::World;

/**
 * @file test_scene_router.cpp
 * @brief 驗證 SceneRouter：章節／interlude／ending 轉場的觀察者，將「換 roster」與
 *        「套用副作用」拆成兩半，使可見的 bug（npcs[] 名單延遲一格）關閉，同時保持
 *        副作用半部與舊的單塊行為等價。
 *
 * 兩半各自的職責：
 *   SettleRoster      — Update 末段；只做 roster 換班。
 *   SettleSideEffects — Update 前段；玩家位置 / 消耗品 / 事件。
 *
 * 這些測試釘住的契約：
 *   1. 狀態改變時 SettleRoster 確實重生 NPC。
 *   2. SettleRoster 對自己的 cursor 為冪等。
 *   3. 進入 Interlude 時 SettleSideEffects 移動玩家、清空消耗品、發出抵達提示、重置出口 latch。
 *   4. 進入 Ch4 時 SettleSideEffects 清除雨傘狀態。
 *   5. 同一 tick 內先 SettleRoster 再 SettleSideEffects，副作用半部仍與拆分前的單塊行為
 *      等價——不重複生成、不重複發佈。
 */

namespace {

bool HasNpcId(const World& w, const char* id) {
    for (const auto& o : w.Objects())
        if (o->NpcId() == std::string_view(id)) return true;
    return false;
}

// 取得最新一筆 ShowMessage 內容——用以驗證抵達提示是否在正確的呼叫時機發佈。
[[nodiscard]] EventBus::Subscription
SubscribeToLatest(std::string& latest) {
    return EventBus::Instance().ScopedSubscribe(
        EventType::ShowMessage,
        [&latest](const Event& e) { latest = e.text; });
}

} // namespace

// ctor 後兩個 cursor 都從初始狀態起算。
TEST_CASE("SceneRouter ctor：兩個 cursor 都從初始狀態起算") {
    SceneRouter r{SemesterState::Chapter1_AddDrop};
    CHECK(r.LastRosterState() == SemesterState::Chapter1_AddDrop);
    CHECK(r.LastRosterRespawnState() == SemesterState::Chapter1_AddDrop);
    CHECK(r.InterludeExitLatchMut() == false);
}

// FSM 未移動時 SettleRoster 為 no-op。
TEST_CASE("SettleRoster：FSM 未移動時為 no-op") {
    World w("", /*loadSprites=*/false);
    SceneRouter r{w.Semester().Current()};
    REQUIRE(HasNpcId(w, "victim"));        // Ch1 roster 已存在

    r.SettleRoster(w);                     // FSM 仍在 Ch1
    CHECK(HasNpcId(w, "victim"));          // 不變
    CHECK(r.LastRosterRespawnState() == SemesterState::Chapter1_AddDrop);
}

// 發生轉場時 SettleRoster 重生新章節的 NPC。
TEST_CASE("SettleRoster：發生轉場時重生新章節的 NPC") {
    World w("", /*loadSprites=*/false);
    SceneRouter r{w.Semester().Current()};
    REQUIRE(HasNpcId(w, "victim"));        // Ch1 NPC

    // FSM 在不經過 SceneRouter 的情況下推進（模擬 CheckChapterGates / EventWiring 的
    // 轉場在 controller 的 Update 內觸發）。
    w.Semester().Transition(SemesterState::Chapter2_Midterms);
    r.SettleRoster(w);

    // Ch2 roster 在 FSM 移動的「同一格」就對 View（及 npcs[]）可見——拆分前此處會慢一格。
    CHECK(HasNpcId(w, "librarian"));       // Ch2 專屬任務給予者
    CHECK(HasNpcId(w, "victim"));          // 苦主在 Ch2 也在
    CHECK(r.LastRosterRespawnState() == SemesterState::Chapter2_Midterms);
}

// SettleRoster 不會動到 SettleSideEffects 的 cursor（拆分確實生效）。
TEST_CASE("SettleRoster：不會動到 SettleSideEffects 的 cursor（拆分確實生效）") {
    World w("", /*loadSprites=*/false);
    SceneRouter r{w.Semester().Current()};

    w.Semester().Transition(SemesterState::Chapter2_Midterms);
    r.SettleRoster(w);

    // 「副作用 cursor」不得前進——它追蹤的是 Update 前段那一半，本測試尚未呼叫。若兩個
    // cursor 一起移動，下一次 SettleSideEffects 會變成 no-op，玩家便永遠不會被重新定位。
    CHECK(r.LastRosterRespawnState() == SemesterState::Chapter2_Midterms);
    CHECK(r.LastRosterState()        == SemesterState::Chapter1_AddDrop);
}

// 進入 Interlude：SettleSideEffects 同時處理位置、消耗品、提示與 latch。
TEST_CASE("SettleSideEffects 進入 Interlude：位置、消耗品、提示與 latch") {
    EventBus::Instance().Clear();
    std::string latestHud;
    auto sub = SubscribeToLatest(latestHud);

    World w("", /*loadSprites=*/false);
    SceneRouter r{w.Semester().Current()};

    Player* p = w.GetPlayer();
    REQUIRE(p != nullptr);

    // 前置狀態：假裝玩家走離 Interlude 入口很遠、身上帶著 Ch1 消耗品，且南側 latch 先前已觸發。
    p->SetPosition(nccu::engine::math::Vec2{100.0f, 100.0f});
    p->AddConsumable("EnergyDrink");
    p->AddConsumable("EnergyDrink");
    r.InterludeExitLatchMut() = true;
    REQUIRE(p->ConsumableCount("EnergyDrink") == 2);

    // FSM 推進到 Interlude（模擬 EventWiring 的 Ch1→IL 轉場）。
    w.Semester().Transition(SemesterState::Interlude_Market);
    r.SettleSideEffects(w);

    // 四項可見副作用同時發生：
    CHECK(p->GetPosition().x == doctest::Approx(nccu::kInterludeEntry.x));
    CHECK(p->GetPosition().y == doctest::Approx(nccu::kInterludeEntry.y));
    CHECK(p->ConsumableCount("EnergyDrink") == 0);     // 已清空
    CHECK(latestHud == nccu::kInterludeArrivalHint);
    CHECK(r.InterludeExitLatchMut() == false);

    // cursor 已戳記，故重複呼叫為 no-op。
    CHECK(r.LastRosterState() == SemesterState::Interlude_Market);
    p->SetPosition(nccu::engine::math::Vec2{42.0f, 42.0f});
    r.SettleSideEffects(w);                            // 冪等
    CHECK(p->GetPosition().x == doctest::Approx(42.0f));

    EventBus::Instance().Clear();
}

// 進入 Ch4：SettleSideEffects 清除雨傘與 TrueUmbrella 旗標。
TEST_CASE("SettleSideEffects 進入 Ch4：清除雨傘與 TrueUmbrella 旗標") {
    EventBus::Instance().Clear();

    World w("", /*loadSprites=*/false);
    SceneRouter r{w.Semester().Current()};

    Player* p = w.GetPlayer();
    REQUIRE(p != nullptr);
    p->SetHasUmbrella(true);
    p->SetFlag(nccu::kFlagHasTrueUmbrella);

    // FSM 直接跳到 Ch4（模擬 ChapterGate 的 Interlude→Ch4）。
    w.Semester().Transition(SemesterState::Chapter4_Finals);
    r.SettleSideEffects(w);

    // 劇情上玩家走出集英樓時又沒了傘。Ch4 分支即此設定的強制執行。
    CHECK_FALSE(p->HasUmbrella());
    CHECK_FALSE(p->HasFlag(nccu::kFlagHasTrueUmbrella));
    CHECK(r.LastRosterState() == SemesterState::Chapter4_Finals);

    EventBus::Instance().Clear();
}

// 每章「傘又掉了」現已機制上成立：進入 Ch2/Ch3/Ch4 任一者都會清除手持的傘（原本只在
// Ch4 清除，導致真傘殘留在 Ch2 背包）。
TEST_CASE("SettleSideEffects 進入 Ch2 時清除手持的傘") {
    EventBus::Instance().Clear();
    World w("", /*loadSprites=*/false);
    SceneRouter r{w.Semester().Current()};
    Player* p = w.GetPlayer();
    REQUIRE(p != nullptr);

    // 假裝玩家從 Ch1 帶著一把手持傘（例如苦主交還的真傘）進入背包。
    p->SetHeldUmbrella(HeldUmbrella::True);
    p->SetFlag(nccu::kFlagHasTrueUmbrella);
    REQUIRE(p->HeldUmbrellaKind() == HeldUmbrella::True);

    w.Semester().Transition(SemesterState::Chapter2_Midterms);
    r.SettleSideEffects(w);

    CHECK_FALSE(p->HasUmbrella());
    CHECK(p->HeldUmbrellaKind() == HeldUmbrella::None);   // 背包的傘消失
    CHECK_FALSE(p->HasFlag(nccu::kFlagHasTrueUmbrella));
    EventBus::Instance().Clear();
}

// 進入 Ch3 時也清除手持的傘。
TEST_CASE("SettleSideEffects 進入 Ch3 時清除手持的傘") {
    EventBus::Instance().Clear();
    World w("", /*loadSprites=*/false);
    SceneRouter r{w.Semester().Current()};
    Player* p = w.GetPlayer();
    REQUIRE(p != nullptr);

    // Ch3 接在 Ch2 之後；假裝有把 Ch2 借來的傘殘留。
    p->SetHeldUmbrella(HeldUmbrella::Loaner);
    REQUIRE(p->HeldUmbrellaKind() == HeldUmbrella::Loaner);

    w.Semester().Transition(SemesterState::Chapter3_SportsDay);
    r.SettleSideEffects(w);

    CHECK_FALSE(p->HasUmbrella());
    CHECK(p->HeldUmbrellaKind() == HeldUmbrella::None);   // 借來的傘消失
    EventBus::Instance().Clear();
}

// 端到端流程：同一 tick 內先 SettleRoster、下一格再 SettleSideEffects，各半皆冪等。
TEST_CASE("端到端：同一 tick 內先 SettleRoster 再 SettleSideEffects") {
    // 釘住完整流程：轉場在某格中途觸發（在對話分支 / E-probe / CheckChapterGates 等之後）；
    // controller 在 Update 末段呼叫 SettleRoster，使 View 接下來的 Draw 以新章節的 NPC 繪製。
    // 下一格的 Update 前段，SettleSideEffects 才做玩家位置傳送 + 抵達提示。兩半每次轉場各
    // 執行恰好一次（在各自 cursor 下冪等）。
    EventBus::Instance().Clear();
    std::string latestHud;
    auto sub = SubscribeToLatest(latestHud);

    World w("", /*loadSprites=*/false);
    SceneRouter r{w.Semester().Current()};
    Player* p = w.GetPlayer();
    REQUIRE(p != nullptr);

    // 模擬撿傘的轉場，但不驅動整套事件匯流排：只移動 FSM（SceneRouter 只關心 FSM 狀態，
    // 不輪詢 EventBus）。
    w.Semester().Transition(SemesterState::Interlude_Market);

    // ---- 第 N 格：Update 末段 SettleRoster ----
    REQUIRE(HasNpcId(w, "victim"));        // 呼叫前 Ch1 NPC 仍在
    r.SettleRoster(w);
    CHECK_FALSE(HasNpcId(w, "victim"));    // IL 的 roster 為空
    // 玩家尚未被傳送——可見的觀察值不變。（本測試自己不移動玩家；此處僅驗證 SettleRoster
    // 這一趟「沒有」發佈抵達提示或清空消耗品——那是下一格 SettleSideEffects 的工作。）
    CHECK(latestHud.empty());

    // ---- 第 N+1 格：Update 前段 SettleSideEffects ----
    r.SettleSideEffects(w);
    CHECK(p->GetPosition().x == doctest::Approx(nccu::kInterludeEntry.x));
    CHECK(p->GetPosition().y == doctest::Approx(nccu::kInterludeEntry.y));
    CHECK(latestHud == nccu::kInterludeArrivalHint);

    // 重複呼叫時兩半皆冪等。
    p->SetPosition(nccu::engine::math::Vec2{7.0f, 8.0f});
    latestHud.clear();
    r.SettleRoster(w);
    r.SettleSideEffects(w);
    CHECK(p->GetPosition().x == doctest::Approx(7.0f));  // 未被重新傳送
    CHECK(latestHud.empty());                            // 未重複發佈

    EventBus::Instance().Clear();
}

// 若 SettleRoster 被略過，SettleSideEffects 會防禦性地重生 roster。
TEST_CASE("SettleRoster 被略過時 SettleSideEffects 會防禦性地重生 roster") {
    // 拆分讓兩半都能各自獨立執行，故繞過 SettleRoster 的呼叫端（例如測試，或未來只走
    // Update 前段分支的程式路徑）仍能取得一致的 roster。當 SettleSideEffects 的 respawn
    // cursor 與 FSM 狀態不一致時，它會呼叫 RespawnChapterRoster。
    EventBus::Instance().Clear();

    World w("", /*loadSprites=*/false);
    SceneRouter r{w.Semester().Current()};
    REQUIRE(HasNpcId(w, "victim"));        // Ch1

    // FSM 在未先經過 SettleRoster 的情況下移動。
    w.Semester().Transition(SemesterState::Chapter2_Midterms);
    r.SettleSideEffects(w);

    // roster 現為 Ch2——防禦性的 RespawnChapterRoster 已觸發。
    CHECK(HasNpcId(w, "librarian"));       // Ch2 NPC
    CHECK(r.LastRosterRespawnState() == SemesterState::Chapter2_Midterms);
    CHECK(r.LastRosterState() == SemesterState::Chapter2_Midterms);

    EventBus::Instance().Clear();
}

// 跨 Interlude 的背包倖存者：Ch1 → Interlude → Ch2 後，唯一可留存的列只有金幣（跨章節
// 金錢）與申請書（跨章節攜帶物，TA 線仰賴）。消耗品在進市集時清空、手持傘在進 Ch2 時清除，
// 故兩者都不留存。
TEST_CASE("跨 Interlude 後背包只留下金幣與申請書") {
    EventBus::Instance().Clear();
    World w("", /*loadSprites=*/false);
    SceneRouter r{w.Semester().Current()};
    Player* p = w.GetPlayer();
    REQUIRE(p != nullptr);

    // Ch1 末的背包：金錢（留存）、申請書（攜帶物）、手持傘（苦主的真傘）、以及在市集買的
    // 一個 Ch1 消耗品。
    p->SetFlag(nccu::kFlagFoundForm);
    p->SetHeldUmbrella(HeldUmbrella::True);
    p->SetFlag(nccu::kFlagHasTrueUmbrella);
    p->AddConsumable("EnergyDrink");
    {
        const auto rows = nccu::BuildInventoryRows(*p);
        // 健全性檢查：轉場前背包「確實」持有傘 + 消耗品
        bool hasUmb = false, hasDrink = false;
        for (const auto& row : rows) {
            if (row.itemId == nccu::kItemTrueUmbrella) hasUmb = true;
            if (row.itemId == "EnergyDrink")           hasDrink = true;
        }
        REQUIRE(hasUmb);
        REQUIRE(hasDrink);
    }

    // 進入 Interlude 清空消耗品（並重新定位、提示、latch）。
    w.Semester().Transition(SemesterState::Interlude_Market);
    r.SettleSideEffects(w);
    // 進入 Ch2 清除手持傘。
    w.Semester().Transition(SemesterState::Chapter2_Midterms);
    r.SettleSideEffects(w);

    const auto rows = nccu::BuildInventoryRows(*p);
    // 恰好兩種倖存類別，依固定的 catalog 順序：先金錢、再申請書。別無其他。
    std::vector<std::string> ids;
    for (const auto& row : rows) ids.push_back(row.itemId);
    CHECK(ids == std::vector<std::string>{nccu::kItemMoney, nccu::kItemForm});
    // 明確的反例：沒有傘的列，也沒有消耗品的列。
    CHECK(std::none_of(rows.begin(), rows.end(), [](const nccu::InventoryRow& row) {
        return row.itemId.find("umbrella") != std::string::npos;
    }));
    CHECK(p->ConsumableCount("EnergyDrink") == 0);

    EventBus::Instance().Clear();
}
