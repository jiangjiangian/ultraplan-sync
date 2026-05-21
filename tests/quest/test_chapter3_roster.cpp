#include "doctest/doctest.h"
#include "quest/ChapterSpawns.h"
#include "dialog/DialogSource.h"
#include "state/SemesterState.h"

#include <set>
#include <string>
#include <string_view>

#ifndef TEST_CONTENT_DIR
#error "TEST_CONTENT_DIR must be defined by the build system"
#endif

using nccu::SemesterState;

// S5d-1: Ch3 校慶運動會 has a real roster — the 5 archetypes + the 3
// 物物交換鏈 nodes — and the new npcIds resolve to chapter3.md sections.

TEST_CASE("ChapterNpcSpawns: Ch3 is the 8 chapter3.md NPCs") {
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

    // The 3 物物交換鏈 nodes drive the main quest; the 5 archetypes
    // are ripple / optional (isQuestGiver=false, same stance as Ch2).
    CHECK(questGivers == std::set<std::string>{
        "vendor_sausage_a", "loudspeaker_b", "senior_c"});
}

TEST_CASE("DialogSource: the 3 Ch3 chain npcIds resolve to chapter3.md") {
    nccu::dialog::SetContentDir(TEST_CONTENT_DIR);

    for (const char* id : {"vendor_sausage_a", "loudspeaker_b", "senior_c"}) {
        const auto& subs =
            nccu::dialog::Entries(id, SemesterState::Chapter3_SportsDay);
        REQUIRE_FALSE(subs.empty());                 // section found + parsed

        const nccu::dialog::SubEntry* a = nullptr;
        for (const auto& s : subs) if (s.subState == 0) a = &s;
        REQUIRE(a != nullptr);
        CHECK_FALSE(a->lines.empty());               // (a) opener has lines
    }

    // The 5 archetypes still resolve in Ch3 (shared section names).
    CHECK_FALSE(nccu::dialog::Entries(
        "suit_senior", SemesterState::Chapter3_SportsDay).empty());

    // A Ch3-only npcId degrades to empty in an ending file (no section).
    CHECK(nccu::dialog::Entries(
        "senior_c", SemesterState::Ending_A).empty());
}
