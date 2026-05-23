#ifndef QUEST_OBJECTIVE_H_
#define QUEST_OBJECTIVE_H_
#include "entities/Player.h"
#include "quest/Chapter2Quest.h"
#include "state/SemesterState.h"
#include <string>

// One short line of "what to do next", shown at the top of the screen so
// the player is never lost (the playtest's "沒有指引" / "任務指引在最上
// 方"). Deliberately a FINITE map of (state, the one gating flag that
// matters in that state) → text — not a reactive read of every flag
// combination. Chapter 1 has three beats because the umbrella pickup is
// now quest-gated (see TransparentUmbrella): tell the player to meet the
// 苦主 first, then to search, then to deliver. Endings show nothing (the
// View replaces the world with the ending card anyway).
namespace nccu {

inline std::string CurrentObjective(SemesterState state,
                                    const Player& player) {
    switch (state) {
        case SemesterState::Chapter1_AddDrop:
            if (!player.HasFlag("Flag_PromisedVictim"))
                return "目標：在校門口指南路（你出生點旁）找苦主同學，"
                       "靠近按 E 跟他對話、答應幫他找傘";
            if (!player.HasUmbrella())
                return "目標：傘被吹到校園中央（羅馬廣場南側、四維道一帶）"
                       "——走過去靠近傘按 E 撿起";
            return "目標：拿著傘回校門口指南路，交還給苦主";
        case SemesterState::Interlude_Market:
            return "目標：在羅馬廣場市集向攤販按 E 採買，"
                   "逛完往南（校門口方向）離開";
        case SemesterState::Chapter2_Midterms:
            if (!player.HasFlag("Flag_Bookworm"))
                return "目標：先到中正圖書館找管理員問線索，"
                       "再去羅馬廣場雕像下喚醒學霸";
            if (!nccu::Chapter2NotesComplete(player))
                return "目標：撿回學霸散落的三頁筆記"
                       "（分散在校園不同角落）";
            return "目標：帶著三頁筆記回羅馬廣場找學霸換回你的傘";
        case SemesterState::Chapter3_SportsDay:
            return "目標：先繞操場跑一圈參加校慶，再到羅馬廣場找 ABC "
                   "三系換情報、取回真傘";
        case SemesterState::Chapter4_Finals:
            return "目標：期末考終焉——回集英樓面對你的最終選擇";
        case SemesterState::Ending_A:
        case SemesterState::Ending_B:
        case SemesterState::Ending_C:
            return std::string{};
    }
    return std::string{};
}

} // namespace nccu

#endif // QUEST_OBJECTIVE_H_
