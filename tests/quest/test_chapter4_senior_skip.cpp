#include "doctest/doctest.h"
#include "quest/ChapterSpawns.h"
#include "entities/GameObject.h"
#include "entities/NPC.h"
#include "entities/Player.h"
#include "state/SemesterState.h"
#include "world/World.h"

#include <string>
#include <string_view>

using nccu::SemesterState;
using nccu::World;

// M7 (cycle9c): chapter4.md L6 promises that a player who scolded the
// 西裝學長 in Ch1 (Flag_ScoldedSenior) will not see him again at the
// finals — UNLESS the Ch2 callback (Flag_HelpedSenior) repaired the
// relationship, in which case he is back. Before this fix the suit_senior
// always spawned in the Ch4 roster (ChapterSpawns.h annotated this as
// "KNOWN OMISSION"). The fix lives in World::SpawnChapterNpcs as a
// spawn-time filter so the missing NPC is identically modelled to the
// way the librarian is "not in Ch4" — absent from objects_, no special
// dialog opener path, no chapterRoster_ leak across the next Transition.
//
// The roster filter MUST observe both flags atomically: scolding without
// the Ch2 mending hides the senior; scolding then helping puts him back.

namespace {

bool HasNpcId(const World& w, const char* id) {
    for (const auto& o : w.Objects())
        if (o->NpcId() == std::string_view(id)) return true;
    return false;
}

} // namespace

TEST_CASE("M7: Flag_ScoldedSenior hides suit_senior from Ch4") {
    World w("", /*loadSprites=*/false);
    Player* p = w.GetPlayer();
    REQUIRE(p != nullptr);

    SUBCASE("ScoldedSenior alone removes suit_senior from Ch4") {
        p->SetFlag("Flag_ScoldedSenior");
        w.RespawnChapterRoster(SemesterState::Chapter4_Finals);

        // The other 4 archetypes stay; only suit_senior is gone.
        CHECK(HasNpcId(w, "victim"));
        CHECK(HasNpcId(w, "bookworm"));
        CHECK(HasNpcId(w, "ta"));
        CHECK(HasNpcId(w, "shop_auntie"));
        CHECK_FALSE(HasNpcId(w, "suit_senior"));
    }

    SUBCASE("ScoldedSenior + HelpedSenior re-introduces suit_senior") {
        // Player mended the relationship in Ch2 (callback note).
        p->SetFlag("Flag_ScoldedSenior");
        p->SetFlag("Flag_HelpedSenior");
        w.RespawnChapterRoster(SemesterState::Chapter4_Finals);

        CHECK(HasNpcId(w, "suit_senior"));
        CHECK(HasNpcId(w, "victim"));
        CHECK(HasNpcId(w, "bookworm"));
        CHECK(HasNpcId(w, "ta"));
        CHECK(HasNpcId(w, "shop_auntie"));
    }

    SUBCASE("Neither flag: default Ch4 roster has every archetype") {
        // No scolding, no callback — peak-intensity Ch4 with the full 5.
        w.RespawnChapterRoster(SemesterState::Chapter4_Finals);

        CHECK(HasNpcId(w, "suit_senior"));
        CHECK(HasNpcId(w, "victim"));
        CHECK(HasNpcId(w, "bookworm"));
        CHECK(HasNpcId(w, "ta"));
        CHECK(HasNpcId(w, "shop_auntie"));
    }

    SUBCASE("HelpedSenior alone is the default — suit_senior stays") {
        // A player who never scolded the senior should always see him.
        p->SetFlag("Flag_HelpedSenior");
        w.RespawnChapterRoster(SemesterState::Chapter4_Finals);

        CHECK(HasNpcId(w, "suit_senior"));
    }
}

TEST_CASE("M7: filter is Ch4-only — other chapters keep suit_senior") {
    // The Ch2 / Ch3 rosters include suit_senior with isQuestGiver=false
    // for the ripple narrative; M7 must not bleed into those chapters
    // even when the flag set would match.
    World w("", /*loadSprites=*/false);
    Player* p = w.GetPlayer();
    REQUIRE(p != nullptr);
    p->SetFlag("Flag_ScoldedSenior");   // would skip Ch4 only

    w.RespawnChapterRoster(SemesterState::Chapter2_Midterms);
    CHECK(HasNpcId(w, "suit_senior"));

    w.RespawnChapterRoster(SemesterState::Chapter3_SportsDay);
    CHECK(HasNpcId(w, "suit_senior"));

    // Walk all the way to Ch4 — gone.
    w.RespawnChapterRoster(SemesterState::Chapter4_Finals);
    CHECK_FALSE(HasNpcId(w, "suit_senior"));

    // ... and back to Ch3 — present again (no roster leak).
    w.RespawnChapterRoster(SemesterState::Chapter3_SportsDay);
    CHECK(HasNpcId(w, "suit_senior"));
}
