#ifndef CHAPTER_QUEST_ITEMS_H_
#define CHAPTER_QUEST_ITEMS_H_
#include "quest/Chapter1Quest.h"
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
// precedent); this table is for chapter-scoped, roster-swept quest items.
// Ch1 = the 苦主's transparent umbrella (the 善有善報 redesign): the
// findable item the 西裝學長 dropped near 集英樓, picked up to set
// Flag_HasVictimUmbrella, then carried BACK to the victim (the actual
// chapter clear is TryReturnVictimUmbrella's grant, NOT this pickup —
// grabbing it off the ground does not clear Ch1). It is spawned at chapter
// entry through the shared SpawnChapterQuestItems helper (unlike the Ch2
// notes, which are deferred until the 學霸 is woken). Ch2 = the 3 散落筆記;
// whoever collects all three earns 學霸 (b)'s `// karma +3` (chapter2.md)
// via QuestFlagPickup's completion hook, and 圖書館管理員 (b) then points
// to 羅馬廣場.
struct QuestItemPlacement {
    nccu::gfx::Vec2          pos;
    std::string              flag;
    std::string              message;          // ShowMessage on pickup
    std::vector<std::string> completionFlags;  // all set => grant karma
    int                      completionKarma;
    // Optional COUNT-BASED lines: when non-empty, the on-pickup message is
    // chosen by HOW MANY completionFlags the player now holds (1st/2nd/3rd
    // collected), NOT by which item. Lets the 3 notes read "1st / 2nd /
    // last" in ANY pickup order. Empty -> the single `message` is used.
    std::vector<std::string> countMessages = {};
};

inline const std::vector<QuestItemPlacement>&
ChapterQuestItems(SemesterState state) {
    // The 3 notes share the same completion set + karma; whichever is
    // collected LAST sees every flag set and grants 學霸 (b) +3 exactly
    // once (QuestFlagPickup::OnPickup — the earlier siblings see a
    // missing flag and skip, and a collected sibling has deactivated).
    //
    // They are DEFERRED-spawned (World::MaybeSpawnChapter2Notes), so they
    // only appear AFTER the player wakes the 學霸 and he asks for them —
    // never at chapter entry. Coordinates are now scattered across THREE
    // different campus areas so collecting them is roam-worthy (not a
    // clustered library tour): note1 NW near 法學院 (450,850), note2 SE
    // near 集英樓/新聞館 (1400,1250), note3 S-central near 校友服務中心/正門
    // (1040,1640). All mask-verified STRICTLY walkable AND flood-REACHABLE
    // from the plaza/學霸 (the full bookworm→note→note→note→bookworm loop
    // checked via .claude/tools/map_registry.py --route; an earlier east
    // pick at (1480,1120) was walkable but walled-off / unreachable, so it
    // was moved here).
    //
    // Messages are COUNT-BASED (kNoteMsgs), chosen by how many notes the
    // player now holds — 1st/2nd/last — NOT by which note. So picking
    // note3 first correctly prints the "1st found" line, fixing the old
    // identity-keyed bug where grabbing note3 first wrongly announced the
    // "last page". `message` is kept as a sane fallback but is unused
    // while kNoteMsgs is supplied.
    static const std::vector<std::string> kNoteSet = {
        kFlagFoundNote1, kFlagFoundNote2, kFlagFoundNote3};
    static const std::vector<std::string> kNoteMsgs = {
        "撿到一頁學霸的筆記。還有兩頁散在別處。",
        "第二頁筆記到手——空白處寫著「從現在開始」。",
        "最後一頁找齊了。三頁都在手上了，回去找學霸。",
    };
    static const std::vector<QuestItemPlacement> kChapter2 = {
        {{ 450.0f,  850.0f}, kFlagFoundNote1,
         "撿到一頁學霸的筆記。", kNoteSet, 3, kNoteMsgs},
        {{1400.0f, 1250.0f}, kFlagFoundNote2,
         "撿到一頁學霸的筆記。", kNoteSet, 3, kNoteMsgs},
        {{1040.0f, 1640.0f}, kFlagFoundNote3,
         "撿到一頁學霸的筆記。", kNoteSet, 3, kNoteMsgs},
    };
    // Ch1 = the 苦主's transparent umbrella. Single flag, no completion set
    // (karma 0 — the +5 善行 lives on the (b) 承諾 choice and the grant is
    // its own payoff, not this pickup). Placed at (1700,1610), just SOUTH of
    // 集英樓 (rect 1524,1353,224x192) and ~94 px from the 西裝學長
    // (1620,1560): the senior "拿著透明傘往集英樓方向跑" (chapter1.md 苦主
    // (a)), so it reads as the umbrella he dropped there. Chosen for a wide
    // walkable clearance (r≈96 open square) so the harness axis-driver
    // routes to it robustly via the clear east corridor (x≈1744-1752),
    // avoiding the thin diagonal slots west of 集英樓 that soft-lock a pure
    // Manhattan goto (BUGLEDGER I7's routing model). Mask-verified STRICTLY
    // walkable AND flood-reachable from the 苦主 @綜合院館.
    static const std::vector<QuestItemPlacement> kChapter1 = {
        {{1700.0f, 1610.0f}, kFlagHasVictimUmbrella,
         "撿到一把眼熟的透明傘，傘柄上有 A 君的名字貼紙。", {}, 0, {}},
    };
    static const std::vector<QuestItemPlacement> kNone;

    switch (state) {
        case SemesterState::Chapter1_AddDrop:   return kChapter1;
        case SemesterState::Chapter2_Midterms:  return kChapter2;
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
