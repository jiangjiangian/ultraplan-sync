#ifndef NPC_SPAWNS_H_
#define NPC_SPAWNS_H_
#include "engine/math/Vec2.h"
#include <string>
#include <string_view>
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
        // 學霸 — at the 中正圖書館 (rect 698,254,382x255) front, matching
        // chapter1.md「場景：中正圖書館 1 樓入口附近，抱著一疊厚厚的講義」.
        // (820,545) sits just south of the library rect (bottom y=509) on the
        // entrance apron, mask-verified STRICTLY walkable (100%, all 4
        // neighbours 100%) and flood-reachable from the (500,1860) spawn.
        {gfx::Vec2{820, 545}, "resources/assets/sprites/school_uniform_3/female_03.png",
         "bookworm", false},
        // 助教 — at the 行政大樓 (rect 1057,598,240x156) front, matching
        // chapter1.md「場景：行政大樓 1 樓走廊，手持一疊加退選資料夾」.
        // (1220,775) is just south of the rect (bottom y=754) on the open
        // apron east of the central campus, mask-verified STRICTLY walkable
        // (100%, all 4 neighbours 100%) and flood-reachable.
        {gfx::Vec2{1220, 775}, "resources/assets/sprites/npc/ta.png",
         "ta", false},
        // 福利社阿姨 — at 樂活小舖 (rect 1388,1578,309x205), matching
        // chapter1.md「場景：樂活小舖內，正在整理飲料架」. (1560,1560) sits just
        // north of the building's north wall (y=1578), centred on the shop
        // front, mask-verified STRICTLY walkable (100%, all 4 neighbours
        // 100%) and flood-reachable; it stays 122 px clear of the Ch1 spine
        // route's east corridor so the I7 reachability spine is unaffected.
        {gfx::Vec2{1560, 1560}, "resources/assets/sprites/npc/shop_auntie.png",
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

// G-1: the Ch1 加退選搶課 crowd — a batch of ambient WANDERING students
// scattered across the central campus to convey the course-grab rush. Each
// is `{pos, sprite, "", false, /*wander*/true}` exactly like
// AmbientStudentSpawns: no npcId (no dialog, no `!`), non-blocking
// (NPC::BlocksMovement is false for wander_), so they random-walk + animate
// (UI-A) and the player walks straight through them. Sprites reuse the
// already-cached Pipoya school-uniform subset (cheap — no new texture). All
// coords mask-verified STRICTLY walkable (100%, all 4 neighbours 100%) and
// flood-reachable from the (500,1860) spawn (.claude/tools/map_registry.py),
// and ALL sit on the central campus (y≈860..1180) — well clear of the Ch1
// 苦主/學長/助教/阿姨/學霸 quest anchors AND the south Zhinan-Rd corridor the
// smoke-walk script traverses, so they never block a quest interaction nor
// perturb a script that doesn't talk to them. World::SpawnChapterNpcs wires
// EnableWander + SetWanderMask for each (Ch1 only; swept on chapter exit by
// the chapterRoster_ sweep, exactly like the Ch3 操場 crowd).
inline const std::vector<NpcSpawn>& Chapter1CrowdSpawns() {
    static const std::vector<NpcSpawn> kAll = {
        {gfx::Vec2{1000,  900}, "resources/assets/sprites/school_uniform_3/male_04.png",   "", false, true},
        {gfx::Vec2{ 900,  950}, "resources/assets/sprites/school_uniform_3/female_04.png", "", false, true},
        {gfx::Vec2{ 960, 1080}, "resources/assets/sprites/school_uniform_3/male_05.png",   "", false, true},
        {gfx::Vec2{1140, 1140}, "resources/assets/sprites/school_uniform_3/female_05.png", "", false, true},
        {gfx::Vec2{ 840,  860}, "resources/assets/sprites/school_uniform_3/male_07.png",   "", false, true},
        {gfx::Vec2{1240,  980}, "resources/assets/sprites/school_uniform_3/female_06.png", "", false, true},
        {gfx::Vec2{1000, 1180}, "resources/assets/sprites/school_uniform_3/male_08.png",   "", false, true},
        {gfx::Vec2{ 880, 1120}, "resources/assets/sprites/school_uniform_3/female_07.png", "", false, true},
    };
    return kAll;
}

// G-2: Ch1 stationary FLAVOR NPCs — a few standing students who gripe about
// 搶課 / campus chatter on E. STATIONARY (wander=false → solid, the player
// bumps off them) and NON-quest (isQuestGiver=false → no `!`,
// Ch1IndicatorVisible's fall-through returns their false bit). Each carries a
// distinct npcId whose chapter1.md「## NPC：…」(a) section is its line pool.
// World::SpawnChapterNpcs loads that pool into the NPC's dialogLines_ via
// NPC::LoadDialog at spawn; GameController's E-interact routes a flavor npcId
// to NPC::Interact(), which cycles the pool one line per talk (currentLine
// Index_ advances modulo the pool size) — a DETERMINISTIC, reproducible
// "random" pick (no std::rand / time seed; the harness stays byte-identical).
// The flavor path NEVER runs the spine hooks (TryReturnVictimUmbrella etc.)
// and NEVER sets a quest flag, so the hard-gated 苦主→學長→苦主 spine is
// untouched. Coords sit on the central campus, ≥90 px from every quest anchor
// and far from the smoke-walk corridor (so a blocking flavor NPC can't deflect
// the scripted player). Mask-verified walkable + flood-reachable; swept on
// chapter exit with the rest of the Ch1 roster.
inline const std::vector<NpcSpawn>& Chapter1FlavorSpawns() {
    static const std::vector<NpcSpawn> kAll = {
        {gfx::Vec2{1010, 1100}, "resources/assets/sprites/school_uniform_3/male_09.png",   "ch1_flavor_grab", false, false},
        {gfx::Vec2{ 900, 1000}, "resources/assets/sprites/school_uniform_3/female_08.png", "ch1_flavor_rain", false, false},
        {gfx::Vec2{1150, 1050}, "resources/assets/sprites/school_uniform_3/male_11.png",   "ch1_flavor_bag",  false, false},
    };
    return kAll;
}

// G-2: is this npcId one of the Ch1 stationary flavor NPCs? Single source of
// truth (derived from Chapter1FlavorSpawns so the set never drifts). The
// GameController E-interact dispatch consults this to route a flavor NPC to
// the cycling NPC::Interact() path INSTEAD of the spine hooks + DialogOpener
// — making the "sets no quest flag" guarantee structural (the flavor NPC
// never even reaches a quest hook). Empty / unknown id -> false, so quest /
// ambient NPCs are unaffected.
[[nodiscard]] inline bool IsChapter1FlavorNpc(std::string_view npcId) noexcept {
    if (npcId.empty()) return false;
    for (const auto& s : Chapter1FlavorSpawns())
        if (npcId == s.npcId) return true;
    return false;
}

} // namespace nccu

#endif // NPC_SPAWNS_H_
