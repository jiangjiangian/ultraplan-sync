#include "game/state/EndingGate.h"
#include "game/quest/Flags.h"
#include "game/state/ChapterToast.h"
#include "engine/events/EventBus.h"
#include "game/entities/Player.h"
#include "game/state/SemesterStateMachine.h"
#include "game/state/SemesterState.h"
#include "game/dialog/DialogState.h"

/**
 * @file EndingGate.cpp
 * @brief 結局閘門判定實作：四種結局的優先序、對話延後與卡死防護的不變式。
 */

namespace nccu {

// 四種結局在此解析，全程僅在 Chapter4_Finals 生效。判定優先序 A→B→D→C：誠實高
// 業力的真結局優先，其次詛咒／墮落路線，再者選擇體諒但未達 A 的苦甜「風雨同行」，
// 最後是破財消災／平穩的預設。每個分支命中後都會關閉殘留對話並 return，確保單次
// 輪詢只觸發一個結局。第一章向阿姨買傘僅是純敘事鋪陳、不設旗標；真正觸發結局 C
// 的是第四章集英樓攤販的購買（設立 Flag_BoughtUglyUmbrella）。
void CheckEndingGates(EventBus& bus, Player& player,
                      SemesterStateMachine& semester, DialogState& dialog) {
    if (semester.Current() != SemesterState::Chapter4_Finals) return;

    // 不變式：對話／內心獨白仍在畫面上時不得解析結局。當決定性旗標設立的同一幀
    // 還顯示著收尾台詞（助教終局的後續台詞，或購買／拾取／認領時開啟的自白）時，
    // 須先讓玩家讀完，等對話框關閉後的後續輪詢才轉移結局。否則選項確認的同一幀就
    // Transition()+Close()，會吞掉玩家還沒看到的後續台詞。決定性旗標都是持久的，
    // 因此延後只會推遲、絕不會漏掉判定；GameController 每個非對話幀都重新輪詢，
    // 旁白一關閉即觸發。
    if (dialog.Active()) return;

    // 結局 A：業力 > 80、且持有第四章重新尋回的真傘、且對助教選擇了體諒
    // （Flag_ConsoledTA）。Flag_HasTrueUmbrella 專屬於真傘且已於進入第四章時清除，
    // 故此處成立即代表「在第四章重新取得」。
    if (player.GetKarma() > 80 &&
        player.HasFlag(kFlagHasTrueUmbrella) &&
        player.HasFlag(kFlagConsoledTA)) {
        semester.Transition(SemesterState::Ending_A);
        // 連終局畫面也先閃一則過場提示：否則進入 Ending_A 的這次轉移在外部紀錄上
        // 與「什麼都沒發生的一幀」難以區分。
        PublishChapterTransitionToast(bus, SemesterState::Ending_A);
        dialog.Close();
        return;
    }

    // 結局 B：詛咒傘路線（第一章鋪陳的旗標延續至此）、業力跌破零，或冷淡終局——
    // 玩家走到助教的結算並選擇「質問／強硬索回」而非體諒（設立
    // Flag_TaFinaleChoiceMade 但未設 Flag_ConsoledTA）。在高潮處、面對睡眠不足且
    // 主動致歉的助教仍拒絕體諒，正是「你成為了你曾經最討厭的那種人」的主題。原始
    // 設計的 B 觸發條件（詛咒傘或業力<0）在此擴充出冷淡終局這一支：否則冷淡選擇會
    // 漏出所有閘門，而終局選單又會以 Flag_TaFinaleChoiceMade 自我鎖定，導致遊戲在
    // 第四章永久卡死（無結局可達）。
    const bool coldFinale = player.HasFlag(kFlagTaFinaleChoiceMade) &&
                            !player.HasFlag(kFlagConsoledTA);
    if (player.HasFlag(kFlagTookCursedUmbrella) ||
        player.GetKarma() < 0 || coldFinale) {
        semester.Transition(SemesterState::Ending_B);
        PublishChapterTransitionToast(bus, SemesterState::Ending_B);
        dialog.Close();
        return;
    }

    // 結局 D：風雨同行。走到此分支代表玩家選擇了體諒（Flag_ConsoledTA），卻未達成
    // 結局 A（前面的 A 已吃掉業力>80 + 持真傘 + 體諒），也未落入結局 B（詛咒／業力<0
    // ／冷淡終局皆已在上面排除，而體諒本身即蘊含非冷淡終局）。因此此處單憑
    // Flag_ConsoledTA ⇒ 一顆善心搭配業力落在 [0,80]：苦甜但非壞的結局。刻意排在 C
    // 之前，讓「體諒但不完美」偏向溫暖的 D 而非破財消災的 C；即使玩家同時買了醜傘，
    // 道德選擇仍勝過購物決定而得到更有份量的 D。結局摘要會據此向 UI 提供破傘外觀。
    if (player.HasFlag(kFlagConsoledTA)) {
        semester.Transition(SemesterState::Ending_D);
        PublishChapterTransitionToast(bus, SemesterState::Ending_D);
        dialog.Close();
        return;
    }

    // 結局 C：在第四章集英樓便利商店買下超醜螢光綠雨傘（攤販設立
    // Flag_BoughtUglyUmbrella）——即設計指定的破財消災「普通結局」。其中
    // Flag_TaFinaleChoiceMade 這一支作為保險的「完全」預設而保留：邏輯上它已被前
    // 面的分支搶先（質問→冷淡終局→B，體諒→A／D），但保留它可確保閘門對任何終局後
    // 的旗標組合（現在或未來）都必能解析、永不在第四章卡死。四結局樹下，一旦
    // Flag_TaFinaleChoiceMade 設立，閘門即為完全：終局路線必有 A／B／D 其一觸發，C
    // 則接住買傘或任何殘餘情形。嚴格設限，使終局前的第四章自由探索（漫遊、拿詛咒
    // 傘、買醜傘）行為完全不變。
    if (player.HasFlag(kFlagBoughtUglyUmbrella) ||
        player.HasFlag(kFlagTaFinaleChoiceMade)) {
        semester.Transition(SemesterState::Ending_C);
        PublishChapterTransitionToast(bus, SemesterState::Ending_C);
        dialog.Close();
        return;
    }
}

} // namespace nccu
