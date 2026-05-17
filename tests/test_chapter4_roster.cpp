#include "doctest/doctest.h"
#include "ChapterSpawns.h"
#include "DialogSource.h"
#include "SemesterState.h"

#include <set>
#include <string>

#ifndef TEST_CONTENT_DIR
#error "TEST_CONTENT_DIR must be defined by the build system"
#endif

using nccu::SemesterState;

// S5e-1: Ch4 期末考終焉 — the 5 archetypes at peak, no new NPCs,
// all isQuestGiver=false (the finale is gate-driven).

TEST_CASE("ChapterNpcSpawns: Ch4 is the 5 chapter4.md archetypes") {
    const auto& ch4 = nccu::ChapterNpcSpawns(SemesterState::Chapter4_Finals);
    REQUIRE(ch4.size() == 5);

    std::set<std::string> ids;
    for (const auto& s : ch4) {
        ids.insert(s.npcId);
        CHECK_FALSE(s.isQuestGiver);                 // gate-driven finale
    }
    CHECK(ids == std::set<std::string>{"victim", "suit_senior",
                                       "bookworm", "ta", "shop_auntie"});
}

TEST_CASE("DialogSource: the 5 archetypes resolve to chapter4.md") {
    nccu::dialog::SetContentDir(TEST_CONTENT_DIR);

    for (const char* id : {"victim", "suit_senior", "bookworm",
                           "ta", "shop_auntie"}) {
        const auto& subs =
            nccu::dialog::Entries(id, SemesterState::Chapter4_Finals);
        REQUIRE_FALSE(subs.empty());                 // section found + parsed

        const nccu::dialog::SubEntry* a = nullptr;
        for (const auto& s : subs) if (s.subState == 0) a = &s;
        REQUIRE(a != nullptr);
        CHECK_FALSE(a->lines.empty());
    }

    // librarian is a Ch2-only npcId — no section in chapter4.md.
    CHECK(nccu::dialog::Entries(
        "librarian", SemesterState::Chapter4_Finals).empty());
}
