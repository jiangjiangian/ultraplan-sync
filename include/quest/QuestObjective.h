#ifndef QUEST_OBJECTIVE_H_
#define QUEST_OBJECTIVE_H_
#include "entities/Player.h"
#include "quest/Chapter1Quest.h"
#include "quest/Chapter2Quest.h"
#include "state/SemesterState.h"
#include <string>
#include <string_view>
#include <vector>

// One short line of "what to do next", shown at the top of the screen so
// the player is never lost (the playtest's "沒有指引" / "任務指引在最上
// 方"). Deliberately a FINITE map of (state, the one gating flag that
// matters in that state) → text — not a reactive read of every flag
// combination. Chapter 1 has three 善有善報 beats: meet the 苦主 (who
// also lost his umbrella) and promise to help, then find HIS umbrella out
// in the world, then carry it BACK to him — he then returns YOUR真傘 and
// the chapter clears (TryReturnVictimUmbrella). The chapter does NOT clear
// on grabbing an umbrella off the ground. Endings show nothing (the View
// replaces the world with the ending card anyway).
//
// 5c/T5: the objective LINES are pulled out as named string_view constants
// so (a) CurrentObjective stitches them and (b) QuestObjectiveStrings()
// enumerates EVERY one for the glyph-coverage scan — the owner reported `?`
// (tofu) in objective text (綜合院館 / 集英樓 / 羅馬廣場 / 操場 / 校慶 /
// 期末考終焉…). The scan FAILS the build on any glyph here not baked into
// gfx::Font.h, so a future objective edit can't silently reintroduce tofu.
namespace nccu {

namespace objective {
inline constexpr std::string_view kCh1MeetVictim =
    "目標：到綜合院館（地圖東北）找蹲在傘架旁的苦主同學，"
    "靠近按 E 對話、答應幫他找傘";
inline constexpr std::string_view kCh1FindUmbrella =
    "目標：苦主的傘被西裝學長帶往集英樓一帶"
    "——過去找到那把透明傘按 E 撿起";
inline constexpr std::string_view kCh1ReturnUmbrella =
    "目標：帶著苦主的傘回綜合院館交給他（他會把你的真傘還你）";
inline constexpr std::string_view kInterludeMarket =
    "目標：在羅馬廣場市集向攤販按 E 採買，"
    "逛完往南（校門口方向）離開";
inline constexpr std::string_view kCh2FindBookworm =
    "目標：先到中正圖書館找管理員問線索，"
    "再去羅馬廣場雕像下喚醒學霸";
inline constexpr std::string_view kCh2FindNotes =
    "目標：撿回學霸散落的三頁筆記"
    "（分散在校園不同角落）";
inline constexpr std::string_view kCh2ReturnNotes =
    "目標：帶著三頁筆記回羅馬廣場找學霸換回你的傘";
inline constexpr std::string_view kCh3SportsDay =
    "目標：先繞操場跑一圈參加校慶，再到羅馬廣場找 ABC "
    "三系換情報、取回真傘";
inline constexpr std::string_view kCh4Finals =
    "目標：期末考終焉——回集英樓面對你的最終選擇";
}  // namespace objective

inline std::string CurrentObjective(SemesterState state,
                                    const Player& player) {
    using namespace nccu::objective;
    switch (state) {
        case SemesterState::Chapter1_AddDrop:
            if (!player.HasFlag("Flag_PromisedVictim"))
                return std::string{kCh1MeetVictim};
            if (!player.HasFlag(nccu::kFlagHasVictimUmbrella))
                return std::string{kCh1FindUmbrella};
            return std::string{kCh1ReturnUmbrella};
        case SemesterState::Interlude_Market:
            return std::string{kInterludeMarket};
        case SemesterState::Chapter2_Midterms:
            if (!player.HasFlag("Flag_Bookworm"))
                return std::string{kCh2FindBookworm};
            if (!nccu::Chapter2NotesComplete(player))
                return std::string{kCh2FindNotes};
            return std::string{kCh2ReturnNotes};
        case SemesterState::Chapter3_SportsDay:
            return std::string{kCh3SportsDay};
        case SemesterState::Chapter4_Finals:
            return std::string{kCh4Finals};
        case SemesterState::Ending_A:
        case SemesterState::Ending_B:
        case SemesterState::Ending_D:
        case SemesterState::Ending_C:
            return std::string{};
    }
    return std::string{};
}

// 5c/T5 — every objective LINE the HUD can show, for the glyph-coverage
// scan. Pure data; no Player needed (enumerates the constants directly).
[[nodiscard]] inline std::vector<std::string> QuestObjectiveStrings() {
    using namespace nccu::objective;
    return {
        std::string{kCh1MeetVictim},   std::string{kCh1FindUmbrella},
        std::string{kCh1ReturnUmbrella}, std::string{kInterludeMarket},
        std::string{kCh2FindBookworm}, std::string{kCh2FindNotes},
        std::string{kCh2ReturnNotes},  std::string{kCh3SportsDay},
        std::string{kCh4Finals},
    };
}

} // namespace nccu

#endif // QUEST_OBJECTIVE_H_
