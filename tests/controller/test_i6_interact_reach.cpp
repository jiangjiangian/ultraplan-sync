/**
 * @file test_i6_interact_reach.cpp
 * @brief 驗證 harness 的 `interact <id>` 計畫動詞能真正按下 E 並開啟 NPC 對話。
 *
 * 對會擋路（BlocksMovement）的 NPC，其移動碰撞箱是位於 NPC 原點、玩家大小的箱，physics
 * 會讓玩家恰好貼齊停住（碰到但不嚴格重疊）。本動詞必須朝 NPC 原點直行，並以 GameController
 * 採用的同一個充氣 AABB 觸及測試（kInteractReach）來判定何時按 E，使被牆／碰撞箱貼齊擋住、
 * 走近的玩家仍能對話——正是人類遊玩時所依賴的幾何。
 *
 * 驅動方式與 harness 完全相同：ScriptInput 為 Input source，每格的迴圈對應 Harness
 *（Advance + 對前一格的 World 快照 ResolvePlan，再 controller.Update），故走的是正式產品
 * 接縫，而非單元替身。無頭、確定性、無 GL。
 */

#include "doctest/doctest.h"
#include "engine/platform/ScriptInput.h"
#include "game/controller/GameController.h"
#include "game/world/World.h"
#include "game/entities/Player.h"
#include "game/dialog/DialogState.h"
#include "game/dialog/DialogSource.h"
#include "engine/events/EventBus.h"
#include "engine/core/GameObject.h"
#include "engine/input/Input.h"
#include "engine/platform/Time.h"

#include <cmath>
#include <sstream>
#include <string>
#include <vector>

#ifndef TEST_CONTENT_DIR
#error "TEST_CONTENT_DIR must be defined by the build system"
#endif

using nccu::ScriptInput;
using nccu::World;

namespace {

const GameObject* FindNpc(const World& w, const char* id) {
    for (const auto& u : w.Objects())
        if (u && u->IsActive() && u->NpcId() == id) return u.get();
    return nullptr;
}

// 每格一筆觀察值：對話是否開啟、玩家最終在哪。對應 Harness::BeginFrame/EndFrame 的順序
//（計畫動詞是對「前一格 EndFrame 時擷取的 World」解析）。
struct Outcome {
    bool  dialogOpened = false;
    std::string npc;
    int   openedAtFrame = -1;
    float endX = 0.0f, endY = 0.0f;
    float startX = 0.0f;
};

// Ch1 苦主從 (380,1860) 生成列移到了綜合院館 (1660,1010)，位於南側校園牆以北。harness 的
// `interact <id>` 動詞是純粹的「先 X 後 Y」軸向驅動器（非尋路），無法直接往上走到苦主——必須
// 先經由唯一的缺口列繞行。此繞行路線把玩家停在苦主正南方、淨空的 x=1660 列上；最後的
// `interact victim` 才做貼齊靠近 + E。
const char* const kRouteToVictimStaging =
    "goto 1040 1712\n" "goto 1048 1704\n" "goto 1264 1632\n"
    "goto 1280 1624\n" "goto 1296 1616\n" "goto 1312 1608\n"
    "goto 1328 1600\n" "goto 1408 1512\n" "goto 1416 1504\n"
    "goto 1424 1496\n" "goto 1432 1488\n" "goto 1448 1480\n"
    "goto 1496 1456\n" "goto 1504 1448\n" "goto 1512 1440\n"
    "goto 1520 1432\n" "goto 1528 1424\n" "goto 1544 1328\n"
    "goto 1552 1320\n" "goto 1568 1312\n" "goto 1584 1304\n"
    "goto 1592 1296\n" "goto 1660 1120\n";

Outcome RunInteract(const char* npcId, int maxFrames,
                    const char* preRoute = kRouteToVictimStaging) {
    nccu::dialog::SetContentDir(TEST_CONTENT_DIR);
    nccu::engine::platform::Time::SetFixedStep(1.0f / 60.0f);
    EventBus::Instance().Clear();

    World world("", /*loadSprites=*/false);
    nccu::GameController controller{world, EventBus::Instance()};

    std::string script = preRoute ? preRoute : "";
    script += "interact ";
    script += npcId;
    script += '\n';
    ScriptInput in;
    std::istringstream src(script);
    in.Load(src);
    nccu::engine::input::Input::SetSource(&in);

    Outcome out;
    const Player* p0 = world.GetPlayer();
    out.startX = p0 ? p0->GetPosition().x : 0.0f;

    const World* snapshot = nullptr;     // 第 0 格為 null，與 harness 相同
    for (int f = 0; f < maxFrames; ++f) {
        in.Advance();
        in.ResolvePlan(snapshot);
        controller.Update();
        snapshot = &world;

        const auto& d = world.Dialog();
        if (d.Active() && !out.dialogOpened) {
            out.dialogOpened = true;
            out.npc = std::string(d.NpcId());
            out.openedAtFrame = f;
        }
        if (in.WantsQuit() || (in.HasPlan() && f >= 1 && in.PlanDone()))
            break;
    }
    const Player* p = world.GetPlayer();
    out.endX = p ? p->GetPosition().x : 0.0f;
    out.endY = p ? p->GetPosition().y : 0.0f;

    nccu::engine::input::Input::SetSource(nullptr);
    nccu::engine::platform::Time::SetFixedStep(0.0f);
    EventBus::Instance().Clear();
    return out;
}

}  // namespace

// 核心鎖定點。Ch1 苦主是會擋路的 NPC；`interact victim` 把玩家走進它，移動碰撞箱讓玩家
// 「碰到」苦主貼齊停住（不重疊），動詞必須仍能按 E 使對話開啟。苦主現位於綜合院館
// (1660,1010)；RunInteract 先經缺口繞到他正南方、淨空的 x=1660 列上，故最後的貼齊靠近是
// 沿 Y 軸（往北）、X 對齊。
TEST_CASE("I6: harness `interact victim` opens the NPC dialog (flush-blocked)") {
    World probe("", /*loadSprites=*/false);
    const GameObject* v = FindNpc(probe, "victim");
    REQUIRE(v != nullptr);
    const float vx = v->GetPosition().x;             // {1660,1010}
    const float vy = v->GetPosition().y;

    const Outcome o = RunInteract("victim", 3000);

    // 動詞確實按了 E，遊戲也開啟了苦主對話。
    CHECK(o.dialogOpened);                            // <-- 互動鎖定點
    CHECK(o.npc == "victim");
    CHECK(o.openedAtFrame >= 0);

    // 它以人類走近的方式抵達 NPC：在靠近軸（此處為 Y）上貼齊（非穿過）24x24 移動碰撞箱，
    // 在另一軸（X）對齊。證明按下是來自觸及範圍的幾何，而非傳送到 NPC 上。停留點在苦主
    // 南方，故玩家最終在他下方（endY >= vy）。
    CHECK(o.endY >= vy);                              // 從未穿過
    CHECK(std::fabs(o.endY - vy) <= 24.0f + 8.0f);    // 在觸及範圍內
    // X 對齊到 goto 抵達誤差內（停留跳躍落在 x=1660 列約 2 px 內，原點朝向的驅動再把它
    // 維持在該列）。
    CHECK(std::fabs(o.endX - vx) <= 2.0f);            // Y 軸靠近的列
}

// 配套：繞行靠近 + 朝原點驅動是 (計畫步驟, World 快照) 的純函式，故同一缺口路線 +
// `interact victim` 跑兩次會得到逐位元相同的軌跡（開啟格、最終位置）。
TEST_CASE("I6: `interact` opens dialog deterministically (two runs identical)") {
    const Outcome a = RunInteract("victim", 3000);
    const Outcome b = RunInteract("victim", 3000);

    CHECK(a.dialogOpened);
    CHECK(b.dialogOpened);
    // (計畫步驟, World 快照) 的純函式：逐位元相同的軌跡。
    CHECK(a.dialogOpened   == b.dialogOpened);
    CHECK(a.npc            == b.npc);
    CHECK(a.openedAtFrame  == b.openedAtFrame);
    CHECK(a.endX           == b.endX);
    CHECK(a.endY           == b.endY);
}
