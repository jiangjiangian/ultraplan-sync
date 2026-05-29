/**
 * @file test_chapter_spawns.cpp
 * @brief 驗證 Ch1 名冊資料與生成、名冊抽換時保住 Player 不變量、往返切換的復原，以及重複 respawn 不重複/不洩漏。
 */
#include "doctest/doctest.h"
#include "game/quest/ChapterSpawns.h"
#include "game/quest/ChapterQuestItems.h"
#include "game/entities/CashPickup.h"
#include "game/world/World.h"
#include "game/entities/Player.h"
#include "engine/core/GameObject.h"
#include <algorithm>
#include <set>
#include <string>
#include <string_view>
#include <vector>

using nccu::ChapterNpcSpawns;
using nccu::SemesterState;
using nccu::World;

namespace {

// 活動物件名冊中存在的 npcId（NPC 覆寫了 NpcId()）。
std::set<std::string> RosterNpcIds(const World& w) {
    std::set<std::string> ids;
    for (const auto& o : w.Objects()) {
        const std::string_view id = o->NpcId();
        if (!id.empty()) ids.insert(std::string(id));
    }
    return ids;
}

bool HasNpc(const World& w, const char* id) {
    return RosterNpcIds(w).count(id) != 0;
}

// Ch1 的 5 個主線／漣漪原型——即 ChapterNpcSpawns(Ch1) 資料表宣告的 npcId。
// 另外有一批 Ch1 人潮（遊走、空 npcId）與 3 個靜態風味 NPC（ch1_flavor_* npcId）
// 是經由 World::SpawnChapterNpcs 的程式碼區塊加入，而非資料表，因此活動的 npcId
// 名冊現在是 5 個原型再加上 3 個風味 id。這些輔助函式讓主線斷言保持精確
//（5 個原型不變），同時容忍風味 id。
const std::set<std::string>& Ch1Archetypes() {
    static const std::set<std::string> kIds = {
        "victim", "suit_senior", "bookworm", "ta", "shop_auntie"};
    return kIds;
}

// 只計算主線原型的活動數量（排除風味 NPC）。
std::size_t ArchetypeNpcCount(const World& w) {
    std::size_t n = 0;
    for (const auto& id : RosterNpcIds(w))
        if (Ch1Archetypes().count(id)) ++n;
    return n;
}

// CashPickup 也是章節名冊成員（各章金幣，會像 NPC 一樣在狀態改變時被清除），
// 因此「跨切換後存活」的計數也必須把它們排除。
std::size_t RosterCashCount(const World& w) {
    std::size_t n = 0;
    for (const auto& o : w.Objects())
        if (dynamic_cast<const CashPickup*>(o.get())) ++n;
    return n;
}

} // namespace

// Ch1 資料表為 5 個原型，且與舊版 DefaultNpcSpawns() 逐欄相同；幕間市集與三個結局都沒有 NPC 名冊。
TEST_CASE("ChapterNpcSpawns: Ch1 is the 5 archetypes; Interlude+endings empty") {
    const auto& ch1 = ChapterNpcSpawns(SemesterState::Chapter1_AddDrop);
    std::set<std::string> ids;
    for (const auto& s : ch1) ids.insert(s.npcId);
    CHECK(ids == std::set<std::string>{"victim", "suit_senior", "bookworm",
                                       "ta", "shop_auntie"});
    CHECK(ch1.size() == 5);

    // Ch1 必須與舊版 DefaultNpcSpawns() 逐位元相同。
    const auto& legacy = nccu::DefaultNpcSpawns();
    REQUIRE(ch1.size() == legacy.size());
    for (std::size_t i = 0; i < ch1.size(); ++i) {
        CHECK(std::string(ch1[i].npcId) == std::string(legacy[i].npcId));
        CHECK(ch1[i].pos.x == legacy[i].pos.x);
        CHECK(ch1[i].pos.y == legacy[i].pos.y);
        CHECK(ch1[i].isQuestGiver == legacy[i].isQuestGiver);
    }

    // 全程都沒有 NPC 名冊的狀態（穩定、不會隨各章陸續填入而變動）：幕間市集
    // 沒有 NPC（它的角色是經 ChapterVendors 的攤商，不是 NPC；出口是一個觸發區），
    // 而三個結局只渲染一張卡片、沒有名冊。Ch2/Ch3/Ch4 依設計會被填入——它們的
    // 名冊由各自的測試（test_chapter2_roster 等）釘住，而不是在此用一句「後續狀態為空」概括。
    CHECK(ChapterNpcSpawns(SemesterState::Interlude_Market).empty());
    CHECK(ChapterNpcSpawns(SemesterState::Ending_A).empty());
    CHECK(ChapterNpcSpawns(SemesterState::Ending_B).empty());
    CHECK(ChapterNpcSpawns(SemesterState::Ending_D).empty());
    CHECK(ChapterNpcSpawns(SemesterState::Ending_C).empty());
}

// World 建構子會生成 Ch1 名冊，且 Player 維持在物件清單最前端。
TEST_CASE("World ctor spawns the Ch1 roster, Player stays at front") {
    World w("", /*loadSprites=*/false);

    REQUIRE_FALSE(w.Objects().empty());
    Player* player = w.GetPlayer();
    REQUIRE(player != nullptr);
    CHECK(static_cast<GameObject*>(player) == w.Objects().front().get());

    for (const char* id : {"victim", "suit_senior", "bookworm",
                           "ta", "shop_auntie"})
        CHECK(HasNpc(w, id));
}

// RespawnChapterRoster 會抽換 NPC 名冊，但保住 Player 不變量；往返切換後完整名冊復原、非名冊物件不受影響。
TEST_CASE("RespawnChapterRoster swaps NPCs but preserves the Player invariant") {
    World w("", /*loadSprites=*/false);

    // 在任何抽換之前，先記下身分與「非章節物件」的數量。
    Player*     beforePlayer = w.GetPlayer();
    GameObject* beforeFront  = w.Objects().front().get();
    REQUIRE(beforePlayer != nullptr);
    REQUIRE(static_cast<GameObject*>(beforePlayer) == beforeFront);

    const std::size_t totalCh1   = w.Objects().size();
    const std::size_t ch1Cash    = RosterCashCount(w);
    // 5 個主線原型在進場時就在場（人潮與風味 NPC 是附加的——以下用活動名冊計算，不在此計入）。
    REQUIRE(ArchetypeNpcCount(w) == 5);
    REQUIRE(ch1Cash == 5);                              // Ch1 金幣分佈
    REQUIRE(nccu::ChapterQuestItems(                    // 表宣告 1 個……
                SemesterState::Chapter1_AddDrop).size() == 1);

    // --- 切換到一個名冊為空的狀態。---
    // 選 Ending_A 作為穩定的對照：結局只渲染一張卡片，依設計沒有 NPC 名冊／
    // 沒有攤商／沒有金幣——它全程維持為空，因此這個「抽換機制 + Player 不變量」
    // 的測試不會因為其他章節陸續填入而需要反覆改寫。
    w.RespawnChapterRoster(SemesterState::Ending_A);

    // Ch1 的 NPC 都消失了（5 個原型與 3 個風味 NPC——每個章節名冊成員都被清除）；
    // 不殘留任何章節 NPC id。
    CHECK(RosterNpcIds(w).empty());
    for (const char* id : {"victim", "suit_senior", "bookworm",
                           "ta", "shop_auntie"})
        CHECK_FALSE(HasNpc(w, id));

    // Player 身分與「最前端」不變量在抽換後完好。
    CHECK(w.GetPlayer() == beforePlayer);
    CHECK(w.Objects().front().get() == beforeFront);
    CHECK(static_cast<GameObject*>(w.GetPlayer()) == w.Objects().front().get());

    // 所有非章節名冊成員仍在場（玩家 + 3 把道德傘 + 建構子的申請書 QuestFlagPickup
    // + 路人學生——這些都不受名冊追蹤）。Ch1 人潮與風味 NPC 受名冊追蹤，因此也被清除；
    // 這裡直接取抽換後的存活數量，而不去推導它。
    const std::size_t nonChapter = w.Objects().size();
    CHECK(nonChapter < totalCh1);                        // 名冊已被清除

    // --- 往返回到 Ch1：完整名冊復原。---
    w.RespawnChapterRoster(SemesterState::Chapter1_AddDrop);

    for (const char* id : {"victim", "suit_senior", "bookworm",
                           "ta", "shop_auntie"})
        CHECK(HasNpc(w, id));
    CHECK(ArchetypeNpcCount(w) == 5);                    // 主線不變
    CHECK(w.Objects().size() == totalCh1);               // 人潮與風味也復原

    // 往返之後不變量仍成立。
    CHECK(w.GetPlayer() == beforePlayer);
    CHECK(w.Objects().front().get() == beforeFront);
}

// 對同一狀態重複呼叫 respawn 不會重複生成或洩漏 NPC。
TEST_CASE("Repeated same-state respawn does not duplicate or leak NPCs") {
    World w("", /*loadSprites=*/false);
    const std::size_t total = w.Objects().size();

    w.RespawnChapterRoster(SemesterState::Chapter1_AddDrop);
    w.RespawnChapterRoster(SemesterState::Chapter1_AddDrop);

    CHECK(w.Objects().size() == total);
    CHECK(ArchetypeNpcCount(w) == 5);                    // 主線：不重複、不洩漏
    CHECK(static_cast<GameObject*>(w.GetPlayer()) == w.Objects().front().get());
}
