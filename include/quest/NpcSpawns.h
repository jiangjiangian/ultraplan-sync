#ifndef NPC_SPAWNS_H_
#define NPC_SPAWNS_H_
#include "gfx/Vec2.h"
#include <string>
#include <vector>

namespace nccu {

// The 5 NPC archetypes from the design doc, each parked at the south
// edge of their anchor building's trigger rect so they read as standing
// at the entrance. Sprite paths point at the curated Pipoya subset
// under resources/assets/sprites/ (see sprites/ATTRIBUTIONS.md). Each
// NPC sits just OUTSIDE its anchor building's collision rect (the
// trigger-rect shrunk by main.cpp's kBuildingInset) so a player walking
// up to talk does not get pushed away by the wall.
//
// `npcId` keys the runtime dialog content — the opener pulls the
// per-(npcId, SemesterState) lines at talk time, so dialog is no longer
// hard-coded here. "" = no dialog (ambient pedestrians).
struct NpcSpawn {
    gfx::Vec2   pos;
    const char* spritePath;
    const char* npcId;
    bool        isQuestGiver;
    bool        wander = false;
};

inline const std::vector<NpcSpawn>& DefaultNpcSpawns() {
    static const std::vector<NpcSpawn> kAll = {
        // 苦主 — Ch1 quest-giver, now at 綜合院館 (rect 1681,677,371x326),
        // where the 善有善報 redesign opens: the player's own透明傘 vanished
        // from the 綜院 1 樓傘架 (chapter1.md 開場), and the victim — who
        // ALSO lost his there — is found just outside the building's SW
        // corner. (1660,1010) is mask-verified STRICTLY walkable (100%, all
        // 4 neighbours 100%, in no building trigger) and flood-reachable
        // from the (500,1860) spawn via the gap column (map_registry.py
        // --route). Quest-giver `!` marks him as the first stop.
        {gfx::Vec2{1660, 1010}, "resources/assets/sprites/school_uniform_3/male_02.png",
         "victim", true},
        // 西裝學長 — at 集英樓 (rect 1524,1353,224x192), the building the
        // 苦主 says he "拿著透明傘往集英樓方向跑" toward (chapter1.md 苦主
        // (a)). (1620,1560) is just south of the 集英樓 rect, mask-verified
        // STRICTLY walkable (100%, all 4 neighbours 100%) and flood-
        // reachable; the victim's-umbrella pickup sits nearby at (1450,1450).
        {gfx::Vec2{1620, 1560}, "resources/assets/sprites/npc/suit_senior.png",
         "suit_senior", false},
        // 學霸 — on the central umbrella strip (same row as the umbrellas),
        // west of the planter prop so the spawn box stays walkable.
        {gfx::Vec2{480, 1280}, "resources/assets/sprites/school_uniform_3/female_03.png",
         "bookworm", false},
        // 助教 — Zhinan Rd, SE corner near the 學思樓 approach, nudged off
        // the baked perimeter/prop collision so the player can reach them.
        {gfx::Vec2{1706, 1766}, "resources/assets/sprites/npc/ta.png",
         "ta", false},
        // 福利社阿姨 — outside 樂活小舖, inside its trigger but clear of the wall.
        {gfx::Vec2{460, 1500}, "resources/assets/sprites/npc/shop_auntie.png",
         "shop_auntie", false},
    };
    return kAll;
}

// Ambient pedestrians — students drifting along Zhinan Rd and the
// central strip to make the山下 campus feel populated. No dialog (the
// player walks straight past), non-blocking, wander=true so World wires
// EnableWander(). Spawn points sit on open road clear of footprints.
inline const std::vector<NpcSpawn>& AmbientStudentSpawns() {
    static const std::vector<NpcSpawn> kAll = {
        {gfx::Vec2{ 700, 1880}, "resources/assets/sprites/school_uniform_3/male_01.png",   "", false, true},
        {gfx::Vec2{1080, 1870}, "resources/assets/sprites/school_uniform_3/female_01.png", "", false, true},
        {gfx::Vec2{1500, 1880}, "resources/assets/sprites/school_uniform_3/male_03.png",   "", false, true},
        // female_02 / male_02 were embedded in 商學院 / 資訊大樓 wall bases
        // (read in-game as "someone stuck in the wall"); moved onto the
        // open central campus, verified walkable + reachable.
        {gfx::Vec2{ 980, 1640}, "resources/assets/sprites/school_uniform_3/female_02.png", "", false, true},
        {gfx::Vec2{1450, 1620}, "resources/assets/sprites/school_uniform_3/male_02.png",   "", false, true},
        {gfx::Vec2{ 980, 1500}, "resources/assets/sprites/school_uniform_3/female_03.png", "", false, true},
    };
    return kAll;
}

} // namespace nccu

#endif // NPC_SPAWNS_H_
