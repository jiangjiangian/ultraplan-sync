#include "doctest/doctest.h"
#include "game/quest/Flags.h"
#include "engine/platform/ScriptInput.h"
#include "game/controller/GameController.h"
#include "engine/events/EventBus.h"
#include "game/world/World.h"
#include "game/entities/Player.h"
#include "game/dialog/DialogState.h"
#include "game/dialog/DialogSource.h"
#include "engine/core/GameObject.h"
#include "engine/input/Input.h"
#include "engine/platform/Time.h"

#include <cmath>
#include <sstream>
#include <string>
#include <vector>

/**
 * @file test_scriptinput_plan.cpp
 * @brief 驗證 ScriptInput 的高階計畫動詞（goto / interact / choose / advance），讓時間軸能
 *        驅動遊戲而不必逐格手算。驅動方式與 harness 相同：ScriptInput 為 Input source，迴圈
 *        每格 Advance() 後對「前一格 EndFrame 擷取的 World 快照」ResolvePlan()，再 Update()。
 *        解析器是 (計畫步驟, World 快照) 的純函式（無 wall-clock、無 RNG），故同一腳本兩次
 *        執行的軌跡逐位元相同。
 */

#ifndef TEST_CONTENT_DIR
#error "TEST_CONTENT_DIR must be defined by the build system"
#endif

using nccu::ScriptInput;
using nccu::World;

namespace {

// 每格的可觀察軌跡 token；兩次執行須逐元素相等。
struct Frame {
    float x, y;
    bool  dialog;
    int   cursor;
    std::string npc;
    bool operator==(const Frame& o) const {
        return x == o.x && y == o.y && dialog == o.dialog &&
               cursor == o.cursor && npc == o.npc;
    }
};

// 對全新 World 跑 `script` 至多 `maxFrames` 格，忠實對應 Harness::BeginFrame/EndFrame 的
// 順序。離開時以 RAII 還原 live input source / 真實 timestep。
std::vector<Frame> RunPlan(const std::string& script, int maxFrames) {
    nccu::dialog::SetContentDir(TEST_CONTENT_DIR);
    nccu::engine::platform::Time::SetFixedStep(1.0f / 60.0f);

    World world("", /*loadSprites=*/false);
    nccu::GameController controller{world, EventBus::Instance()};

    ScriptInput in;
    std::istringstream src(script);
    in.Load(src);
    nccu::engine::input::Input::SetSource(&in);

    std::vector<Frame> trace;
    const World* snapshot = nullptr;   // 第 0 格為 null，與 harness 相同
    for (int f = 0; f < maxFrames; ++f) {
        in.Advance();
        in.ResolvePlan(snapshot);
        controller.Update();
        snapshot = &world;             // 於 EndFrame 擷取

        const Player* p = world.GetPlayer();
        const auto& d   = world.Dialog();
        trace.push_back(Frame{
            p ? p->GetPosition().x : 0.0f,
            p ? p->GetPosition().y : 0.0f,
            d.Active(), d.ChoiceCursor(), d.NpcId()});

        if (in.WantsQuit() || (in.HasPlan() && f >= 1 && in.PlanDone()))
            break;
    }

    nccu::engine::input::Input::SetSource(nullptr);
    nccu::engine::platform::Time::SetFixedStep(0.0f);
    return trace;
}

const GameObject* FindNpc(const World& w, const char* id) {
    for (const auto& u : w.Objects())
        if (u && u->IsActive() && u->NpcId() == id) return u.get();
    return nullptr;
}

// Ch1 苦主移到了綜合院館 (1660,1010)，位於南側校園牆以北，故純軸向驅動的 `interact victim`
// 無法從 (500,1860) 生成點直達他。此繞行路線把玩家停在苦主正南方、淨空的 x=1660 列上；尾端的
// 動詞才做貼齊靠近。
const char* const kRouteToVictimStaging =
    "goto 1040 1712\n" "goto 1048 1704\n" "goto 1264 1632\n"
    "goto 1280 1624\n" "goto 1296 1616\n" "goto 1312 1608\n"
    "goto 1328 1600\n" "goto 1408 1512\n" "goto 1416 1504\n"
    "goto 1424 1496\n" "goto 1432 1488\n" "goto 1448 1480\n"
    "goto 1496 1456\n" "goto 1504 1448\n" "goto 1512 1440\n"
    "goto 1520 1432\n" "goto 1528 1424\n" "goto 1544 1328\n"
    "goto 1552 1320\n" "goto 1568 1312\n" "goto 1584 1304\n"
    "goto 1592 1296\n" "goto 1660 1120\n";

}  // namespace

// `goto` 是玩家位置 + 3 px/格的純函式：必須落在抵達誤差內，且不越過。
TEST_CASE("plan：`goto` 在 epsilon 內把玩家驅動到淨空目標") {
    // 玩家生成在 {500,1860} 的開闊南側道路。沿路往東到 x=1000（唯一的牆缺口），再直上淨空的
    // 通道到 y=1300——全程無牆且無 NPC（已對照地形遮罩與預設 NPC 生成點驗證）。
    const std::vector<Frame> tr =
        RunPlan("goto 1000 1860\ngoto 1000 1300\n", 3000);

    REQUIRE_FALSE(tr.empty());
    const Frame& last = tr.back();
    CHECK(std::fabs(last.x - 1000.0f) < 3.0f);  // 在一格的行進距離內
    CHECK(std::fabs(last.y - 1300.0f) < 3.0f);
}

// `interact <npcId>` 查找 NPC 的即時世界位置並確定性地驅動玩家前往（兩個位置的純函式）。
// 動詞會朝 NPC 原點直行，並在 8 px（kInteractReach）充氣後的玩家 AABB 與 NPC 重疊時按 E，
// 約在移動碰撞箱貼齊停住前 8 px；controller 隨即開啟對話並返回（對話開著時不移動），使玩家
// 凍結在離原點約 27 px 處（貼齊 24 + 觸及邊際）。故驅動終點落在 [約 24, 24+8] 的觸及帶內，
// 而非剛好貼齊 24。本案例守護驅動與確定性；對話確實「開啟」由 test_i6_interact_reach.cpp 守護。
TEST_CASE("plan：`interact victim` 確定性地抵達該 NPC") {
    World probeWorld("", /*loadSprites=*/false);
    const GameObject* v = FindNpc(probeWorld, "victim");
    REQUIRE(v != nullptr);
    const float vx = v->GetPosition().x;        // {1660,1010}
    const float vy = v->GetPosition().y;

    const std::vector<Frame> tr =
        RunPlan(std::string(kRouteToVictimStaging) + "interact victim\n", 4000);
    REQUIRE_FALSE(tr.empty());
    const Frame& last = tr.back();

    // 玩家經缺口繞行後，駛入觸及帶：在靠近軸（此處為 Y——停在苦主南方）貼齊 24 加上 8 px 的
    // kInteractReach 邊際（對話在貼齊前開啟，玩家隨即為對話凍結），並在另一軸（X）精準對齊。
    CHECK(std::fabs(last.y - vy) <= 24.0f + 8.0f);   // 觸及帶
    CHECK(std::fabs(last.x - vx) <= 24.0f);
    CHECK((std::fabs(last.y - vy) >= 23.0f ||
           std::fabs(last.x - vx) < 1.0f));     // 抵達緊鄰，而非遠處
}

// 動詞的實際操作（驅動 + E）在「不擋路」目標上端到端正確：玩家走到苦主雨傘的 QuestFlagPickup
// 上（一個 BlocksMovement()==false 的 Item，玩家可重疊），按一次 E 即抵達遊戲的 OnPickup 路徑——
// 證明合成的 edge 對 GameController 而言與手動腳本化輸入無法區分。
//
// 世界中的 TrueUmbrella 已被移除（改由苦主授予），故舊的「走到 TrueUmbrella{320,1280}」目標
// 不再存在。集英樓南方 (1700,1610) 可找到的苦主雨傘 pickup 是自然的非擋路 Item 替代。路線：
// 只經 x 缺口跨過南側牆，再沿淨空的東側走廊上行到開闊的雨傘地點。斷言 kFlagHasVictimUmbrella
//（僅由該 QuestFlagPickup 的 OnPickup 設定），故確實演練了驅動+E→OnPickup 路徑，而不只是最終
// 座標。對照：test_i6_interact_reach.cpp（NPC 對話觸及）。
TEST_CASE("plan：goto+E 在可達的（非擋路）item 上實際操作遊戲") {
    nccu::dialog::SetContentDir(TEST_CONTENT_DIR);
    nccu::engine::platform::Time::SetFixedStep(1.0f / 60.0f);
    World world("", /*loadSprites=*/false);
    nccu::GameController controller{world, EventBus::Instance()};

    // 硬性前置：苦主的雨傘 pickup 現為延遲生成——只在西裝學長的選擇提交後
    //（kFlagSuitSeniorChoiceMade）才透過 MaybeSpawnChapter1VictimUmbrella（由 controller
    // 推進）生成。先設定該選擇旗標，使延遲生成在第一格觸發，本測試才仍能演練其命名所指的真正
    // 驅動+E→OnPickup 路徑。
    REQUIRE(world.GetPlayer() != nullptr);
    world.GetPlayer()->SetFlag(nccu::kFlagSuitSeniorChoiceMade);

    ScriptInput in;
    std::istringstream src(
        "goto 1040 1712\n" "goto 1048 1704\n" "goto 1264 1632\n"
        "goto 1280 1624\n" "goto 1296 1616\n" "goto 1312 1608\n"
        "goto 1328 1600\n" "goto 1700 1610\n"
        "interact victimumb 1700 1610\n" // 走到 Item 上 + E => OnPickup
        "wait 20\n");
    in.Load(src);
    nccu::engine::input::Input::SetSource(&in);

    const World* snap = nullptr;
    bool planDone = false;
    for (int f = 0; f < 8000 && !planDone; ++f) {
        in.Advance();
        in.ResolvePlan(snap);
        controller.Update();
        snap = &world;
        if (in.HasPlan() && f >= 1 && in.PlanDone()) planDone = true;
    }
    nccu::engine::input::Input::SetSource(nullptr);
    nccu::engine::platform::Time::SetFixedStep(0.0f);

    const Player* p = world.GetPlayer();
    REQUIRE(p != nullptr);
    // 驅動+E 端到端抵達 QuestFlagPickup::OnPickup：kFlagHasVictimUmbrella 僅在該處設定，故這
    // 證明合成的 E edge 在一個可達的非擋路 Item 上實際操作了遊戲——正是本案例命名所指。
    CHECK(p->HasFlag(nccu::kFlagHasVictimUmbrella));
}

// 核心保證：同一腳本（混用每種動詞）兩次執行得到逐位元相同的狀態軌跡——相當於兩份相同的
// 記錄檔。
TEST_CASE("plan：replay 具確定性——兩次執行逐位元相同") {
    const std::string script =
        "goto 750 1860\n"
        "interact victim\n"
        "advance\n"
        "goto 750 1280\n";
    const std::vector<Frame> a = RunPlan(script, 3000);
    const std::vector<Frame> b = RunPlan(script, 3000);

    REQUIRE(a.size() == b.size());
    bool identical = true;
    for (std::size_t i = 0; i < a.size(); ++i)
        if (!(a[i] == b[i])) { identical = false; break; }
    CHECK(identical);
}

// 與動詞穿插時，classic 的 `<frame> <action>` 文法仍須維持運作（新增功能，不得造成迴歸）。
TEST_CASE("plan：classic 計時指令與動詞穿插時仍可解析") {
    ScriptInput in;
    std::istringstream src(
        "# a verb and classic lines in one file\n"
        "goto 100 100\n"
        "0 down D\n"
        "2 quit\n");
    in.Load(src);
    CHECK(in.HasPlan());

    in.Advance();                 // 第 0 格：classic D 按下
    CHECK(in.IsDown(nccu::engine::input::Key::D));
    CHECK(in.IsPressed(nccu::engine::input::Key::D));
    in.Advance();                 // 第 1 格：仍按住，無 edge
    CHECK(in.IsDown(nccu::engine::input::Key::D));
    CHECK_FALSE(in.IsPressed(nccu::engine::input::Key::D));
    in.Advance();                 // 第 2 格：classic quit 觸發
    CHECK(in.WantsQuit());
}
