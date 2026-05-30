#include "game/quest/Chapter4Quest.h"
#include "game/entities/Player.h"
#include "game/dialog/DialogState.h"

/**
 * @file Chapter4Quest.cpp
 * @brief 第四章（期末考）終局的任務邏輯：助教「!」提示的可見性、溫柔終局授予真傘、
 *        各 NPC 漣漪的 karma 回呼，以及依結局優先序觸發的對應自白對白。
 */

namespace nccu {

// 在玩家確認 (d) 結算選項之前，助教是終局唯一亮著「!」的對象（確認時由 GameController
// 設下 Flag_TaFinaleChoiceMade；(c) 退出「不」設此旗標，故退出後「!」仍亮著供再次接觸）。
// 其餘所有第四章 npcId 皆不亮——終局是閘控驅動的，若在那些漣漪風味原型上也點「!」，
// 只會是純粹的雜訊。
bool Ch4IndicatorVisible(std::string_view npcId, const Player& player) {
    if (npcId == "ta") return !player.HasFlag(kFlagTaFinaleChoiceMade);
    return false;
}

void TryGrantTaFinaleUmbrella(Player& player, std::string_view npcId,
                              SemesterState state) {
    if (state != SemesterState::Chapter4_Finals) return;
    if (npcId != "ta") return;
    if (!player.HasFlag(kFlagConsoledTA)) return;   // 僅限溫柔分支
    if (player.HasFlag(kFlagHasTrueUmbrella)) return;  // 等冪
    // 你心存善意時，助教會把「你的」傘交還給你——這是溫柔路線上滿足結局 A 持傘條件的
    // 途徑。EndingGate 仍保有 karma>80 這道閘，故此處本身不會「強制」結局 A；它只是讓
    // 「體諒＋高 karma」在「沒有」那把隱藏的第四章傘時也足以達成。
    // SetHeldUmbrella 記錄手持的真傘（並設定 HasUmbrella），使溫柔終局之後背包會顯示
    // 真傘那一列。
    player.SetHeldUmbrella(HeldUmbrella::True);
    player.SetFlag(kFlagHasTrueUmbrella);
}

void TryApplyCh4Ripple(Player& player, std::string_view npcId,
                       SemesterState state) {
    if (state != SemesterState::Chapter4_Finals) return;

    if (npcId == "suit_senior") {
        // (b) 路線的 karma>70 崩潰坦白，加 10 karma——只在 (b) 路線
        //（HelpedSenior && karma>70）發生一次。
        if (player.HasFlag(kFlagCh4RippledSenior)) return;
        if (player.HasFlag(kFlagHelpedSenior) && player.GetKarma() > 70)
            player.AddKarma(10).SetFlag(kFlagCh4RippledSenior);
        return;
    }

    if (npcId == "bookworm") {
        // (b) 第二章救過他的回呼，加 5 karma，發生一次。
        if (player.HasFlag(kFlagCh4RippledBookworm)) return;
        if (player.HasFlag(kFlagBookwormRecovered))
            player.AddKarma(5).SetFlag(kFlagCh4RippledBookworm);
        return;
    }

    if (npcId == "shop_auntie") {
        // 阿姨 (a) 直接情報的回呼，加 3 karma（第一章請她喝咖啡的情分，第四章以直接情報
        // 兌現），發生一次。只在直接情報路線（Flag_BoughtCoffeeForAuntie）發生——(d)
        // 間接情報路線不給獎勵，與學長 +10 僅限 (b) 路線同理。
        if (player.HasFlag(kFlagCh4RippledAuntie)) return;
        if (player.HasFlag(kFlagBoughtCoffeeForAuntie))
            player.AddKarma(3).SetFlag(kFlagCh4RippledAuntie);
        return;
    }

    if (npcId == "ta") {
        // 兩個「互相獨立」的效果：(b) 坦白對 HelpedTA_Ch1 加 10，以及 (c) 對峙對
        // HasProfessorTrap 減 15——兩者同時成立時，畫面顯示 (b)，但 -15 仍會生效。各用
        // 獨立的一次性鍵，使「幫過助教且持有教授陷阱」的玩家淨得 +10-15 = -5。
        if (!player.HasFlag(kFlagCh4RippledTAHelped) &&
            player.HasFlag(kFlagHelpedTACh1))
            player.AddKarma(10).SetFlag(kFlagCh4RippledTAHelped);
        if (!player.HasFlag(kFlagCh4RippledProfTrap) &&
            player.HasFlag(kFlagHasProfessorTrap))
            player.AddKarma(-15).SetFlag(kFlagCh4RippledProfTrap);
        return;
    }
}

bool TryOpenEndingConfession(Player& player, DialogState& dialog,
                             SemesterState state) {
    if (state != SemesterState::Chapter4_Finals) return false;
    if (dialog.Active()) return false;   // 絕不打斷已開啟的對話框

    // 優先序與 CheckEndingGates 一致（cursed → B 優先於 D/C），使玩家讀到的自白，正是
    // 最終真正會觸發的那個結局。每段自白都以各自的一次性鍵限定只播一次；已坦白過的
    // 觸發條件會被略過，使之後的輪詢改去解算（延後的）結局閘。

    // 詛咒反噬自白——第一章帶來的 Flag_TookCursedUmbrella 在第四章解算為結局 B。在厄運
    // 降臨前先給它一個喘息的節拍。
    if (player.HasFlag(kFlagTookCursedUmbrella) &&
        !player.HasFlag(kFlagCh4ConfessedCursed)) {
        player.SetFlag(kFlagCh4ConfessedCursed);
        dialog.Open({
            "（你握著那把刻著別人名字的傘，傘骨還在發出細微的嗡鳴）",
            "（從加退選那天起，雨就沒真正停過，它一直跟著你）",
            "（你心裡很清楚——你早就成了你最不想變成的那種人）"});
        return true;
    }

    // 務實自白——買了那把螢光綠醜傘；解算為結局 C。
    if (player.HasFlag(kFlagBoughtUglyUmbrella) &&
        !player.HasFlag(kFlagCh4ConfessedUgly)) {
        player.SetFlag(kFlagCh4ConfessedUgly);
        dialog.Open({
            "（你撐開那把醜得理直氣壯的螢光綠傘）",
            "（沒人會想跟你拿錯，這場雨你是花錢買過的）",
            "（算了，傘能擋雨就好。其他的，就這樣吧。）"});
        return true;
    }

    // 尋回真傘自白——在助教終局「之前」，從地上撿回那把藏在體育館的真傘（溫柔終局會播放
    // 它自己的後續台詞，故以 !Flag_TaFinaleChoiceMade 為閘，避免在那裡重複播一拍）。
    if (player.HasFlag(kFlagHasTrueUmbrella) &&
        !player.HasFlag(kFlagTaFinaleChoiceMade) &&
        !player.HasFlag(kFlagCh4ConfessedTrue)) {
        player.SetFlag(kFlagCh4ConfessedTrue);
        dialog.Open({
            "（你的手指扣上那道熟悉的弧度——傘骨紮實，沒有一根是歪的）",
            "（找了一整個學期，繞了那麼遠，它終究回到你手上）",
            "（雨好像小了一點。剩下的路，你想好好走完。）"});
        return true;
    }

    return false;
}

} // namespace nccu
