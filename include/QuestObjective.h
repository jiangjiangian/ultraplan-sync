#ifndef QUEST_OBJECTIVE_H_
#define QUEST_OBJECTIVE_H_
#include "Player.h"
#include "SemesterState.h"
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
                return "目標：找到那位在雨中淋濕的同學，聽他說發生了什麼事";
            if (!player.HasUmbrella())
                return "目標：在校園裡找回苦主的傘——找到後靠近按 E 撿起";
            return "目標：傘到手了，回去把傘交還給苦主";
        case SemesterState::Interlude_Market:
            return "目標：在羅馬廣場市集採買下一章要用的道具，再往南離開";
        case SemesterState::Chapter2_Midterms:
            return "目標：到圖書館找管理員問線索，設法喚醒學霸";
        case SemesterState::Chapter3_SportsDay:
            return "目標：校慶運動會——完成物物交換，取回真傘";
        case SemesterState::Chapter4_Finals:
            return "目標：期末考終焉——面對你的選擇";
        case SemesterState::Ending_A:
        case SemesterState::Ending_B:
        case SemesterState::Ending_C:
            return std::string{};
    }
    return std::string{};
}

} // namespace nccu

#endif // QUEST_OBJECTIVE_H_
