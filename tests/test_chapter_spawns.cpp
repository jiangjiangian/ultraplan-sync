#include "doctest/doctest.h"
#include "ChapterSpawns.h"
#include "World.h"
#include "Player.h"
#include "GameObject.h"
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

} // namespace

TEST_CASE("ChapterNpcSpawns: Ch1 is the 5 archetypes, later states empty") {
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

    CHECK(ChapterNpcSpawns(SemesterState::Interlude_Market).empty());
    CHECK(ChapterNpcSpawns(SemesterState::Chapter2_Midterms).empty());
    CHECK(ChapterNpcSpawns(SemesterState::Chapter3_SportsDay).empty());
    CHECK(ChapterNpcSpawns(SemesterState::Chapter4_Finals).empty());
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
    REQUIRE(ch1Npcs == 5);
    const std::size_t nonChapter = totalCh1 - ch1Npcs;  // player + 4 umbrellas
                                                        // + pickup + ambients

    // --- Transition to a chapter with an empty roster. ---
    w.RespawnChapterRoster(SemesterState::Interlude_Market);

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
