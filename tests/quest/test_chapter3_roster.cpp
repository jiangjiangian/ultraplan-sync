/**
 * @file test_chapter3_roster.cpp
 * @brief 驗證 Ch3 名冊為 8 個 NPC（5 原型＋3 個物物交換鏈節點），新 npcId 能解析到 chapter3.md。
 */
#include "doctest/doctest.h"
#include "game/quest/ChapterSpawns.h"
#include "game/dialog/DialogSource.h"
#include "game/state/SemesterState.h"

#include <set>
#include <string>
#include <string_view>

#ifndef TEST_CONTENT_DIR
#error "TEST_CONTENT_DIR must be defined by the build system"
#endif

using nccu::SemesterState;

// Ch3 校慶運動會擁有完整名冊：5 個原型 NPC 加上 3 個物物交換鏈節點，
// 且新的 npcId 都能解析到 chapter3.md 對應段落。

// Ch3 名冊應為 8 個 NPC，其中只有 3 個物物交換鏈節點是任務給予者。
TEST_CASE("ChapterNpcSpawns：Ch3 是 chapter3.md 的 8 個 NPC") {
    const auto& ch3 = nccu::ChapterNpcSpawns(SemesterState::Chapter3_SportsDay);
    REQUIRE(ch3.size() == 8);

    std::set<std::string> ids;
    std::set<std::string> questGivers;
    for (const auto& s : ch3) {
        ids.insert(s.npcId);
        if (s.isQuestGiver) questGivers.insert(s.npcId);
    }
    for (const char* id : {"victim", "suit_senior", "bookworm", "ta",
                           "shop_auntie", "vendor_sausage_a",
                           "loudspeaker_b", "senior_c"})
        CHECK(ids.count(id) == 1);

    // 3 個物物交換鏈節點推進主線任務；5 個原型 NPC 屬於漣漪／選擇性內容
    // （isQuestGiver=false，與 Ch2 的立場一致）。
    CHECK(questGivers == std::set<std::string>{
        "vendor_sausage_a", "loudspeaker_b", "senior_c"});
}

// 3 個交換鏈 npcId 都應能解析到 chapter3.md 並具備開場 (a) 對話。
TEST_CASE("DialogSource：3 個 Ch3 交換鏈 npcId 解析到 chapter3.md") {
    nccu::dialog::SetContentDir(TEST_CONTENT_DIR);

    for (const char* id : {"vendor_sausage_a", "loudspeaker_b", "senior_c"}) {
        const auto& subs =
            nccu::dialog::Entries(id, SemesterState::Chapter3_SportsDay);
        REQUIRE_FALSE(subs.empty());                 // 有找到段落並完成解析

        const nccu::dialog::SubEntry* a = nullptr;
        for (const auto& s : subs) if (s.subState == 0) a = &s;
        REQUIRE(a != nullptr);
        CHECK_FALSE(a->lines.empty());               // (a) 開場有對白
    }

    // 5 個原型 NPC 在 Ch3 仍能解析（段落名稱共用）。
    CHECK_FALSE(nccu::dialog::Entries(
        "suit_senior", SemesterState::Chapter3_SportsDay).empty());

    // Ch3 專屬的 npcId 在結局檔（無對應段落）應退化為空。
    CHECK(nccu::dialog::Entries(
        "senior_c", SemesterState::Ending_A).empty());
}
