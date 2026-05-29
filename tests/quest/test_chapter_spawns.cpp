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

// The 5 Ch1 spine/ripple archetypes — the npcIds the ChapterNpcSpawns(Ch1)
// DATA table declares. G-1/G-2 added a Ch1 crowd (wanderers, empty npcId) +
// 3 stationary flavor NPCs (ch1_flavor_* npcIds) via World::SpawnChapterNpcs'
// code block, NOT the data table, so the live npcId roster is now the 5
// archetypes PLUS the 3 flavor ids. These helpers keep the spine assertions
// exact (the 5 archetypes are unchanged) while tolerating the flavor ids.
const std::set<std::string>& Ch1Archetypes() {
    static const std::set<std::string> kIds = {
        "victim", "suit_senior", "bookworm", "ta", "shop_auntie"};
    return kIds;
}

// Live count of just the spine archetypes (excludes the flavor NPCs).
std::size_t ArchetypeNpcCount(const World& w) {
    std::size_t n = 0;
    for (const auto& id : RosterNpcIds(w))
        if (Ch1Archetypes().count(id)) ++n;
    return n;
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
    CHECK(ChapterNpcSpawns(SemesterState::Ending_D).empty());   // G1
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
    const std::size_t ch1Cash    = RosterCashCount(w);
    // The 5 spine archetypes are present at entry (G-1/G-2's crowd + flavor
    // NPCs are ADDITIVE — counted via the live roster below, not here).
    REQUIRE(ArchetypeNpcCount(w) == 5);
    REQUIRE(ch1Cash == 5);                              // S5b-4 Ch1 spread
    REQUIRE(nccu::ChapterQuestItems(                    // table declares 1...
                SemesterState::Chapter1_AddDrop).size() == 1);

    // --- Transition to a state with an empty roster. ---
    // Vehicle history: Interlude (S5a-1) → non-empty at S5b-3 (Vendors);
    // Chapter2 (S5b-3) → non-empty at S5c-1. Ending_A is the DURABLE
    // choice: an ending renders a card and has no NPC roster / no
    // Vendors / no CashPickups by design — it stays empty through all of
    // Phase 2, so this swap-mechanism + Player-invariant test never
    // churns again as S5d/S5e fill the remaining chapters.
    w.RespawnChapterRoster(SemesterState::Ending_A);

    // The Ch1 NPCs are gone (the 5 archetypes AND the 3 flavor NPCs — every
    // chapter-roster member is swept); no chapter NPC id remains.
    CHECK(RosterNpcIds(w).empty());
    for (const char* id : {"victim", "suit_senior", "bookworm",
                           "ta", "shop_auntie"})
        CHECK_FALSE(HasNpc(w, id));

    // Player identity + front invariant survive the swap intact.
    CHECK(w.GetPlayer() == beforePlayer);
    CHECK(w.Objects().front().get() == beforeFront);
    CHECK(static_cast<GameObject*>(w.GetPlayer()) == w.Objects().front().get());

    // Everything that is NOT a chapter-roster member is still present
    // (player + 3 morality umbrellas + the ctor 申請書 QuestFlagPickup +
    // ambient students — none of which are roster-tracked). The Ch1 crowd +
    // flavor NPCs (G-1/G-2) ARE roster-tracked, so they were swept too; we
    // capture the post-swap survivor count directly rather than deriving it.
    const std::size_t nonChapter = w.Objects().size();
    CHECK(nonChapter < totalCh1);                        // the roster was swept

    // --- Round-trip back to Ch1: the full roster comes back. ---
    w.RespawnChapterRoster(SemesterState::Chapter1_AddDrop);

    for (const char* id : {"victim", "suit_senior", "bookworm",
                           "ta", "shop_auntie"})
        CHECK(HasNpc(w, id));
    CHECK(ArchetypeNpcCount(w) == 5);                    // spine unchanged
    CHECK(w.Objects().size() == totalCh1);               // crowd+flavor restored

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
    CHECK(ArchetypeNpcCount(w) == 5);                    // spine: no dup/leak
    CHECK(static_cast<GameObject*>(w.GetPlayer()) == w.Objects().front().get());
}
