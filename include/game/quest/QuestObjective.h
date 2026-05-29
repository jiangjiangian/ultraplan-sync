#ifndef QUEST_OBJECTIVE_H_
#define QUEST_OBJECTIVE_H_
#include "game/entities/Player.h"
#include "game/quest/Chapter1Quest.h"
#include "game/quest/Chapter2Quest.h"
#include "game/state/SemesterState.h"
#include <string>
#include <string_view>
#include <vector>

/**
 * @file QuestObjective.h
 * @brief 螢幕最上方那一行「接下來該做什麼」的任務指引，確保玩家不會迷失方向。
 *
 * 刻意設計成「(章節狀態, 該狀態下唯一關鍵的擋路旗標) → 文字」的有限對照，而
 * 非對所有旗標組合做反應式判讀。Chapter 1 有三段善有善報節拍：先遇見同樣丟了
 * 傘的苦主並答應幫忙，再到場景中找回「他的」傘，最後把傘帶「回去」交給他——他
 * 才把「你的」真傘還你、章節結束（TryReturnVictimUmbrella）。撿地上的傘並不會
 * 結束章節。結局狀態不顯示指引（View 此時已用結局卡片取代整個世界）。
 *
 * 每行指引被抽成具名 string_view 常數，目的有二：(a) CurrentObjective 據以
 * 拼接、(b) QuestObjectiveStrings() 列舉「每一行」供字形覆蓋掃描使用——指引文
 * 字曾出現缺字（豆腐方塊）。任何此處用到、卻未烘進字型的字形都會「使編譯失
 * 敗」，避免未來改指引時悄悄重新引入缺字。
 */
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
    "目標：期末考終焉——自由探索校園，尋找屬於你的結局";
}  // namespace objective

/**
 * @brief 取得目前章節狀態下該顯示的單行任務指引。
 * @param state  目前的學期章節狀態。
 * @param player 玩家（用於判讀關鍵擋路旗標決定走到第幾段）。
 * @return 對應的指引文字；結局狀態回傳空字串。
 */
inline std::string CurrentObjective(SemesterState state,
                                    const Player& player) {
    using namespace nccu::objective;
    switch (state) {
        case SemesterState::Chapter1_AddDrop:
            if (!player.HasFlag(kFlagPromisedVictim))
                return std::string{kCh1MeetVictim};
            if (!player.HasFlag(nccu::kFlagHasVictimUmbrella))
                return std::string{kCh1FindUmbrella};
            return std::string{kCh1ReturnUmbrella};
        case SemesterState::Interlude_Market:
            return std::string{kInterludeMarket};
        case SemesterState::Chapter2_Midterms:
            if (!player.HasFlag(kFlagBookworm))
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

/**
 * @brief 列出 HUD 可能顯示的每一行任務指引，供字形覆蓋掃描使用。
 * @return 全部指引文字字串的向量。
 *
 * 純資料，不需 Player（直接列舉上面那些常數）。
 */
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
