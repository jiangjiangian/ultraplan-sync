#ifndef CHAPTER_SPAWNS_H_
#define CHAPTER_SPAWNS_H_
#include "game/quest/NpcSpawns.h"
#include "game/state/SemesterState.h"
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
    // reuse Ch1's (art polish later). Placement matches the narrative
    // geography (the 場景 lines), not the Ch1 anchors:
    //   • 圖書館管理員 (quest-giver) sits at the 中正圖書館 服務台 — just
    //     south of the library rect (rect bottom y=509; desk at y=545) —
    //     because chapter2.md routes the whole main line through her desk.
    //     KEEP (the Ch2 quest depends on her position).
    //   • 學霸 (bookworm) sits under the 羅馬廣場 statue (plaza centre
    //     ≈1088,960; placed at the south rim 1088,1100) — the rescue beat
    //     every direction (note3 / 管理員(b) / 旁白) points the player to.
    //     KEEP (the Ch2 rescue beat depends on it).
    //   • 西裝學長 → 行政大樓 front (1220,775): chapter2.md「場景：行政大樓
    //     正門附近，靠著柱子滑手機」.
    //   • 助教 → 中正圖書館 front (900,545): chapter2.md「場景：中正圖書館…
    //     圖書館一樓走廊（背景 NPC 身份）」.
    //   • 苦主 → 中正圖書館 front, WEST corner (720,560): chapter2.md「場景：
    //     中正圖書館二樓閱覽區走廊角落」. Sits west of the 管理員 desk so it
    //     stays >=80 px clear of the ta / librarian / 自動販賣機 (980,560)
    //     cluster sharing the library apron.
    //   • 福利社阿姨 → 樂活小舖 (1560,1560): chapter2.md「場景：樂活小舖內」.
    // The 4 archetypes are optional / ripple (isQuestGiver=false). All
    // coords mask-verified STRICTLY walkable + flood-reachable via
    // .claude/tools/map_registry.py, whose Ch2 expectation checks pin
    // librarian↔中正圖書館 and bookworm↔羅馬廣場.
    static const std::vector<NpcSpawn> kChapter2 = {
        {nccu::engine::math::Vec2{ 720,  560}, "resources/assets/sprites/school_uniform_3/male_02.png",
         "victim", false},
        {nccu::engine::math::Vec2{1220,  775}, "resources/assets/sprites/npc/suit_senior.png",
         "suit_senior", false},
        {nccu::engine::math::Vec2{1088, 1100}, "resources/assets/sprites/school_uniform_3/female_03.png",
         "bookworm", false},
        {nccu::engine::math::Vec2{ 900,  545}, "resources/assets/sprites/npc/ta.png",
         "ta", false},
        {nccu::engine::math::Vec2{1560, 1560}, "resources/assets/sprites/npc/shop_auntie.png",
         "shop_auntie", false},
        {nccu::engine::math::Vec2{ 820,  545}, "resources/assets/sprites/school_uniform_3/female_01.png",
         "librarian", true},
    };
    // Ch3 校慶運動會. The 5 archetypes (ripple / optional,
    // isQuestGiver=false) now match their 場景 lines instead of their
    // Ch1/Ch2 anchors:
    //   • 西裝學長 → 操場 看台邊緣 (1430,910): chapter3.md「場景：操場看台
    //     邊緣，啦啦隊加油區旁」.
    //   • 助教 → 操場 邊緣 折疊桌 (1530,930): chapter3.md「場景：操場邊緣的
    //     折疊桌…負責登記活動出席」.
    //   • 苦主 → 操場 邊緣角落 (1620,920): chapter3.md「場景：操場邊緣，不在
    //     人群裡…在角落擺了一個小販攤」.
    //   • 福利社阿姨 → 四維道 (1100,860): chapter3.md「場景：四維道擺臨時攤」
    //     (just south of 四維堂 rect 1048,727,146x126).
    //   • 學霸 → 中正圖書館 (900,545): chapter3.md「場景：不在操場。在中正
    //     圖書館…繼續讀書」 — he POINTEDLY skips the 校慶, so he is at the
    //     library, NOT the 操場.
    // The 3 操場 archetypes sit on the FIELD SOUTH RIM (y≈910-930), all
    // outside the runner circle (track centre 1694,740, r150 — they are
    // 195-314 px from centre) and ≥128 px from the nearest idler, so they
    // never sit on the lap track or overlap the 校慶 crowd.
    // The 3 物物交換鏈 quest-givers (A系香腸 / B系大聲公 / C系學姊,
    // isQuestGiver=true) STAY scattered across 羅馬廣場 — off the central
    // gate→圖書館 lane so they don't block it (player request, a deliberate
    // override of their narrative 場景) — where the player heads after the
    // 操場 lap. Their `!` indicators reveal SEQUENTIALLY (Ch3IndicatorVisible:
    // A until traded, then B, then C) instead of all at once, and the chain
    // must run A→B→C (TryAdvanceCh3Trade redirects an out-of-order talk).
    // All coords mask-verified STRICTLY walkable + flood-reachable
    // (map_registry.py; test_spawn_reachability).
    static const std::vector<NpcSpawn> kChapter3 = {
        {nccu::engine::math::Vec2{1620,  920}, "resources/assets/sprites/school_uniform_3/male_02.png",
         "victim", false},
        {nccu::engine::math::Vec2{1430,  910}, "resources/assets/sprites/npc/suit_senior.png",
         "suit_senior", false},
        {nccu::engine::math::Vec2{ 900,  545}, "resources/assets/sprites/school_uniform_3/female_03.png",
         "bookworm", false},
        {nccu::engine::math::Vec2{1530,  930}, "resources/assets/sprites/npc/ta.png",
         "ta", false},
        {nccu::engine::math::Vec2{1100,  860}, "resources/assets/sprites/npc/shop_auntie.png",
         "shop_auntie", false},
        {nccu::engine::math::Vec2{ 980, 1000}, "resources/assets/sprites/npc/shop_auntie.png",
         "vendor_sausage_a", true},
        {nccu::engine::math::Vec2{1150, 1010}, "resources/assets/sprites/school_uniform_3/male_02.png",
         "loudspeaker_b", true},
        {nccu::engine::math::Vec2{1060, 1120}, "resources/assets/sprites/school_uniform_3/female_01.png",
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
    //
    // Placement now matches the chapter4.md 場景 lines:
    //   • 西裝學長 → 行政大樓 門口 (1220,775): 「場景：行政大樓門口
    //     (Flag_HelpedSenior = true 路徑)」.
    //   • 助教 → 研究大樓 走廊 (980,1560): 「場景：研究大樓走廊中段」 (just
    //     east of the 研究大樓 rect 590,1453,367x255).
    //   • 苦主 → 正門 旁 (1010,1700): 「場景：正門旁的廊柱下」 (NE of the
    //     正門 rect 887,1722,101x116; sits in the south-wall gap, reachable).
    //   • 福利社阿姨 → 樂活小舖 (1560,1560): 「場景：樂活小舖，期末考週備品」.
    //   • 學霸 → 中正圖書館 考場外 (900,545): 「場景：中正圖書館考場外走廊」.
    // All isQuestGiver=false (the finale is gate-driven). All coords
    // mask-verified STRICTLY walkable + flood-reachable.
    static const std::vector<NpcSpawn> kChapter4 = {
        {nccu::engine::math::Vec2{1010, 1700}, "resources/assets/sprites/school_uniform_3/male_02.png",
         "victim", false},
        {nccu::engine::math::Vec2{1220,  775}, "resources/assets/sprites/npc/suit_senior.png",
         "suit_senior", false},
        {nccu::engine::math::Vec2{ 900,  545}, "resources/assets/sprites/school_uniform_3/female_03.png",
         "bookworm", false},
        {nccu::engine::math::Vec2{ 980, 1560}, "resources/assets/sprites/npc/ta.png",
         "ta", false},
        {nccu::engine::math::Vec2{1560, 1560}, "resources/assets/sprites/npc/shop_auntie.png",
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
        case SemesterState::Ending_D:           return kEndingA;  // no roster (G1)
        case SemesterState::Ending_C:           return kEndingC;
    }
    return kInterlude;  // unreachable; keeps non-void paths total
}

} // namespace nccu

#endif // CHAPTER_SPAWNS_H_
