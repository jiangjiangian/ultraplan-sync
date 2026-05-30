/**
 * @file test_spawn_reachability.cpp
 * @brief 載入實際碰撞地圖，以洪水填充驗證每個任務關鍵生成點都不嵌牆且從玩家出生點可達；無資產時優雅跳過。
 */
#include "doctest/doctest.h"
#include "game/world/CollisionMask.h"
#include "game/quest/NpcSpawns.h"
#include "game/quest/ChapterSpawns.h"
#include "game/quest/ChapterQuestItems.h"
#include "game/quest/ChapterPickups.h"
#include "game/quest/ChapterVendors.h"
#include "game/quest/Chapter3Quest.h"
#include "game/state/SemesterState.h"
#include <array>
#include <queue>
#include <string>
#include <vector>

#ifndef TEST_CONTENT_DIR
#error "TEST_CONTENT_DIR must be defined by the build system"
#endif

// 碰撞地圖整合的縱深防禦。若烘焙出的地形遮罩封住了校園邊界，或某個生成座標
// 落在碰撞層的物件上，遊戲會悄悄變得無法遊玩，而其他測試卻全綠（曾發生過的問題：
// 南側邊界牆唯一的缺口在正門美術約 100 px 之東，且學霸坐在一塊花圃多邊形上）。
// 本測試載入實際出貨的遮罩，並以從玩家出生點出發的洪水填充，斷言每個任務關鍵
// 實體都確實可通行且可達。
//
// 它會優雅降級：全新 checkout 沒有未追蹤的 resources/ 資產時會得到空遮罩
//（一切皆可通行）——此時守門選擇跳過而非失敗，因為它只能斷言它載入得到的東西。

using nccu::CollisionMask;

namespace {

constexpr float kBox = 24.0f;  // 對應 world::kPlayerWidth/Height

struct Spot { const char* name; float x; float y; };

// 玩家／雨傘／任務撿取物的生成座標必須與 src/World.cpp 建構子中的字面值一致——
// 若那些座標移動，要同步更新此清單。原型 NPC 與路人行人是從 DefaultNpcSpawns() /
// AmbientStudentSpawns() 即時取得，因此未來移動生成點可零重複地涵蓋。納入路人行人
// 是因為一個一「開始」就嵌在牆裡的遊走者，在遊戲中看起來就像「有人卡在牆裡」，
// 即使它從不擋住進度。
std::vector<Spot> GameplaySpots() {
    std::vector<Spot> s = {
        {"player",            500.0f, 1860.0f},
        {"TrueUmbrella",      320.0f, 1280.0f},
        {"FragileUmbrella",   750.0f, 1280.0f},
        {"ProfTrapUmbrella", 1200.0f, 1256.0f},
        {"CursedUmbrella",   1560.0f, 1280.0f},
        {"QuestForm",         560.0f, 1725.0f},
    };
    for (const auto& n : nccu::DefaultNpcSpawns())
        s.push_back(Spot{n.npcId, n.pos.x, n.pos.y});
    for (const auto& n : nccu::AmbientStudentSpawns())
        s.push_back(Spot{n.spritePath, n.pos.x, n.pos.y});

    // 各章的生成座標——這些先前從未做過可達性驗證（試玩時發現物件落在牆裡／樹上）。
    // 涵蓋每個章節實際會生成的座標：各章 NPC 名冊（新角色 librarian／香腸／
    // 大聲公／學姊，以及重新放置的原型）、Ch2 散落筆記、解析出的幕間攤位，加上
    // 寫死的 Ch2 自販機／Ch4 集英樓攤商，以及 Ch3/Ch4 道具箱的 TrueUmbrella。
    using nccu::SemesterState;
    for (auto st : {SemesterState::Chapter2_Midterms,
                    SemesterState::Chapter3_SportsDay,
                    SemesterState::Chapter4_Finals}) {
        for (const auto& n : nccu::ChapterNpcSpawns(st))
            s.push_back(Spot{n.npcId, n.pos.x, n.pos.y});
    }
    for (const auto& q : nccu::ChapterQuestItems(SemesterState::Chapter2_Midterms))
        s.push_back(Spot{"Ch2note", q.pos.x, q.pos.y});
    for (const auto& pk : nccu::ChapterPickups(SemesterState::Chapter1_AddDrop))
        s.push_back(Spot{"Ch1cash", pk.pos.x, pk.pos.y});

    nccu::SetVendorContentDir(TEST_CONTENT_DIR);
    nccu::ReloadVendors();
    for (auto st : {SemesterState::Interlude_Market,
                    SemesterState::Chapter2_Midterms,
                    SemesterState::Chapter4_Finals}) {
        for (const auto& v : nccu::ChapterVendors(st))
            s.push_back(Spot{"vendor", v.pos.x, v.pos.y});
    }
    // Ch4 藏在體育館後方的 TrueUmbrella（進場生成，刻意保留在體育館範圍內作為彩蛋——
    // 它只需可通行且可達，遮擋是刻意的）。
    s.push_back(Spot{"Ch4 體育館後台 TrueUmbrella", 1640.0f, 375.0f});
    // Ch3 給線索後才現身的 TrueUmbrella，重新放到體育館左側以免被遮擋
    //（World::MaybeSpawnChapter3Umbrella 在 Flag_KnowsUmbrellaLoc 後生成於
    // kChapter3UmbrellaPos）。仍必須從玩家出生點可通行且可達。
    s.push_back(Spot{"Ch3 TrueUmbrella (left of gym)",
                     nccu::kChapter3UmbrellaPos.x,
                     nccu::kChapter3UmbrellaPos.y});
    return s;
}

} // namespace

// 每個遊戲生成點都必須可通行，且從玩家出生點以洪水填充可達（否則代表校園被封死或物件嵌牆）。
TEST_CASE("每個遊戲生成點都可通行且從玩家出生點可達") {
    const CollisionMask mask = nccu::LoadTerrainMask();
    if (mask.Empty()) {
        MESSAGE("terrain mask asset absent (untracked resources/) — "
                "reachability guard skipped");
        return;
    }

    const auto spots = GameplaySpots();

    // 1. 任何實體都不得生成在實心地形裡。
    for (const auto& s : spots) {
        INFO("spawn '" << std::string(s.name) << "' at (" << s.x << ", "
             << s.y << ") is inside solid terrain");
        CHECK_FALSE(mask.BlockedBox(s.x, s.y, kBox, kBox));
    }

    // 2. 以 8 px 網格從玩家出生點對可通行空間做洪水填充；其餘每個生成點都必須在其一格範圍內。
    constexpr int kStep = 8;
    const int gw = mask.Width()  / kStep;
    const int gh = mask.Height() / kStep;
    auto freeCell = [&](int cx, int cy) {
        const float x = static_cast<float>(cx * kStep);
        const float y = static_cast<float>(cy * kStep);
        if (cx < 0 || cy < 0 || cx >= gw || cy >= gh) return false;
        return !mask.BlockedBox(x, y, kBox, kBox);
    };

    const int sx = 500 / kStep, sy = 1860 / kStep;
    std::vector<char> seen(static_cast<std::size_t>(gw) * gh, 0);
    std::queue<std::pair<int,int>> q;
    if (freeCell(sx, sy)) {
        seen[static_cast<std::size_t>(sy) * gw + sx] = 1;
        q.push({sx, sy});
    }
    const std::array<std::pair<int,int>,4> dirs = {{{1,0},{-1,0},{0,1},{0,-1}}};
    while (!q.empty()) {
        auto [cx, cy] = q.front(); q.pop();
        for (auto [dx, dy] : dirs) {
            const int nx = cx + dx, ny = cy + dy;
            if (nx < 0 || ny < 0 || nx >= gw || ny >= gh) continue;
            auto& cell = seen[static_cast<std::size_t>(ny) * gw + nx];
            if (cell || !freeCell(nx, ny)) continue;
            cell = 1;
            q.push({nx, ny});
        }
    }

    auto reachable = [&](float fx, float fy) {
        const int bx = static_cast<int>(fx) / kStep;
        const int by = static_cast<int>(fy) / kStep;
        for (int dx = -3; dx <= 3; ++dx)
            for (int dy = -3; dy <= 3; ++dy) {
                const int cx = bx + dx, cy = by + dy;
                if (cx >= 0 && cy >= 0 && cx < gw && cy < gh &&
                    seen[static_cast<std::size_t>(cy) * gw + cx])
                    return true;
            }
        return false;
    };

    for (const auto& s : spots) {
        INFO("spawn '" << std::string(s.name) << "' at (" << s.x << ", "
             << s.y << ") is unreachable from the player spawn (campus sealed?)");
        CHECK(reachable(s.x, s.y));
    }
}
