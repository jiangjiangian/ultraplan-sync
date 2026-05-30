/**
 * @file test_ch1_spine_reachable.cpp
 * @brief 在實際出貨的碰撞地圖上驗證 Ch1 最小互惠主線可達 Ch2，並確認該路線在兩次執行間具重播確定性。
 */
#include "doctest/doctest.h"
#include "game/quest/Flags.h"
#include "engine/platform/ScriptInput.h"
#include "game/controller/GameController.h"
#include "engine/events/EventBus.h"
#include "game/world/World.h"
#include "game/entities/Player.h"
#include "game/dialog/DialogState.h"
#include "game/dialog/DialogSource.h"
#include "game/quest/ChapterVendors.h"
#include "game/state/SemesterState.h"
#include "game/world/CollisionMask.h"
#include "game/quest/NpcSpawns.h"
#include "engine/math/Rect.h"
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

// 此測試確保在實際出貨的 collision_mask.png 上，Ch1→Ch2 的進度仍然可達。
//
// 問題背景：collision_mask.png 在整個南側校園 y≈1761–1819 烘焙了一道連續的
// 東西向牆，其唯一的縱向缺口在 x≈880–1042 這一行。先前所有的 `goto` 路線都
// 直接往上撞進有牆的縱列（x=320 / 560 / 750 / 1140 / 1180 / 1500 / 1560 /
// 1706），玩家因此貼牆停在 y≈1821 而無法前進——Ch1 卡死、抵達不了任何結局，
// 且每次執行都一樣。`goto` 依設計是純軸向驅動（不是尋路器）；過時的是路線
// 而非引擎。校園本身確實可通行（見 test_spawn_reachability 的洪水填充），
// 只是必須改走那個缺口。
//
// 本測試釘住一條經地圖與 NPC 驗證過、穩健的最小 Ch1 互惠主線路線，依序：
//   1. 在綜合院館找苦主對話（Flag_PromisedVictim），
//   2. 在集英樓對峙西裝學長並做出選擇（Flag_SuitSeniorChoiceMade）——這才會
//      讓雨傘出現（選擇前撿取物根本不存在於世界中，主線無法跳過），
//   3. 在集英樓附近找到他的透明傘（Flag_HasVictimUmbrella），
//   4. 把傘帶回給他（授予：TryReturnVictimUmbrella 設下 Flag_HasTrueUmbrella
//      並發布 UmbrellaClaimed → Ch1 清關 → 經事件接線進入幕間市集），
//   5. 從南側離開幕間市集（→ Chapter2_Midterms）。
// 章節是在「歸還」雨傘時清關，而非在地上撿到一把傘時。所有路段都走 x≈1041 的
// 缺口／牆北走廊／淨空的 x=1660 縱列。它驅動真正的 ScriptInput + GameController
// 接縫（與 Harness 的執行順序一致）。若日後地圖修改重新封住缺口、路線退化、
// 或硬性關卡走進死路，學期就到不了 Ch2，此測試便會失敗。

namespace {

struct SpineResult {
    nccu::SemesterState semester;
    bool  promisedVictim;
    bool  hasTrueUmbrella;
    int   karma;
    int   frames;
};

SpineResult RunSpine(const std::string& script, int maxFrames) {
    nccu::dialog::SetContentDir(TEST_CONTENT_DIR);
    nccu::SetVendorContentDir(TEST_CONTENT_DIR);
    nccu::ReloadVendors();
    nccu::engine::platform::Time::SetFixedStep(1.0f / 60.0f);

    World world("", /*loadSprites=*/false);
    nccu::GameController controller{world, EventBus::Instance()};

    ScriptInput in;
    std::istringstream src(script);
    in.Load(src);
    nccu::engine::input::Input::SetSource(&in);

    const World* snap = nullptr;
    int f = 0;
    for (; f < maxFrames; ++f) {
        in.Advance();
        in.ResolvePlan(snap);
        controller.Update();
        snap = &world;
        if (in.WantsQuit() || (in.HasPlan() && f >= 1 && in.PlanDone()))
            break;
    }
    nccu::engine::input::Input::SetSource(nullptr);
    nccu::engine::platform::Time::SetFixedStep(0.0f);

    const Player* p = world.GetPlayer();
    return SpineResult{
        world.Semester().Current(),
        p && p->HasFlag(nccu::kFlagPromisedVictim),
        p && p->HasFlag(nccu::kFlagHasTrueUmbrella),
        p ? p->GetKarma() : -999,
        f};
}

// 經驗證的最小 Ch1 互惠主線路線。每個路段都對照真正的 CollisionMask 與
// DefaultNpcSpawns 碰撞框追蹤過（參見下方套件內的「牆縫」健全性案例），
// 並端到端跑過 harness。
const char* kSpineScript =
    // (1) 出生點 → 穿過缺口到綜合院館 (1660,1010) 的苦主，然後承諾。
    "goto 1040 1712\n" "goto 1048 1704\n" "goto 1264 1632\n"
    "goto 1280 1624\n" "goto 1296 1616\n" "goto 1312 1608\n"
    "goto 1328 1600\n" "goto 1408 1512\n" "goto 1416 1504\n"
    "goto 1424 1496\n" "goto 1432 1488\n" "goto 1448 1480\n"
    "goto 1496 1456\n" "goto 1504 1448\n" "goto 1512 1440\n"
    "goto 1520 1432\n" "goto 1528 1424\n" "goto 1544 1328\n"
    "goto 1552 1320\n" "goto 1568 1312\n" "goto 1584 1304\n"
    "goto 1592 1296\n" "goto 1660 1120\n"
    "interact victim\n"               // → Flag_PromisedVictim（+5 karma）
    "choose 0\n"
    "advance\nadvance\nadvance\nadvance\nadvance\nadvance\n"
    // (2) 硬性關卡：在集英樓 (1620,1560) 對峙西裝學長。集英樓矩形
    //     (1524,1353,224x192) 的底部在 y≈1545 形成牆，而學長就站在它南側——
    //     因此在 y≈1545 直接往西走會貼牆卡住。要先沿淨空的東側走廊往下到
    //     y≈1620（建築下方），再往西走向他。`interact suit_senior` 完成貼近
    //     後再按 E。選擇 (d) 善意提醒（+5，Flag_HelpedSenior）——這會設下
    //     Flag_SuitSeniorChoiceMade，使苦主的傘出現（在此之前它不存在於任何地方）。
    //     選單把子狀態 ≥1 由小到大排列（b→0, c→1, d→2），所以 `choose 2` 選的是 (d)。
    "goto 1744 1168\n" "goto 1752 1620\n" "goto 1620 1610\n"
    "interact suit_senior\n"          // → Flag_SuitSeniorChoiceMade（+5）
    "choose 2\n"
    "advance\nadvance\nadvance\nadvance\nadvance\n"
    "wait 5\n"                        // → 觸發 MaybeSpawnChapter1VictimUmbrella
    // (3) → 集英樓南側 (1700,1610) 已生成的苦主之傘，撿起它。學長的接近過程把
    //     玩家留在建築南側，因此往東一小步即可到撿取點。
    "goto 1700 1610\n"
    "interact victimumb 1700 1610\n"  // → Flag_HasVictimUmbrella
    "wait 10\n"
    // (4) 沿東側走廊把傘帶回給苦主；授予（靜默）並開啟 (d) 重逢致謝的交換對話。
    //     章節清關會延後到該對話關閉之後，所以必須先讀完（advance），
    //     LiftChapter1Clear 才會發布 UmbrellaClaimed → 幕間市集。
    "goto 1752 1480\n" "goto 1744 1344\n" "goto 1728 1336\n"
    "goto 1704 1328\n" "goto 1688 1320\n" "goto 1672 1312\n"
    "goto 1660 1120\n"
    "interact victim\n"               // 授予（旗標）並開啟 (d) 交換對話
    "advance\nadvance\nadvance\nadvance\nadvance\n"  // 讀完並關閉 (d) 這段
    "wait 20\n"                       // → 觸發 LiftChapter1Clear → 幕間市集
    // (5) 離開幕間市集（進場會重定位到 {500,1500}）→ Chapter2。
    "goto 380 1750\n"                 // 幕間進場點是 {500,1500}；先回到缺口
    "goto 1041 1750\n"                // （x=500 縱列有牆），再
    "goto 1041 1965\n"                // 往下穿過缺口進入幕間出口區
    "wait 40\n";                      // → Chapter2_Midterms

}  // namespace

// 健全性檢查：直接從實際地圖資產證明南牆只有 x≈880-1042 這個缺口，給上面的路線一個前提依據。
TEST_CASE("出貨遮罩的南牆恰好只有 x≈880-1042 這個缺口") {
    // 用實際資產證明問題的幾何，讓路線的前提不會悄悄腐壞。若資產不存在
    //（全新 clone）則優雅降級——此時一切皆可通行、主線自然成立，故跳過幾何斷言。
    const nccu::CollisionMask m = nccu::LoadTerrainMask();
    if (m.Empty()) {
        MESSAGE("terrain mask absent — gap-geometry check skipped");
        return;
    }
    constexpr float B = 24.0f;
    // 舊路線使用的有牆縱列：從路面往上會被擋住。
    for (float x : {320.f, 560.f, 750.f, 1140.f, 1500.f, 1560.f, 1706.f}) {
        bool blockedUp = false;
        for (float y = 1858.f; y >= 1300.f; y -= 2.f)
            if (m.BlockedBox(x, y, B, B)) { blockedUp = true; break; }
        INFO("old-route column x=" << x << " should be wall-blocked");
        CHECK(blockedUp);
    }
    // 缺口縱列 x=1000 由上到下都淨空（路線的前提）。
    bool gapClear = true;
    for (float y = 1900.f; y >= 1280.f; y -= 2.f)
        if (m.BlockedBox(1000.f, y, B, B)) { gapClear = false; break; }
    CHECK(gapClear);
    // 牆北走廊 y=1750 從缺口（x=1041）往西到 x=380 攀升縱列為止，
    // 既沒有地圖遮擋、也沒有任何 Ch1 NPC。
    std::vector<nccu::engine::math::Rect> npc;
    for (const auto& n : nccu::DefaultNpcSpawns())
        npc.push_back({n.pos.x, n.pos.y, B, B});
    bool corridorClear = true;
    for (float x = 1041.f; x >= 380.f; x -= 2.f) {
        nccu::engine::math::Rect a{x, 1750.f, B, B};
        bool hit = m.BlockedBox(x, 1750.f, B, B);
        for (const auto& r : npc) if (a.Intersects(r)) hit = true;
        if (hit) { corridorClear = false; break; }
    }
    CHECK(corridorClear);
}

// 最小 Ch1 主線在實際地圖上應一路推進到 Chapter 2；若路線被擋住、學期停在 Ch1，本案就會失敗。
TEST_CASE("最小 Ch1 主線在出貨遮罩上可抵達 Chapter 2") {
    const SpineResult r = RunSpine(kSpineScript, 9000);

    // 取傘閘門已觸發（已和苦主對話）。
    CHECK(r.promisedVictim);
    // 進入 Chapter2 時會清掉持有的傘與 Flag_HasTrueUmbrella（「傘又掉了」的卡片
    // 在機制上成立——每章都從沒有傘開始）。因此這條主線在其結尾（位於 Ch2）取樣時，
    // 不應仍帶著該旗標。Ch1 的授予確實有觸發（TryReturnVictimUmbrella →
    // Flag_HasTrueUmbrella + UmbrellaClaimed），由「主線竟能抵達 Ch2」這件事本身
    // 在此證明——這條路線只透過該授予清關 Ch1（全程不碰道德傘）——而授予本身
    // 在 Ch1 內由 test_scriptinput_plan 的 drive+E 案例直接釘住。
    CHECK_FALSE(r.hasTrueUmbrella);
    // 苦主 (b) 承諾 +5（在 karma 50 的起點上），再加西裝學長 (d) 善意提醒 +5
    //（主線現在會經過學長的選擇）= karma 60。
    CHECK(r.karma == 60);
    // 主線推進了 Ch1 → 幕間市集 → Chapter2_Midterms。這就是路線被擋時會失敗的
    // 斷言（玩家在 Ch1 卡死、學期不前進）：用舊的有牆 `goto 750 …` 路線時，
    // 學期會停在 Chapter1_AddDrop。
    CHECK(r.semester == nccu::SemesterState::Chapter2_Midterms);
    // 有上限——卡死的執行會耗盡全部 9000 幀；驗證過的路線遠在此之下即完成。
    CHECK(r.frames < 9000);
}

// 重播確定性：解析器是 (計畫步驟, World 快照) 的純函式，因此同一腳本跑兩次必須得到逐位元相同的結尾狀態。
TEST_CASE("Ch1 主線路線在兩次執行間具確定性") {
    const SpineResult a = RunSpine(kSpineScript, 9000);
    const SpineResult b = RunSpine(kSpineScript, 9000);
    CHECK(a.semester == b.semester);
    CHECK(a.promisedVictim == b.promisedVictim);
    CHECK(a.hasTrueUmbrella == b.hasTrueUmbrella);
    CHECK(a.karma == b.karma);
    CHECK(a.frames == b.frames);
    // 並且它確實抵達了 Ch2（避免「確定性地卡住」也讓相等檢查輕易通過）。
    CHECK(a.semester == nccu::SemesterState::Chapter2_Midterms);
}
