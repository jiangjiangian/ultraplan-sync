#include "doctest/doctest.h"
#include "quest/ChapterSpawns.h"
#include "quest/ChapterQuestItems.h"
#include "entities/CashPickup.h"
#include "world/World.h"
#include "entities/Player.h"
#include "entities/GameObject.h"
#include <algorithm>
#include <set>
#include <string>
#include <string_view>
#include <vector>

using nccu::ChapterNpcSpawns;
using nccu::SemesterState;
using nccu::World;

namespace {

// npcIds present in the live object roster (NPC overrides NpcId()).
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

// CashPickups are chapter-roster members too (S5b-4: per-chapter coins,
// swept on a state change like the NPCs), so the "survives a swap" count
// must exclude them as well.
std::size_t RosterCashCount(const World& w) {
    std::size_t n = 0;
    for (const auto& o : w.Objects())
        if (dynamic_cast<const CashPickup*>(o.get())) ++n;
    return n;
}

} // namespace

TEST_CASE("ChapterNpcSpawns: Ch1 is the 5 archetypes; Interlude+endings empty") {
    const auto& ch1 = ChapterNpcSpawns(SemesterState::Chapter1_AddDrop);
    std::set<std::string> ids;
    for (const auto& s : ch1) ids.insert(s.npcId);
    CHECK(ids == std::set<std::string>{"victim", "suit_senior", "bookworm",
                                       "ta", "shop_auntie"});
    CHECK(ch1.size() == 5);

    // Ch1 must be byte-identical to the legacy DefaultNpcSpawns().
    const auto& legacy = nccu::DefaultNpcSpawns();
    REQUIRE(ch1.size() == legacy.size());
    for (std::size_t i = 0; i < ch1.size(); ++i) {
        CHECK(std::string(ch1[i].npcId) == std::string(legacy[i].npcId));
        CHECK(ch1[i].pos.x == legacy[i].pos.x);
        CHECK(ch1[i].pos.y == legacy[i].pos.y);
        CHECK(ch1[i].isQuestGiver == legacy[i].isQuestGiver);
    }

    // States with no NPC roster through ALL of Phase 2 (durable — does
    // not churn as S5c/d/e fill the chapters): the Interlude has none
    // (its actors are parser Vendors via ChapterVendors, not NPCs; the
    // exit is a trigger zone, S5b-2), and the three endings render a
    // card with no roster. Ch2/Ch3/Ch4 are populated by design as each
    // S5c/d/e lands — their rosters are pinned by their own tests
    // (test_chapter2_roster, …), not by a blanket "later states empty"
    // assertion here.
    CHECK(ChapterNpcSpawns(SemesterState::Interlude_Market).empty());
    CHECK(ChapterNpcSpawns(SemesterState::Ending_A).empty());
    CHECK(ChapterNpcSpawns(SemesterState::Ending_B).empty());
    CHECK(ChapterNpcSpawns(SemesterState::Ending_C).empty());
}

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

TEST_CASE("RespawnChapterRoster swaps NPCs but preserves the Player invariant") {
    World w("", /*loadSprites=*/false);

    // Capture identity + the non-chapter object count BEFORE any swap.
    Player*     beforePlayer = w.GetPlayer();
    GameObject* beforeFront  = w.Objects().front().get();
    REQUIRE(beforePlayer != nullptr);
    REQUIRE(static_cast<GameObject*>(beforePlayer) == beforeFront);

    const std::size_t totalCh1   = w.Objects().size();
    const std::size_t ch1Npcs    = RosterNpcIds(w).size();
    const std::size_t ch1Cash    = RosterCashCount(w);
    // 善有善報 redesign: Ch1's ChapterQuestItems also seeds roster-tracked
    // QuestFlagPickup(s) (the 苦主's umbrella) that are swept on a state
    // change, so they too must be excluded from the "survives a swap" set.
    const std::size_t ch1QuestItems =
        nccu::ChapterQuestItems(SemesterState::Chapter1_AddDrop).size();
    REQUIRE(ch1Npcs == 5);
    REQUIRE(ch1Cash == 5);                              // S5b-4 Ch1 spread
    REQUIRE(ch1QuestItems == 1);                        // the 苦主's umbrella
    // Survives a roster swap = everything that is NOT a chapter-roster
    // member. The roster is the 5 NPCs, the 5 CashPickups (S5b-4) AND the
    // Ch1 quest-item pickup(s); all are subtracted; what remains is player
    // + 3 morality umbrellas + the ctor 申請書 QuestFlagPickup (NOT roster-
    // tracked) + ambient students.
    const std::size_t nonChapter =
        totalCh1 - ch1Npcs - ch1Cash - ch1QuestItems;

    // --- Transition to a state with an empty roster. ---
    // Vehicle history: Interlude (S5a-1) → non-empty at S5b-3 (Vendors);
    // Chapter2 (S5b-3) → non-empty at S5c-1. Ending_A is the DURABLE
    // choice: an ending renders a card and has no NPC roster / no
    // Vendors / no CashPickups by design — it stays empty through all of
    // Phase 2, so this swap-mechanism + Player-invariant test never
    // churns again as S5d/S5e fill the remaining chapters.
    w.RespawnChapterRoster(SemesterState::Ending_A);

    // The 5 Ch1 NPCs are gone; no chapter NPCs remain.
    CHECK(RosterNpcIds(w).empty());
    for (const char* id : {"victim", "suit_senior", "bookworm",
                           "ta", "shop_auntie"})
        CHECK_FALSE(HasNpc(w, id));

    // Player identity + front invariant survive the swap intact.
    CHECK(w.GetPlayer() == beforePlayer);
    CHECK(w.Objects().front().get() == beforeFront);
    CHECK(static_cast<GameObject*>(w.GetPlayer()) == w.Objects().front().get());

    // Everything that is NOT a chapter NPC is still present (umbrellas,
    // QuestFlagPickup, ambient students all untouched).
    CHECK(w.Objects().size() == nonChapter);

    // --- Round-trip back to Ch1: the 5 archetypes come back. ---
    w.RespawnChapterRoster(SemesterState::Chapter1_AddDrop);

    for (const char* id : {"victim", "suit_senior", "bookworm",
                           "ta", "shop_auntie"})
        CHECK(HasNpc(w, id));
    CHECK(RosterNpcIds(w).size() == 5);
    CHECK(w.Objects().size() == totalCh1);

    // Invariant still holds after the round-trip.
    CHECK(w.GetPlayer() == beforePlayer);
    CHECK(w.Objects().front().get() == beforeFront);
}

TEST_CASE("Repeated same-state respawn does not duplicate or leak NPCs") {
    World w("", /*loadSprites=*/false);
    const std::size_t total = w.Objects().size();

    w.RespawnChapterRoster(SemesterState::Chapter1_AddDrop);
    w.RespawnChapterRoster(SemesterState::Chapter1_AddDrop);

    CHECK(w.Objects().size() == total);
    CHECK(RosterNpcIds(w).size() == 5);
    CHECK(static_cast<GameObject*>(w.GetPlayer()) == w.Objects().front().get());
}
