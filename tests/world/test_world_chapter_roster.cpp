#include "doctest/doctest.h"
#include "quest/ChapterSpawns.h"
#include "entities/GameObject.h"
#include "entities/NPC.h"
#include "entities/Player.h"
#include "world/World.h"
#include <cmath>
#include <set>
#include <string>
#include <string_view>

using nccu::SemesterState;
using nccu::World;

// H1 (cycle9): the bughunter diagnosis claimed Ch1 NPCs leaked into the
// Interlude/Ending because the World ctor pushed them to objects_ without
// registering them in chapterRoster_. Reading World.cpp:47 shows the ctor
// already routes Ch1 spawns through RespawnChapterRoster -> SpawnChapterNpcs,
// which DOES record every NPC in the tracked roster. The 5-NPC stale
// observation in the playtest was actually the 1-frame respawn lag — the
// state.jsonl snapshot is written AFTER the FSM hop but BEFORE the next
// frame's chapterRoster_ sweep. This test pins the roster-mechanism
// invariant the diagnosis would have caught: any transition AWAY from a
// chapter strips the previous chapter's quest-givers from objects_, with
// the Player + umbrellas + ambient students surviving intact.

namespace {

bool HasNpcId(const World& w, const char* id) {
    for (const auto& o : w.Objects())
        if (o->NpcId() == std::string_view(id)) return true;
    return false;
}

std::size_t QuestGiverCount(const World& w) {
    std::size_t n = 0;
    for (const auto& o : w.Objects())
        if (const auto* npc = dynamic_cast<const NPC*>(o.get()))
            if (npc->IsQuestGiver()) ++n;
    return n;
}

std::set<std::string> ChapterNpcIdSet(SemesterState s) {
    std::set<std::string> ids;
    for (const auto& sp : nccu::ChapterNpcSpawns(s)) ids.insert(sp.npcId);
    return ids;
}

} // namespace

TEST_CASE("World ctor registers Ch1 NPCs in chapterRoster_ so they sweep") {
    World w("", /*loadSprites=*/false);

    // Pre-condition: the 5 Ch1 archetypes are present after the ctor.
    REQUIRE(HasNpcId(w, "victim"));
    REQUIRE(HasNpcId(w, "suit_senior"));
    REQUIRE(HasNpcId(w, "bookworm"));
    REQUIRE(HasNpcId(w, "ta"));
    REQUIRE(HasNpcId(w, "shop_auntie"));

    // Walk to the Interlude. Spawns are empty there; the 5 Ch1 NPCs MUST
    // be gone or the diagnosis claim (chapterRoster_ never tracked them)
    // would be true and the next chapter would inherit them.
    w.RespawnChapterRoster(SemesterState::Interlude_Market);

    CHECK_FALSE(HasNpcId(w, "victim"));
    CHECK_FALSE(HasNpcId(w, "suit_senior"));
    CHECK_FALSE(HasNpcId(w, "bookworm"));
    CHECK_FALSE(HasNpcId(w, "ta"));
    CHECK_FALSE(HasNpcId(w, "shop_auntie"));

    // The cycle9 brief asks for the stronger "after the swap, the object
    // list's NPC ids match ChapterNpcSpawns(state) exactly (modulo
    // ambient pedestrians, which carry no npcId by design)". Interlude's
    // chapter spawns are empty, so we expect ZERO quest-givers and ZERO
    // chapter NPCs after the swap.
    CHECK(QuestGiverCount(w) == 0);
}

TEST_CASE("World ctor: every Ch1 NPC is a chapterRoster_ member") {
    World w("", /*loadSprites=*/false);

    // Counted via behaviour, not direct chapterRoster_ access (which is
    // private). If even one Ch1 NPC were ctor-pushed without being
    // registered, this assertion would fail — exactly the regression
    // the diagnosis described.
    const std::size_t questGiversCh1 = QuestGiverCount(w);
    REQUIRE(questGiversCh1 >= 1);   // at least the 苦主

    w.RespawnChapterRoster(SemesterState::Ending_A);
    CHECK(QuestGiverCount(w) == 0);
}

TEST_CASE("Roster sweep across the full chapter spine: no NPC leaks") {
    World w("", /*loadSprites=*/false);

    // The full Ch1 -> Interlude -> Ch2 -> Interlude -> Ch3 -> Interlude
    // -> Ch4 -> Ending walk. After each step, the only chapter NPCs in
    // objects_ are the ones the destination state's roster claims.
    const SemesterState path[] = {
        SemesterState::Interlude_Market,
        SemesterState::Chapter2_Midterms,
        SemesterState::Interlude_Market,
        SemesterState::Chapter3_SportsDay,
        SemesterState::Interlude_Market,
        SemesterState::Chapter4_Finals,
        SemesterState::Ending_A,
    };

    for (SemesterState s : path) {
        w.RespawnChapterRoster(s);

        const auto expected = ChapterNpcIdSet(s);
        for (const auto& o : w.Objects()) {
            const std::string id{o->NpcId()};
            if (id.empty()) continue;          // ambient students / Player
            CHECK(expected.count(id) == 1);    // current chapter only
        }
        for (const auto& id : expected)
            CHECK(HasNpcId(w, id.c_str()));
    }
}

TEST_CASE("Player invariant survives the full spine traversal") {
    World w("", /*loadSprites=*/false);
    Player*     p0 = w.GetPlayer();
    GameObject* f0 = w.Objects().front().get();
    REQUIRE(p0 != nullptr);
    REQUIRE(static_cast<GameObject*>(p0) == f0);

    for (SemesterState s : {SemesterState::Interlude_Market,
                            SemesterState::Chapter2_Midterms,
                            SemesterState::Chapter3_SportsDay,
                            SemesterState::Chapter4_Finals,
                            SemesterState::Ending_B,
                            SemesterState::Ending_C,
                            SemesterState::Chapter1_AddDrop}) {
        w.RespawnChapterRoster(s);
        CHECK(w.GetPlayer() == p0);
        CHECK(w.Objects().front().get() == f0);
    }
}

TEST_CASE("Ch3 trade-chain NPCs sit scattered in 羅馬廣場") {
    // The 3 物物交換鏈 quest-givers (vendor_sausage_a / loudspeaker_b /
    // senior_c) moved from the old south corridor into 羅馬廣場 (player
    // request), where the player heads after running the 操場 lap. Pin them
    // inside the walkable plaza disc (centre ~1088,960, r~200) so a future
    // tweak can't scatter them back across the south campus.
    World w("", /*loadSprites=*/false);
    w.RespawnChapterRoster(SemesterState::Chapter3_SportsDay);

    for (const char* id : {"vendor_sausage_a", "loudspeaker_b", "senior_c"}) {
        const GameObject* hit = nullptr;
        for (const auto& o : w.Objects())
            if (o->NpcId() == std::string_view(id)) { hit = o.get(); break; }
        REQUIRE_MESSAGE(hit != nullptr,
                        "Ch3 trade-chain NPC missing: " << id);
        const auto p = hit->GetPosition();
        const float d = std::hypot(p.x - 1088.0f, p.y - 960.0f);
        CHECK_MESSAGE(d < 220.0f,
                      id << " at (" << p.x << "," << p.y
                         << ") is " << d << " px from 羅馬廣場 centre");
    }
}

TEST_CASE("Ch3: the 操場 校慶 crowd spawns (5 runners + 10 idlers)") {
    // The decorative crowd populates the 操場 (x1384-2005, y541-940) so the
    // sports day actually has people (player-reported "沒人在操場"). Counted
    // at spawn-time: the umbrella (y375) / archetypes / ABC (plaza) / ambient
    // students (south) all fall outside the field box, so this counts crowd.
    World w("", /*loadSprites=*/false);
    w.RespawnChapterRoster(SemesterState::Chapter3_SportsDay);

    int crowd = 0;
    for (const auto& o : w.Objects()) {
        const auto p = o->GetPosition();
        if (p.x >= 1384.0f && p.x <= 2005.0f && p.y >= 541.0f && p.y <= 940.0f)
            ++crowd;
    }
    CHECK(crowd >= 15);
}
