/**
 * @file test_chapter4_roster.cpp
 * @brief 驗證 Ch4 名冊正好是 5 個原型 NPC、皆非任務給予者，且都能解析到 chapter4.md。
 */
#include "doctest/doctest.h"
#include "game/quest/ChapterSpawns.h"
#include "game/dialog/DialogSource.h"
#include "game/state/SemesterState.h"

#include <set>
#include <string>

#ifndef TEST_CONTENT_DIR
#error "TEST_CONTENT_DIR must be defined by the build system"
#endif

using nccu::SemesterState;

// Ch4 期末考終焉的名冊：5 個原型 NPC 同時登場、沒有新增 NPC，
// 且全部 isQuestGiver=false（結局由閘門條件驅動，而非任務指示）。

// Ch4 名冊應正好是 chapter4.md 的 5 個原型，且皆非任務給予者。
TEST_CASE("ChapterNpcSpawns: Ch4 is the 5 chapter4.md archetypes") {
    const auto& ch4 = nccu::ChapterNpcSpawns(SemesterState::Chapter4_Finals);
    REQUIRE(ch4.size() == 5);

    std::set<std::string> ids;
    for (const auto& s : ch4) {
        ids.insert(s.npcId);
        CHECK_FALSE(s.isQuestGiver);                 // 結局由閘門驅動，非任務給予者
    }
    CHECK(ids == std::set<std::string>{"victim", "suit_senior",
                                       "bookworm", "ta", "shop_auntie"});
}

// 5 個原型 NPC 都應能在 chapter4.md 找到對應段落並解析出開場 (a)。
TEST_CASE("DialogSource: the 5 archetypes resolve to chapter4.md") {
    nccu::dialog::SetContentDir(TEST_CONTENT_DIR);

    for (const char* id : {"victim", "suit_senior", "bookworm",
                           "ta", "shop_auntie"}) {
        const auto& subs =
            nccu::dialog::Entries(id, SemesterState::Chapter4_Finals);
        REQUIRE_FALSE(subs.empty());                 // 有找到段落並完成解析

        const nccu::dialog::SubEntry* a = nullptr;
        for (const auto& s : subs) if (s.subState == 0) a = &s;
        REQUIRE(a != nullptr);
        CHECK_FALSE(a->lines.empty());
    }

    // librarian 只存在於 Ch2，chapter4.md 沒有它的段落，應解析為空。
    CHECK(nccu::dialog::Entries(
        "librarian", SemesterState::Chapter4_Finals).empty());
}
