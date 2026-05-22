#ifndef CHAPTER_QUEST_ITEMS_H_
#define CHAPTER_QUEST_ITEMS_H_
#include "quest/Chapter2Quest.h"
#include "state/SemesterState.h"
#include "gfx/Vec2.h"
#include <string>
#include <vector>

namespace nccu {

// Per-state quest-item placements — the narrative sibling of
// ChapterPickups (cash) / ChapterVendors (stalls) / ChapterNpcSpawns.
// World spawns these as QuestFlagPickup in SpawnChapterNpcs and tracks
// them in the chapter roster, so an uncollected note is swept on the
// next state change (notes never leak past their chapter). Ch1's 申請書
// is NOT here — it stays ctor-spawned (a permanent Ch1 object, its own
// precedent); this table is only for chapter-scoped, roster-swept quest
// items. Ch2 = the 3 散落筆記; whoever collects all three earns 學霸
// (b)'s `// karma +3` (chapter2.md) via QuestFlagPickup's completion
// hook, and 圖書館管理員 (b) then points to 羅馬廣場.
struct QuestItemPlacement {
    nccu::gfx::Vec2          pos;
    std::string              flag;
    std::string              message;          // ShowMessage on pickup
    std::vector<std::string> completionFlags;  // all set => grant karma
    int                      completionKarma;
};

inline const std::vector<QuestItemPlacement>&
ChapterQuestItems(SemesterState state) {
    // The 3 notes share the same completion set + karma; whichever is
    // collected LAST sees every flag set and grants 學霸 (b) +3 exactly
    // once (QuestFlagPickup::OnPickup — the earlier siblings see a
    // missing flag and skip, and a collected sibling has deactivated).
    // Coordinates now trace the gate→廣場→圖書館 climb the player walks
    // anyway: note1 mid-campus (900,1500), note2 just south of 羅馬廣場
    // (1088,1230 — passing the slumped 學霸 at the statue), note3 at the
    // 中正圖書館 front (920,620) so the last page lands the player AT the
    // 管理員 desk, whose (b) line then points back to the plaza. Replaces
    // the old all-south y≈1430/1850 cluster that toured nothing. All
    // mask-verified walkable via .claude/tools/map_registry.py.
    static const std::vector<std::string> kNoteSet = {
        kFlagFoundNote1, kFlagFoundNote2, kFlagFoundNote3};
    static const std::vector<QuestItemPlacement> kChapter2 = {
        {{ 900.0f, 1500.0f}, kFlagFoundNote1,
         "撿到一頁學霸的筆記。字跡工整，但順序不對。", kNoteSet, 3},
        {{1088.0f, 1230.0f}, kFlagFoundNote2,
         "又一頁筆記——空白處寫著「期末準備就從現在開始」。", kNoteSet, 3},
        {{ 920.0f,  620.0f}, kFlagFoundNote3,
         "最後一頁找齊了。管理員說他在外面——大雨裡。", kNoteSet, 3},
    };
    static const std::vector<QuestItemPlacement> kNone;

    switch (state) {
        case SemesterState::Chapter2_Midterms:  return kChapter2;
        case SemesterState::Chapter1_AddDrop:
        case SemesterState::Interlude_Market:
        case SemesterState::Chapter3_SportsDay:
        case SemesterState::Chapter4_Finals:
        case SemesterState::Ending_A:
        case SemesterState::Ending_B:
        case SemesterState::Ending_C:
            return kNone;
    }
    return kNone;  // unreachable; keeps non-void paths total
}

} // namespace nccu

#endif // CHAPTER_QUEST_ITEMS_H_
