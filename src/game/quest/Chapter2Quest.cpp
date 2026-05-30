#include "game/quest/Chapter2Quest.h"
#include "game/dialog/DialogState.h"
#include "engine/events/EventBus.h"
#include "game/entities/Player.h"
#include <string>

namespace nccu {

bool Chapter2NotesComplete(const Player& player) {
    return player.HasFlag(kFlagFoundNote1) &&
           player.HasFlag(kFlagFoundNote2) &&
           player.HasFlag(kFlagFoundNote3);
}

void TryMeetLibrarian(Player& player, std::string_view npcId,
                      SemesterState state) {
    // 見到圖書館管理員是第二章鏈的起點。她的 (a) 線索台詞指引玩家前往羅馬廣場那座學霸
    // 癱倒的雕像，故與她對話會「解鎖」學霸（喚醒步驟 + 他以此旗標為閘的對話）。在她的台詞
    // 開啟、按 E 互動的當下設下——沿用既有的鉤子慣用法（Flag_Bookworm /
    // Flag_SuitSeniorChoiceMade 同樣在互動時設下，而非在對話關閉的回呼）。具冪等性。
    if (state != SemesterState::Chapter2_Midterms) return;
    if (npcId != "librarian") return;
    player.SetFlag(kFlagMetLibrarian);
}

void TryRescueBookworm(EventBus& bus, Player& player,
                       std::string_view npcId, SemesterState state) {
    if (state != SemesterState::Chapter2_Midterms) return;
    if (npcId != "bookworm") return;
    if (player.HasFlag(kFlagBookwormRecovered)) return;   // 已完成

    // 硬閘控——玩家必須先拜訪圖書館管理員（她指向羅馬廣場）。在那之前，學霸無法被喚醒；
    // 開場會把他路由到「先去問櫃台的管理員」的轉向，故此提示為按 E 互動路徑加以呼應。
    // 此處不消耗能量飲料（喚醒步驟在下方）。
    if (!player.HasFlag(kFlagMetLibrarian)) {
        bus.Publish(Event{
            EventType::ShowMessage,
            std::string("這個人睡得很沉……先去問櫃台的管理員吧，"
                        "看看是誰拿走了傘。")});
        return;
    }

    if (!player.HasFlag(kFlagBookworm)) {
        // 階段一——學霸仍癱在羅馬廣場的雕像旁。能量飲料在「此處」、於喚醒步驟消耗
        //（章節內容學霸 (c)「玩家有提神飲料時」）。喚醒會設下 Flag_Bookworm，學霸請玩家
        // 撿回他散落的筆記——World 的延後生成接著以此旗標為閘投放 3 份筆記，故在此刻之前
        // 任何地方都「不」存在筆記。
        if (player.ConsumeOne("EnergyDrink")) {
            player.SetFlag(kFlagBookworm);
            bus.Publish(Event{
                EventType::ShowMessage,
                std::string("學霸回神了：「筆記……被風吹散了，"
                            "幫我撿回來？」")});
        } else {
            // 章節內容學霸 (c-fail) 的防軟鎖：在市集沒買能量飲料的玩家，仍可經
            // 圖書館地下室自動販賣機（ChapterVendors(Chapter2)）喚醒他。
            bus.Publish(Event{
                EventType::ShowMessage,
                std::string("學霸沒有反應……需要提神飲料喚醒他。"
                            "（圖書館地下室自動販賣機 35 元）")});
        }
        return;
    }

    // 階段二——學霸已清醒。以 3 份筆記為交換的閘。
    if (!Chapter2NotesComplete(player)) {
        bus.Publish(Event{
            EventType::ShowMessage,
            std::string("「我的筆記還沒撿齊吧？三頁，散在校園各處。」")});
        return;
    }

    // 章節內容學霸 (d) `// karma +5` + 第二章結算旗標 Flag_BookwormRecovered。交換內容：
    // 筆記 <-> 玩家的傘——學霸把真傘還回玩家手上（SetHeldUmbrella(True)：手持列＋自動遮蔽）。
    // 刻意「不」設 Flag_HasTrueUmbrella（進入下一章即清除，且結局閘只在 Ch4 讀它），故無結局
    // 後果——只是讓背包誠實顯示「換回的真傘」。若玩家同時持有管理員借傘，借傘是另一條由
    // Flag_LibrarianUmbrella 驅動的獨立列，與真傘並存、互不覆蓋（見 BuildInventoryRows）。
    // 走另一條路徑：(d) 引用區塊不帶 `Flag_… = true` 註記，故開場的一次性套用絕不會觸發它
    // ——它在「此處」、於任務完成的程式碼點設下（與 QuestFlagPickup／換回傘 的先例一致：程式碼
    // 設下里程碑旗標，(d) 對話只是致謝回顧）。此處不消耗能量飲料——它已在上方喚醒步驟用掉。
    player.AddKarma(5)
          .SetHeldUmbrella(HeldUmbrella::True)
          .SetFlag(kFlagBookwormRecovered);
    // 背包洩漏修正：3 份筆記現在作為交換的一部分「交還」給學霸——故它們必須「離開」背包。
    // BuildInventoryRows 純由 Flag_FoundNote1/2/3 衍生出任務紙張（學霸的筆記 xN）那一列
    //（見 ItemCatalog.cpp）；若不清除，歸還筆記「之後」在市集背包中仍會看到那一列。在此、
    // 於完成點清除全部三個，使該列在下一幀重建背包時消失。比照第三章交易的攜帶物清潔
    //（Chapter3Quest 在每樣換出的當下清除 Flag_HasSausage / Flag_HasLoudspeaker）與苦主
    // 雨傘（TryReturnVictimUmbrella 在授予時清除 Flag_HasVictimUmbrella）。具冪等性——
    // ClearFlag 在旗標不存在時為空操作，且上方 kFlagBookwormRecovered 的提前返回會阻止
    // 重新對話重跑此段。
    player.ClearFlag(kFlagFoundNote1)
          .ClearFlag(kFlagFoundNote2)
          .ClearFlag(kFlagFoundNote3);
    bus.Publish(Event{
        EventType::ShowMessage,
        std::string("傘換回來了。這次，更換是你。")});
}

void LiftChapter2Clear(Player& player, SemesterState state,
                       const DialogState& dialog) {
    if (state != SemesterState::Chapter2_Midterms) return;
    if (dialog.Active()) return;                       // (d) 對話仍在畫面上
    if (!player.HasFlag(kFlagBookwormRecovered)) return;
    if (player.HasFlag(kFlagCh2Cleared)) return;
    player.SetFlag(kFlagCh2Cleared);
}

void TryLendLibrarianUmbrella(Player& player, std::string_view npcId,
                              SemesterState state) {
    if (state != SemesterState::Chapter2_Midterms) return;
    if (npcId != "librarian") return;
    if (!player.HasFlag(kFlagBookworm)) return;   // 僅她的 (b) 狀態
    if (player.HasFlag(kFlagLibrarianUmbrella)) return;  // 只一次——不堆疊

    // 章節內容管理員 (b)「（遞過一把折疊傘）這個先拿著，別在外面淋著。」：玩家現在「持有」
    // 管理員的傘。借傘純由 Flag_LibrarianUmbrella 表示（背包列由它驅動，見 BuildInventoryRows），
    // 遮蔽則靠 SetHasUmbrella(true)——使「在戶外且撐傘」的降雨路徑（ApplyRainSheltered）在玩家
    // 尋找 3 份筆記期間維持緩慢淋濕。刻意「不」佔用 heldUmbrella_ 槽（也就不是某種 HeldUmbrella），
    // 使稍後換回的真傘（TryRescueBookworm 設 HeldUmbrella::True）能與借傘並存、互不覆蓋——正是
    // 回報的「借傘在插曲段蓋過真傘」缺陷的根因修正。同樣刻意「不」設 Flag_HasTrueUmbrella——借傘
    // 絕不可單憑自身解鎖結局 A。口白的交付台詞位於 (b) 行（DialogOpener 在 Flag_Bookworm 時把
    // 管理員路由到 (b)），故此處不需內聯 ShowMessage（那會與 (b) 場景重複）。
    player.SetHasUmbrella(true);
    player.SetFlag(kFlagLibrarianUmbrella);
}

void TryReturnLibrarianUmbrella(EventBus& bus, Player& player,
                                std::string_view npcId, SemesterState state,
                                SemesterState returnTo) {
    // 嚴格限定於第二章→第三章插曲段的歸還點。返回第三章的那個市集，是唯一玩家仍可能持有
    // 管理員借傘之處（它在第二章借出、並在進入「下一」章時清除——SceneRouter），故
    // InterludeReturnTo()==Chapter3_SportsDay 是精確的閘。其餘任何狀態／市集／npcId 皆為
    // 空操作。
    if (state != SemesterState::Interlude_Market) return;
    if (returnTo != SemesterState::Chapter3_SportsDay) return;
    if (npcId != kNpcLibrarianReturn) return;

    if (player.HasFlag(kFlagLibrarianUmbrellaReturned)) {
        // 歸還後再次對話：一句簡短的收尾台詞，「無」第二次業力。
        bus.Publish(Event{
            EventType::ShowMessage,
            std::string("這把傘已經還給圖書館了。")});
        return;
    }

    // 防禦性：標記只在玩家持有借傘時生成，但仍重新檢查旗標，使偶然的對話無法在沒有真正
    // 持借傘的情況下給予 +10。借傘現純由旗標表示（不再佔 heldUmbrella_），故只查旗標即可。
    if (!player.HasFlag(kFlagLibrarianUmbrella)) {
        return;
    }

    // 責任感的回報：把借傘交還。業力 +10、清除借出閂鎖，並設下一次性旗標使其恰好觸發一次。
    // 遮蔽只在玩家「別無其他持有傘」時關閉：若此時手上仍握著換回的真傘（HeldUmbrella::True），
    // 保留遮蔽與該手持列——歸還借傘絕不可順手弄丟真傘（回報缺陷的另一半）。「不」動到任何
    // 結局旗標——借傘從來就不是 Flag_HasTrueUmbrella，故結局 A 不受影響。
    const bool keepShelter = player.HeldUmbrellaKind() != HeldUmbrella::None;
    player.AddKarma(10)
          .SetHasUmbrella(keepShelter)
          .ClearFlag(kFlagLibrarianUmbrella)
          .SetFlag(kFlagLibrarianUmbrellaReturned);
    bus.Publish(Event{
        EventType::ShowMessage,
        std::string("你把管理員的傘還回了圖書館服務台。"
                    "「傘有借有還，謝謝你特地拿回來。」")});
}

bool Ch2IndicatorVisible(std::string_view npcId, bool isQuestGiver,
                         const Player& player) {
    const bool metLibrarian = player.HasFlag(kFlagMetLibrarian);
    const bool woken        = player.HasFlag(kFlagBookworm);
    const bool recovered    = player.HasFlag(kFlagBookwormRecovered);
    // 順序：管理員 → 學霸 →（缺飲料時 自販機）→ 學霸。
    if (npcId == "librarian")
        return !metLibrarian;          // 鏈頭：見到管理員取得線索前都亮，之後熄滅
    if (npcId == "bookworm")
        return metLibrarian && !recovered;   // 見管理員後亮，貫穿喚醒→撿筆記→換回
    if (npcId == kNpcCh2Vendor)
        // 自販機：見管理員後、學霸尚未喚醒、且手上沒有提神飲料時，作為「先去買瓶飲料」的
        // 中繼指引亮起；一旦持有飲料（或學霸已喚醒）即熄滅（「有就略過」）。
        return metLibrarian && !woken &&
               player.ConsumableCount("EnergyDrink") == 0;
    // 其餘任何 NPC：維持不變——尊重其名冊的 isQuestGiver 位元。
    return isQuestGiver;
}

void TryApplyCh2Ripple(Player& player, std::string_view npcId,
                       SemesterState state) {
    if (state != SemesterState::Chapter2_Midterms) return;

    if (npcId == "suit_senior") {
        if (player.HasFlag(kFlagCh2RippledSuitSenior)) return;  // 只一次
        // HelpedSenior / ScoldedSenior 互斥（第一章的選項防護）。章節內容學長 (b)
        // `// karma +3`（回呼註記帶 Flag_HelpedSenior=true、已持有，故開場的一次性套用
        // 會略過它）。
        //
        // 重新定調：第一章 (b) 選項不再是帶敵意的斥責（-5），而是「理性」的指正（+3）。
        // Flag_ScoldedSenior 僅作為「保持距離」這條支線的鑰匙（第二章 (c) 尷尬讓開／第三章
        // 距離／第四章不主動出場）；學長是出於些許尷尬、而非怨恨而保持禮貌距離，故它「不」
        // 帶業力懲罰——後續再扣 -3 會錯誤地討回該選項剛賺到的理性 +3。此旗標仍經一次性
        // 旗標消費，使支線恰好路由一次；只是少了業力負債。
        if (player.HasFlag(kFlagHelpedSenior)) {
            player.AddKarma(3).SetFlag(kFlagCh2RippledSuitSenior);
        } else if (player.HasFlag(kFlagScoldedSenior)) {
            player.SetFlag(kFlagCh2RippledSuitSenior);          // 業力中性
        }
        return;
    }

    if (npcId == "ta") {
        if (player.HasFlag(kFlagCh2RippledTA)) return;          // 只一次
        // 章節內容助教 (c) `// karma -10`（第一章漣漪效應兌現，「karma -10 在此落地」）
        // ——一個只有業力的條目，絕不自動套用。(b) HelpedTA_Ch1 是無 `// karma` 的資訊
        // 漣漪，故沒有東西可為它落地（未設旗標——它從來就沒有業力負債要清算）。
        if (player.HasFlag(kFlagHasProfessorTrap)) {
            player.AddKarma(-10).SetFlag(kFlagCh2RippledTA);
        }
        return;
    }
}

} // namespace nccu
