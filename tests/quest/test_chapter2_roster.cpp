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

// S5c-1: Ch2 has a real roster and the new librarian npcId resolves to
// the chapter2.md「## NPC：圖書館管理員」section through DialogSource.

TEST_CASE("ChapterNpcSpawns: Ch2 is the 6 chapter2.md NPCs") {
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

    // The librarian is the quest-giver (chapter2.md: 純資訊關鍵 NPC).
    REQUIRE(librarian != nullptr);
    CHECK(librarian->isQuestGiver);
}

TEST_CASE("DialogSource: librarian resolves to chapter2.md content") {
    nccu::dialog::SetContentDir(TEST_CONTENT_DIR);

    const auto& subs =
        nccu::dialog::Entries("librarian", SemesterState::Chapter2_Midterms);
    REQUIRE_FALSE(subs.empty());                 // section found + parsed

    // (a) is the opener — the章節推進關鍵對話 the player must trigger.
    const nccu::dialog::SubEntry* a = nullptr;
    for (const auto& s : subs) if (s.subState == 0) a = &s;
    REQUIRE(a != nullptr);
    CHECK_FALSE(a->lines.empty());

    // The librarian gives no karma (純資訊節點).
    for (const auto& s : subs) {
        CHECK(s.karmaDelta == 0);
        CHECK(s.setsFlag.empty());
    }

    // Unknown-in-Ch1 is fine: librarian only exists from Ch2 on; asking
    // for it in an ending file (no such section) degrades to empty.
    CHECK(nccu::dialog::Entries("librarian", SemesterState::Ending_A).empty());
}
