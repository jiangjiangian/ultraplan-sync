#ifndef CHAPTER_SPAWNS_H_
#define CHAPTER_SPAWNS_H_
#include "quest/NpcSpawns.h"
#include "state/SemesterState.h"
#include <vector>

namespace nccu {

// The chapter-NPC roster keyed by SemesterState. World::RespawnChapterRoster
// reads this every time the semester FSM advances, so the NPCs the player
// can talk to follow the current chapter instead of being frozen at the
// Ch1 set the ctor spawned.
//
// Ch1 delegates to DefaultNpcSpawns() so the roster is byte-identical to
// today (every existing Ch1 spawn/reachability test stays green with zero
// duplicated literals). The later states return their own empty static
// vector for now — the per-chapter rosters land in S5b..S5e; S5a-1 only
// proves the state-driven respawn MECHANISM. Ambient pedestrians are NOT
// per-chapter: AmbientStudentSpawns() stays global and the ctor keeps
// wiring it once after the terrain mask loads.
inline const std::vector<NpcSpawn>& ChapterNpcSpawns(SemesterState state) {
    static const std::vector<NpcSpawn> kInterlude;     // TODO(S5b): chapter roster

    // Ch2 圖書館期中考 (S5c-1). chapter2.md has 6 NPC sections; sprites
    // reuse Ch1's (art polish later). Placement now matches the narrative
    // geography instead of parking everyone on Ch1 anchors:
    //   • 圖書館管理員 (quest-giver) sits at the 中正圖書館 服務台 — just
    //     south of the library rect (rect bottom y=509; desk at y=545) —
    //     because chapter2.md routes the whole main line through her desk.
    //   • 學霸 (bookworm) sits under the 羅馬廣場 statue (plaza centre
    //     ≈1088,960; placed at the south rim 1088,1100) — the rescue beat
    //     every direction (note3 / 管理員(b) / 旁白) points the player to.
    // The other 4 archetypes stay on their Ch1 anchors (optional / ripple,
    // isQuestGiver=false). All coords mask-verified walkable via
    // .claude/tools/map_registry.py, whose Ch2 expectation checks pin
    // librarian↔中正圖書館 and bookworm↔羅馬廣場.
    static const std::vector<NpcSpawn> kChapter2 = {
        {gfx::Vec2{ 380, 1860}, "resources/assets/sprites/school_uniform_3/male_02.png",
         "victim", false},
        {gfx::Vec2{ 980, 1640}, "resources/assets/sprites/npc/suit_senior.png",
         "suit_senior", false},
        {gfx::Vec2{1088, 1100}, "resources/assets/sprites/school_uniform_3/female_03.png",
         "bookworm", false},
        {gfx::Vec2{1706, 1766}, "resources/assets/sprites/npc/ta.png",
         "ta", false},
        {gfx::Vec2{ 460, 1500}, "resources/assets/sprites/npc/shop_auntie.png",
         "shop_auntie", false},
        {gfx::Vec2{ 820,  545}, "resources/assets/sprites/school_uniform_3/female_01.png",
         "librarian", true},
    };
    // Ch3 校慶運動會 (S5d-1). chapter3.md has 8 ## NPC：sections — the
    // 5 archetypes (ripple / optional, isQuestGiver=false, same stance
    // as Ch2) + 3 物物交換鏈 nodes (A系香腸 / B系大聲公 / C系學姊,
    // isQuestGiver=true — they drive the main quest). Sprites reuse
    // existing art; positions reuse Ch1/Ch2-proven walkable coords so
    // the single-z-plane campus stays reachable (reachability is a
    // manual-verify item, same as every map-position table here).
    // cycle9c reposition: the 3 trade-chain quest-givers used to sit
    // at y=1850 — well south of the Ch3 entry's camera viewport on the
    // road's south edge, so the H4 quest-giver "!" indicator never
    // entered view (cycle9_diag_a frame 4200 confirmed 0 indicators
    // visible). Pulling them up to y=1820 — the top of the south
    // corridor, just below the E-W wall at y~1761-1819 — puts the
    // indicator centred in the entry viewport: world Y=1820 sprite
    // top at 1812 + indicator panel at 1776 lands ~80 px below the
    // player's eye-line at the y~1890 Ch3 entry, while the south
    // corridor (y in [1820, 1940]) stays the script's natural
    // navigation lane (the spine's `interact vendor_sausage_a` path
    // is the same axis-only X-then-Y descent it always was). Original
    // x values (760 / 1080 / 1320) preserved verbatim so the
    // "spread across the south campus" narrative beat — chapter3.md's
    // 田徑場旁 / 隔壁B系 / 四維道 — reads identically; only y dropped
    // by 30 px. cycle9_diag_a frame 4200 0 indicators → cycle9c_smoke
    // 3 indicators after this change (verified by gold-pixel scan).
    // New coords mask-verified walkable and pass
    // test_spawn_reachability's flood-fill from the player spawn just
    // like every other production coord.
    static const std::vector<NpcSpawn> kChapter3 = {
        {gfx::Vec2{ 380, 1860}, "resources/assets/sprites/school_uniform_3/male_02.png",
         "victim", false},
        {gfx::Vec2{ 980, 1640}, "resources/assets/sprites/npc/suit_senior.png",
         "suit_senior", false},
        {gfx::Vec2{ 480, 1280}, "resources/assets/sprites/school_uniform_3/female_03.png",
         "bookworm", false},
        {gfx::Vec2{1706, 1766}, "resources/assets/sprites/npc/ta.png",
         "ta", false},
        {gfx::Vec2{ 460, 1500}, "resources/assets/sprites/npc/shop_auntie.png",
         "shop_auntie", false},
        {gfx::Vec2{ 760, 1820}, "resources/assets/sprites/npc/shop_auntie.png",
         "vendor_sausage_a", true},
        {gfx::Vec2{1320, 1820}, "resources/assets/sprites/school_uniform_3/male_02.png",
         "loudspeaker_b", true},
        {gfx::Vec2{1080, 1825}, "resources/assets/sprites/school_uniform_3/female_01.png",
         "senior_c", true},
    };
    // Ch4 期末考終焉 (S5e-1). chapter4.md has 5 ## NPC：sections —
    // the 5 archetypes at peak intensity, no new NPCs. All
    // isQuestGiver=false (the finale is gate-driven, not quest-giver
    // driven — Ending A/B/C resolve in CheckEndingGates; 助教 is
    // central but via the (d) 體諒 choice, not a quest-giver marker).
    // Sprites reuse existing art; positions reuse the proven walkable
    // coords (reachability is a manual-verify item, as everywhere).
    // 西裝學長 still ships in this list because the table is pure data;
    // the M7 (cycle9c) Ch4 「斥責後不出場」 ripple is enforced by a
    // spawn-time filter inside World::SpawnChapterNpcs that skips
    // pushing suit_senior into objects_ when the player carries
    // Flag_ScoldedSenior without the mending Flag_HelpedSenior (the Ch2
    // callback). Keeping the gate at the spawner — not the data table —
    // mirrors how the librarian is only in Ch2: an NPC "not in this
    // chapter" is identically modelled as "not in objects_", and the
    // chapterRoster_ teardown / Chapter4Quest routing both Just Work.
    static const std::vector<NpcSpawn> kChapter4 = {
        {gfx::Vec2{ 380, 1860}, "resources/assets/sprites/school_uniform_3/male_02.png",
         "victim", false},
        {gfx::Vec2{ 980, 1640}, "resources/assets/sprites/npc/suit_senior.png",
         "suit_senior", false},
        {gfx::Vec2{ 480, 1280}, "resources/assets/sprites/school_uniform_3/female_03.png",
         "bookworm", false},
        {gfx::Vec2{1706, 1766}, "resources/assets/sprites/npc/ta.png",
         "ta", false},
        {gfx::Vec2{ 460, 1500}, "resources/assets/sprites/npc/shop_auntie.png",
         "shop_auntie", false},
    };
    static const std::vector<NpcSpawn> kEndingA;       // TODO(S5e): chapter roster
    static const std::vector<NpcSpawn> kEndingB;       // TODO(S5e): chapter roster
    static const std::vector<NpcSpawn> kEndingC;       // TODO(S5e): chapter roster

    switch (state) {
        case SemesterState::Chapter1_AddDrop:   return DefaultNpcSpawns();
        case SemesterState::Interlude_Market:   return kInterlude;
        case SemesterState::Chapter2_Midterms:  return kChapter2;
        case SemesterState::Chapter3_SportsDay: return kChapter3;
        case SemesterState::Chapter4_Finals:    return kChapter4;
        case SemesterState::Ending_A:           return kEndingA;
        case SemesterState::Ending_B:           return kEndingB;
        case SemesterState::Ending_C:           return kEndingC;
    }
    return kInterlude;  // unreachable; keeps non-void paths total
}

} // namespace nccu

#endif // CHAPTER_SPAWNS_H_
