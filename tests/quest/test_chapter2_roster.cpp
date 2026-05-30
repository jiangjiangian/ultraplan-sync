/**
 * @file test_chapter2_roster.cpp
 * @brief 驗證 Ch2 名冊為 6 個 NPC（含新角色 librarian），且 librarian 能解析到 chapter2.md。
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

// Ch2 擁有完整名冊，且新加入的 librarian npcId 會經 DialogSource
// 解析到 chapter2.md 的「## NPC：圖書館管理員」段落。

// Ch2 名冊應正好包含 chapter2.md 的 6 個 NPC，且 librarian 為任務給予者。
TEST_CASE("ChapterNpcSpawns：Ch2 是 chapter2.md 的 6 個 NPC") {
    const auto& ch2 = nccu::ChapterNpcSpawns(SemesterState::Chapter2_Midterms);
    REQUIRE(ch2.size() == 6);

    std::set<std::string> ids;
    const nccu::NpcSpawn* librarian = nullptr;
    for (const auto& s : ch2) {
        ids.insert(s.npcId);
        if (std::string_view(s.npcId) == "librarian") librarian = &s;
    }
    for (const char* id : {"victim", "suit_senior", "bookworm",
                           "ta", "shop_auntie", "librarian"})
        CHECK(ids.count(id) == 1);

    // librarian 是任務給予者（chapter2.md 中的純資訊關鍵 NPC）。
    REQUIRE(librarian != nullptr);
    CHECK(librarian->isQuestGiver);
}

// librarian 應能解析到 chapter2.md：有開場 (a)、不給 karma、不設旗標；其他章節則為空。
TEST_CASE("DialogSource：librarian 解析到 chapter2.md 內容") {
    nccu::dialog::SetContentDir(TEST_CONTENT_DIR);

    const auto& subs =
        nccu::dialog::Entries("librarian", SemesterState::Chapter2_Midterms);
    REQUIRE_FALSE(subs.empty());                 // 有找到段落並完成解析

    // (a) 是開場——玩家必須觸發的章節推進關鍵對話。
    const nccu::dialog::SubEntry* a = nullptr;
    for (const auto& s : subs) if (s.subState == 0) a = &s;
    REQUIRE(a != nullptr);
    CHECK_FALSE(a->lines.empty());

    // librarian 是純資訊節點，不給予 karma、不設旗標。
    for (const auto& s : subs) {
        CHECK(s.karmaDelta == 0);
        CHECK(s.setsFlag.empty());
    }

    // librarian 自 Ch2 起才存在：在結局檔（無此段落）查詢時應安全退化為空。
    CHECK(nccu::dialog::Entries("librarian", SemesterState::Ending_A).empty());
}
