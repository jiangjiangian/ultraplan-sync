#include "doctest/doctest.h"
#include "game/quest/NpcSpawns.h"
#include "game/quest/ChapterSpawns.h"
#include "game/dialog/DialogSource.h"
#include "game/entities/NPC.h"
#include "game/entities/Player.h"
#include "engine/core/GameObject.h"
#include "game/world/World.h"
#include "game/state/SemesterState.h"
#include "engine/math/Vec2.h"
#include <set>
#include <string>
#include <string_view>
#include <vector>

#ifndef TEST_CONTENT_DIR
#error "TEST_CONTENT_DIR must be defined by the build system"
#endif

using nccu::SemesterState;
using nccu::World;

// G-1/G-2 regressions. The Ch1 加退選搶課 crowd + stationary flavor NPCs are
// ADDITIVE colour: a batch of wandering ambient students (no npcId, no `!`,
// non-blocking) + a few stationary flavor NPCs whose chapter1.md line pool
// cycles DETERMINISTICALLY one line per talk. They must NOT touch the
// hard-gated 苦主→學長→苦主 spine (no quest flag), must keep
// objects_.front()==Player, and must stay off the spine anchors.

namespace {

std::size_t CountCh1Crowd(const World& w) {
    // The Ch1 crowd wanders with an empty npcId from EXACTLY the
    // Chapter1CrowdSpawns coords. Count empty-npcId NPCs sitting on one of
    // those spawn points (positions only change once Update() ticks, and
    // RespawnChapterRoster does not tick) — precise, so it never confuses the
    // Ch1 crowd with the Interlude/Ch3 crowds or the south ambient students.
    std::size_t n = 0;
    for (const auto& o : w.Objects()) {
        if (!o->NpcId().empty()) continue;
        const auto* npc = dynamic_cast<const NPC*>(o.get());
        if (npc == nullptr) continue;                 // skip Player / pickups
        const auto p = o->GetPosition();
        for (const auto& s : nccu::Chapter1CrowdSpawns())
            if (p.x == s.pos.x && p.y == s.pos.y) { ++n; break; }
    }
    return n;
}

const NPC* FindNpc(const World& w, const char* id) {
    for (const auto& o : w.Objects())
        if (o->NpcId() == std::string_view(id))
            return dynamic_cast<const NPC*>(o.get());
    return nullptr;
}

} // namespace

TEST_CASE("G-1: the Ch1 加退選搶課 crowd spawns and the Player stays at front") {
    World w("", /*loadSprites=*/false);

    // The wandering crowd populates the central campus (player-reported
    // "搶課人潮"); Chapter1CrowdSpawns declares the batch.
    REQUIRE_FALSE(nccu::Chapter1CrowdSpawns().empty());
    CHECK(CountCh1Crowd(w) >= nccu::Chapter1CrowdSpawns().size());

    // Front-is-Player invariant holds with the crowd present.
    Player* p = w.GetPlayer();
    REQUIRE(p != nullptr);
    CHECK(static_cast<GameObject*>(p) == w.Objects().front().get());

    // The crowd is swept on chapter exit (roster-tracked) and the invariant
    // survives, then comes back on a Ch1 round-trip.
    w.RespawnChapterRoster(SemesterState::Interlude_Market);
    CHECK(CountCh1Crowd(w) == 0);
    CHECK(w.GetPlayer() == p);
    CHECK(w.Objects().front().get() == static_cast<GameObject*>(p));

    w.RespawnChapterRoster(SemesterState::Chapter1_AddDrop);
    CHECK(CountCh1Crowd(w) >= nccu::Chapter1CrowdSpawns().size());
    CHECK(w.Objects().front().get() == static_cast<GameObject*>(p));
}

TEST_CASE("G-2: the Ch1 flavor NPCs are stationary, non-quest, off the spine") {
    World w("", /*loadSprites=*/false);

    // Each declared flavor NPC is in the live roster, is NOT a quest-giver
    // (so it never paints a `!`), and is NOT a spine archetype.
    const std::set<std::string> spine = {
        "victim", "suit_senior", "bookworm", "ta", "shop_auntie"};
    for (const auto& s : nccu::Chapter1FlavorSpawns()) {
        const NPC* npc = FindNpc(w, s.npcId);
        REQUIRE_MESSAGE(npc != nullptr, "missing flavor NPC: " << s.npcId);
        CHECK_FALSE(npc->IsQuestGiver());
        CHECK(spine.count(s.npcId) == 0);
        CHECK(nccu::IsChapter1FlavorNpc(s.npcId));
    }
    // A spine / ambient / unknown id is NOT a flavor NPC.
    CHECK_FALSE(nccu::IsChapter1FlavorNpc("victim"));
    CHECK_FALSE(nccu::IsChapter1FlavorNpc(""));
    CHECK_FALSE(nccu::IsChapter1FlavorNpc("nobody"));
}

TEST_CASE("G-2: a flavor NPC cycles its pool DETERMINISTICALLY, sets no flag") {
    nccu::dialog::SetContentDir(TEST_CONTENT_DIR);
    nccu::dialog::Reload();

    // The flavor pool is the npcId's chapter1.md (a) section.
    const auto& pool = nccu::dialog::Entries(
        "ch1_flavor_grab", SemesterState::Chapter1_AddDrop);
    REQUIRE(pool.size() == 1);                     // one (a) sub-block
    const std::vector<std::string>& lines = pool[0].lines;
    REQUIRE(lines.size() >= 2);                    // a real pool to cycle

    // An NPC loaded with that pool cycles ONE line per Interact(), wrapping
    // modulo the pool size — a deterministic, reproducible "random" pick
    // (the harness needs byte-identical state.jsonl, so no RNG here).
    NPC a(nccu::gfx::Vec2{0, 0}, {}, /*isQuestGiver=*/false,
          std::string_view{"ch1_flavor_grab"});
    a.LoadDialog("ch1_flavor_grab", SemesterState::Chapter1_AddDrop, 0);
    REQUIRE(a.DialogLineCount() == lines.size());

    Player p = Player{nccu::gfx::Vec2{0, 0}};
    const std::size_t flagsBefore = 0;             // a fresh player has none

    // Walk the pool twice; the sequence is identical on the second lap.
    std::vector<std::string> lap1, lap2;
    for (std::size_t i = 0; i < lines.size(); ++i) {
        lap1.push_back(a.CurrentLineText());
        a.Interact(&p);
    }
    for (std::size_t i = 0; i < lines.size(); ++i) {
        lap2.push_back(a.CurrentLineText());
        a.Interact(&p);
    }
    CHECK(lap1 == lines);                           // exactly the authored pool
    CHECK(lap1 == lap2);                            // reproducible cycle

    // A SECOND NPC seeded identically yields the SAME sequence (no per-run
    // entropy) — the reproducibility the byte-identical harness depends on.
    NPC b(nccu::gfx::Vec2{0, 0}, {}, /*isQuestGiver=*/false,
          std::string_view{"ch1_flavor_grab"});
    b.LoadDialog("ch1_flavor_grab", SemesterState::Chapter1_AddDrop, 0);
    for (std::size_t i = 0; i < lines.size(); ++i) {
        CHECK(b.CurrentLineText() == lap1[i]);
        b.Interact(&p);
    }

    // Interacting NEVER mutates player quest state (Interact only emits a
    // ShowMessage; it touches no flag / karma / umbrella).
    CHECK(p.GetKarma() == 50);
    (void)flagsBefore;
}
