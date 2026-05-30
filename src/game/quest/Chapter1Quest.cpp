#include "game/quest/Chapter1Quest.h"
#include "engine/events/EventBus.h"
#include "game/entities/Player.h"
#include "game/dialog/DialogState.h"
#include "game/quest/ItemCatalog.h"        // ItemInfoFor — the 中文 toast name
#include "game/vendor/VendorMessages.h"    // shared 花費/餘額 + 你錢不夠 copy
#include <string>

namespace nccu {

void TryReturnVictimUmbrella(EventBus& bus, Player& player,
                             std::string_view npcId, SemesterState state) {
    if (state != SemesterState::Chapter1_AddDrop) return;
    if (npcId != "victim") return;
    if (player.HasFlag(kFlagHasTrueUmbrella)) return;   // 已授予

    if (!player.HasFlag(kFlagPromisedVictim)) {
        // 承諾這一節拍由 (a) 請求 / (b) 承諾 的 DialogChoice 掌管；玩家連答應幫忙都
        // 還沒之前，沒有什麼可歸還。空操作，使開場原封不動路由到 (a)/(b)。
        return;
    }

    if (!player.HasFlag(kFlagHasVictimUmbrella)) {
        // 承諾了卻兩手空空——提示玩家去找西裝學長掉在集英樓附近、可被尋獲的那把傘
        //（QuestFlagPickup）。不改變狀態。
        bus.Publish(Event{
            EventType::ShowMessage,
            std::string("「找到我的傘了嗎？先幫他找回那把傘吧——"
                        "聽說那個西裝學長往集英樓跑了。」")});
        return;
    }

    // 授予——善有善報的回報（現在是一場在章節通關「之前」真正演出的交換場景）。玩家把
    // 苦主的傘帶了回來；苦主收下後，揭示他「也」找到了玩家的真傘並交還。這會設下
    // HasUmbrella + Flag_HasTrueUmbrella（結局 A 的精確條件，見 EndingGate.cpp）——但
    // 此處「不」發布 UmbrellaClaimed。改作法之前，內聯發布 UmbrellaClaimed 會在「同一幀」
    // 經事件接線的第一章兄弟 if 驅動 Transition(Interlude)，使本章在 (d) 重逢致謝 交換
    // 對話能被讀到「之前」就闔上（緊接其後在 GameController 執行的開場，會看到
    // state==Interlude 而從不把苦主路由到 (d)）。
    //
    // 修正比照第二章的 LiftChapter2Clear：在此「靜默」授予旗標，讓開場把苦主路由到他的
    // (d) 交換回顧（ResolveOpenerSubState：Flag_HasTrueUmbrella → (d)），並把 UmbrellaClaimed
    // 的發布延後到 LiftChapter1Clear——後者只在 (d) 對話關閉「之後」才觸發。於是玩家先讀
    // 完交換，「然後」第一章才清關到插曲段。交換的口白台詞放在章節內容的苦主 (d)；不需內聯
    // ShowMessage（那會與 (d) 場景重複地單行回放）。
    player.ClearFlag(kFlagHasVictimUmbrella);   // 苦主收回他的傘
    // 玩家現在「持有」苦主交還的真傘——背包把「苦主的傘」那一列換成「真傘」那一列
    //（SetHeldUmbrella 同時也設下 HasUmbrella）。Flag_HasTrueUmbrella 維持為結局 A 的標記。
    player.SetHeldUmbrella(HeldUmbrella::True);
    player.SetFlag(kFlagHasTrueUmbrella);
}

void TryReturnTaForm(Player& player, std::string_view npcId,
                     SemesterState state) {
    if (state != SemesterState::Chapter1_AddDrop) return;
    if (npcId != "ta") return;
    if (player.HasFlag(kFlagHelpedTACh1)) return;   // 已交還（冪等）
    if (!player.HasFlag(kFlagFoundForm)) return;     // 還沒撿到申請書，無可歸還

    // 章節內容助教 (b)：玩家把吹散的加退選申請書撿回來交還。正直行為——karma +5 並設下
    // 跨章情分旗標 Flag_HelpedTA_Ch1（Ch2 捷徑指路／Ch3 (c) 漣漪 +5／Ch4 道歉弧／
    // ending_a 名冊條件以它為鍵；先前此旗標「從未被設下」，使整條助教支線形同虛設——
    // 此處補上設置點）。同時清除 Flag_FoundForm 使背包的「申請書」列消失（比照
    // TryReturnVictimUmbrella 清 Flag_HasVictimUmbrella、換回筆記清 FoundNote*）。致謝
    // 口白由助教 (b) 對話呈現（DialogOpener 在 Flag_HelpedTA_Ch1 時把助教路由到 (b)），
    // 故此處不內聯 ShowMessage。
    player.AddKarma(5)
          .SetFlag(kFlagHelpedTACh1)
          .ClearFlag(kFlagFoundForm);
}

void LiftChapter1Clear(EventBus& bus, Player& player, SemesterState state,
                       const DialogState& dialog) {
    if (state != SemesterState::Chapter1_AddDrop) return;
    if (!player.HasFlag(kFlagHasTrueUmbrella)) return;  // 尚未授予
    if (dialog.Active()) return;                          // (d) 對話仍在畫面上
    if (player.HasFlag(kFlagClearChapter1)) return;       // 只觸發一次
    // (d) 重逢致謝 對話已演完並關閉——「現在」才清第一章關。
    // 以 TrueUmbrella::BeClaimed 的精確配對順序發布——ShowMessage「先」、
    // UmbrellaClaimed「後」——使章節通關提示拿下唯一的 Top HUD 插槽，而拾取台詞落在
    // Bottom（反轉此配對會重現由 tests/quest/test_chapter_transitions.cpp 固定的回歸）。
    // UmbrellaClaimed("TrueUmbrella") 經事件接線的第一章兄弟 if 驅動第一章 → 插曲段
    //（returnTo 第二章）。此一次性旗標使其即使每個非對話幀都輪詢，仍恰好觸發一次。
    player.SetFlag(kFlagClearChapter1);
    bus.Publish(Event{
        EventType::ShowMessage,
        std::string("傘找回來了。雨還沒停，但你的心安定了一點。")});
    bus.Publish(Event{
        EventType::UmbrellaClaimed, std::string("TrueUmbrella")});
}

bool TryBuyAuntieUglyUmbrella(EventBus& bus, Player& player,
                              std::string_view npcId,
                              std::string_view choiceLabel,
                              SemesterState state) {
    if (state != SemesterState::Chapter1_AddDrop) return false;
    if (npcId != "shop_auntie") return false;
    // 章節內容中 (c) 的標題是「### (c) 購買醜綠傘」，故選項開場標籤恰為「購買醜綠傘」。
    // 比對它，使阿姨的其他選項（詢問雨傘 / 請阿姨喝一杯熱咖啡 / 我再想想… 退出）不受影響。
    if (choiceLabel != "購買醜綠傘") return false;

    // 冪等：阿姨的選單在重新對話時會再次呈現（shop_auntie 未自我上鎖），故若無此防護，
    // 第二次選取會為玩家已持有的傘再扣 80 元。
    if (player.HeldUmbrellaKind() == HeldUmbrella::Ugly) return false;

    namespace msg = nccu::vendor::msg;

    // DeductMoney 是把關者——錢包不足以支付時回傳 false（無副作用），故持有型傘「只」在
    // 真正扣款時才授予（比照 Vendor::TryBuy 的順序）。
    if (!player.DeductMoney(kCh1UglyUmbrellaPrice)) {
        bus.Publish(Event{
            EventType::ShowMessage, std::string(msg::kInsufficientFunds)});
        return false;
    }

    // 授予「持有型」醜傘：一個背包列 + 自動遮雨（ApplyRainSheltered）。這是一把玩家攜帶的
    //「真實」雨傘，「非」結局 C 鎖——此處刻意「不」設下 Flag_BoughtUglyUmbrella（那是第四章
    // 集英樓攤販的承諾；見 EndingGate.cpp）。也無業力（(c) 的註記是 `// karma +0`，是務實
    // 但道德中性的購買）。
    player.SetHeldUmbrella(HeldUmbrella::Ugly);

    // 花費/餘額 提示——與 Vendor::TryBuy 完全相同的花費行，重用同一份 vendor::msg 文案 +
    // 圖鑑的中文名稱，使第一章阿姨的購買與市集／集英樓攤販讀來一致（「買了螢光綠醜傘，
    // 花了 80 元（剩 N 元）」）。GetMoney() 為扣款後的餘額。
    const std::string itemName{nccu::ItemInfoFor("UglyUmbrella").displayName};
    bus.Publish(Event{
        EventType::ShowMessage,
        std::string(msg::kPurchasedPrefix) + itemName +
            std::string(msg::kSpentMid) + std::to_string(kCh1UglyUmbrellaPrice) +
            std::string(msg::kSpentUnitOpen) + std::to_string(player.GetMoney()) +
            std::string(msg::kSpentUnitClose)});
    return true;
}

bool Ch1IndicatorVisible(std::string_view npcId, bool isQuestGiver,
                         const Player& player) {
    // 苦主 → 西裝學長 → 苦主 的序列。每個主線 NPC「只」在自己那一步亮起，使同一時間恰好
    // 只有一個主要「!」可見（順序錯亂的接觸由按 E 互動鉤子轉向，而非在此）。
    const bool grantDone   = player.HasFlag(kFlagHasTrueUmbrella);
    const bool promised    = player.HasFlag(kFlagPromisedVictim);
    const bool seniorChoice = player.HasFlag(kFlagSuitSeniorChoiceMade);

    if (npcId == "victim") {
        // 在第 1 步亮起（承諾之前——給出線索），並在第 3 步再次亮起（學長選項之後——把他
        // 的傘帶回來）。其間（玩家正去面對學長）與授予之後（(d) 重逢此時是回顧、非目標）
        // 則熄滅。
        if (grantDone) return false;                 // 完成 → 熄滅
        if (!promised) return true;                  // 第 1 步：取得線索
        return seniorChoice;                         // 學長完成後的第 3 步
    }
    if (npcId == "suit_senior") {
        // 「只」在第 2 步亮起：玩家已有線索（已承諾）但尚未做出學長選項。選項一經確認
        //（Flag_SuitSeniorChoiceMade）便立刻熄滅，把「!」交還給苦主進入第 3 步。以 npcId
        // 為鍵（而非名冊位元），因為學長隨附 isQuestGiver=false。
        return promised && !seniorChoice && !grantDone;
    }
    // 其餘所有第一章 npc 維持其名冊位元（助教申請書跑腿／學霸／阿姨皆 isQuestGiver=false
    // → 永遠不會有主要「!」）。
    return isQuestGiver;
}

} // namespace nccu
