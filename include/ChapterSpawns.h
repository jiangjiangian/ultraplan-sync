#ifndef CHAPTER_SPAWNS_H_
#define CHAPTER_SPAWNS_H_
#include "NpcSpawns.h"
#include "SemesterState.h"
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
    static const std::vector<NpcSpawn> kChapter2;      // TODO(S5c): chapter roster
    static const std::vector<NpcSpawn> kChapter3;      // TODO(S5d): chapter roster
    static const std::vector<NpcSpawn> kChapter4;      // TODO(S5e): chapter roster
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
