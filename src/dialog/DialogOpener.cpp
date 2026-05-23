#include "dialog/DialogOpener.h"
#include "quest/Chapter2Quest.h"
#include "quest/Chapter3Quest.h"
#include "quest/Chapter4Quest.h"
#include "dialog/DialogState.h"
#include "dialog/DialogSource.h"
#include "entities/Player.h"
#include <string>
#include <vector>

namespace nccu {

namespace {

// Scaffold: which NPCs present the branch menu in this state. The Ch1
// choice-opener set is now {suit_senior, victim, shop_auntie}.
// 西裝學長 / 苦主 carry the genuine ripple A/B; 福利社阿姨's (c)
// branch sets Flag_BoughtUglyUmbrella -> Ending C (the buy-umbrella
// path — its subState 1/2 already live in the chapter content). Everyone else
// stays line-only.
bool UsesChoiceOpener(std::string_view npcId, SemesterState s) {
    if (s != SemesterState::Chapter1_AddDrop) return false;
    return npcId == "suit_senior" || npcId == "victim" ||
           npcId == "shop_auntie";
}

}  // namespace

void OpenNpcDialogSub(DialogState& dlg, std::string_view npcId,
                      SemesterState state, int subState) {
    for (const auto& sub : nccu::dialog::Entries(npcId, state)) {
        if (sub.subState == subState) {
            dlg.Open(sub.lines);
            return;
        }
    }
    dlg.Open({});  // no match -> stays inactive
}

void OpenNpcDialog(DialogState& dlg, std::string_view npcId,
                   SemesterState state) {
    const auto& subs = nccu::dialog::Entries(npcId, state);

    const nccu::dialog::SubEntry* opener = nullptr;
    for (const auto& sub : subs) {
        if (sub.subState == 0) {
            opener = &sub;
            break;
        }
    }
    if (opener == nullptr) { dlg.Open({}); return; }  // no opener -> inactive

    std::vector<std::string> openerLines = opener->lines;

    if (!UsesChoiceOpener(npcId, state)) {
        dlg.Open(std::move(openerLines));  // line-only
        dlg.SetNpcContext(std::string(npcId));
        return;
    }

    // Highest substate that is a genuine BRANCH of this opener's decision
    // (vs a post-decision recap reached only by ResolveOpenerSubState's
    // routed line-only path). The Ch1 victim now has a (d) 重逢致謝 recap
    // (善有善報: shown AFTER the umbrella is returned, routed via
    // ResolveOpenerSubState==3); it must NOT appear as a premature menu
    // choice at the (a) plea, where the only real branches are (b) 承諾 /
    // (c) 無視. Capping at (c)=2 keeps the victim's plea menu to those two,
    // exactly as it was before the recap substate was added; every other
    // choice-opener NPC keeps every ≥1 substate (cap stays inclusive).
    const int maxChoiceSub =
        (state == SemesterState::Chapter1_AddDrop && npcId == "victim")
            ? 2 : 99;
    std::vector<DialogChoice> choices;
    for (const auto& sub : subs) {
        if (sub.subState >= 1 && sub.subState <= maxChoiceSub) {
            choices.push_back(DialogChoice{
                sub.choiceLabel, sub.karmaDelta,
                sub.setsFlag, sub.flagValue, sub.lines});
        }
    }
    dlg.Open(std::move(openerLines), std::move(choices));
    dlg.SetNpcContext(std::string(npcId));
}

int ResolveOpenerSubState(std::string_view npcId, SemesterState state,
                          const Player& player) {
    if (state == SemesterState::Chapter1_AddDrop) {
        if (npcId == "ta") {
            if (player.HasFlag("Flag_HelpedTA_Ch1")) return 1;
            if (player.HasFlag("Flag_FoundForm"))     return 1;
            return 0;
        }
        if (npcId == "victim") {
            // 善有善報 routing:
            //   授予真傘後 (Flag_HasTrueUmbrella) -> (d) 重逢致謝 recap [3]
            //     （授予 + 清關已由 TryReturnVictimUmbrella 完成；(d) 純
            //      對白，loader 無旗標，once-apply 不重套——victim recap 先例）。
            //   已承諾 (Flag_PromisedVictim)        -> (b) 承諾後續 [1]
            //     （含「先幫他找傘」的提示由 TryReturnVictimUmbrella 的
            //      ShowMessage 呈現；(b) 是 line-only recap）。
            //   否則                                -> (a) 在雨中初遇 [0]。
            if (player.HasFlag("Flag_HasTrueUmbrella")) return 3;   // (d)
            if (player.HasFlag("Flag_PromisedVictim"))  return 1;   // (b)
            return 0;                                               // (a)
        }
    }
    if (state == SemesterState::Chapter2_Midterms) {
        // S5c-3 ripple routing. Line-only recap; the once-per-Ch2 karma
        // is landed by TryApplyCh2Ripple, not here.
        //
        // B5: 學霸/苦主/阿姨's reactive beats USED to be inline
        // `*（若 Flag_X）*` lines the parser silently drops. They are now
        // re-authored as genuine flag-gated SEPARATE subStates in
        // chapter2.md and routed here so the line actually displays
        // (mirrors the 助教 "(c) 取代 (a) 段" pattern):
        //   學霸  Flag_TookCursedUmbrella & !Recovered -> (b) 詛咒冷反應
        //   苦主  Flag_PromisedVictim                  -> (c) 承諾回扣
        //   苦主  Flag_BoughtUglyUmbrella              -> (d) 醜傘辨識
        //   阿姨  Flag_BoughtUglyUmbrella              -> (b) 醜傘辨識
        if (npcId == "suit_senior") {
            if (player.HasFlag("Flag_HelpedSenior"))  return 1;  // (b) +3
            if (player.HasFlag("Flag_ScoldedSenior")) return 2;  // (c) -3
            return 0;                                            // (a) 路過
        }
        if (npcId == "ta") {
            // Precedence (chapter2.md L225 「取代 (a)/(b) 段」 >
            // L211 「取代 (a) 段」): ProfessorTrap outranks HelpedTA.
            if (player.HasFlag("Flag_HasProfessorTrap")) return 2;  // (c) -10
            if (player.HasFlag("Flag_HelpedTA_Ch1"))     return 1;  // (b)
            return 0;                                               // (a)
        }
        if (npcId == "librarian") {
            // (a) 詢問線索：指向羅馬廣場雕像下的學霸（不再交辦撿筆記）
            // -> (b) 喚醒學霸後的確認 recap。純資訊節點（chapter2.md：
            // 不給 karma、無 flag），故 OpenNpcDialog 的 once-apply 區對
            // (b) 是 no-op，純 line-only recap（ta/victim recap 先例）。
            if (player.HasFlag(kFlagBookwormWoken)) return 1;
            return 0;
        }
        if (npcId == "bookworm") {
            // 兩階段任務機（TryRescueBookworm，鑰匙 Flag_Bookworm）的
            // 對話映射：
            //   Recovered          -> (d) 致謝 recap [3]（line-only；+5 已
            //     在 rescue 套，(d) blockquote 無 flag 註解，once-apply
            //     不重套）。
            //   喚醒後未換回傘      -> (c) 已喚醒、等撿筆記 recap [2]
            //     （喚醒互動本身的提示由 TryRescueBookworm 的 ShowMessage
            //     呈現；(c) 子段現已重寫為純對白、可路由）。
            //   喚醒前持詛咒傘      -> (b) 詛咒冷反應變體 [1]
            //     (chapter2.md (b)「取代 (a)」；學霸直覺感應到那把寫著別人
            //     名字的傘——CursedUmbrella.cpp 註記的 Ch2 冷反應)。
            //   否則                -> (a) 常態遊魂初遇 [0]。
            if (player.HasFlag(kFlagBookwormRecovered)) return 3;
            if (player.HasFlag(kFlagBookwormWoken))     return 2;  // (c)
            if (player.HasFlag("Flag_TookCursedUmbrella")) return 1;  // (b)
            return 0;                                                // (a)
        }
        if (npcId == "shop_auntie") {
            // B5: 買過集英樓螢光綠醜傘 -> (b) 阿姨認出你（取代 (a)）。
            // 否則 (a) 考試週招呼。(c) 狼狽版仍由 rainMeter 機制觸發，
            // 非 opener 路由（沿用原狀，已知省略）。
            if (player.HasFlag("Flag_BoughtUglyUmbrella")) return 1;  // (b)
            return 0;                                                // (a)
        }
        if (npcId == "victim") {
            // B5: Ch1 承諾過 -> (c) 她記得承諾（取代 (a)）；買過醜傘
            // -> (d) 她注意到那把醜傘（取代 (b)）。兩旗標互不相斥，
            // 承諾的情感回扣優先於醜傘的辨識橋段。否則 (a) 常態路過。
            if (player.HasFlag("Flag_PromisedVictim")) return 2;     // (c)
            if (player.HasFlag("Flag_BoughtUglyUmbrella")) return 3;  // (d)
            return 0;                                                // (a)
        }
    }
    if (state == SemesterState::Chapter3_SportsDay) {
        // 物物交換鏈三節點：route to (b)「交易完成 / 情報揭露」once
        // this NPC's link has been done, else (a). The (a) sub-blocks
        // 「玩家尚未帶X / 玩家帶著X」are parser-flattened conditional
        // lines (KNOWN OMISSION, same class as S5c-2 (c)/(c-fail) and
        // 學霸 (a) cursed line) — accepted, not routable without a
        // chapter3.md edit. (b) is line-only recap; the +3/+3/+5 is
        // landed by TryAdvanceCh3Trade, not the opener's once-apply.
        //
        // S5d-3 ripple routing (genuine flag-gated SEPARATE subStates;
        // chapter3.md karma is `- \`// karma\`` bullet-doc, NOT a `>`
        // blockquote, so nothing here is parser-applied — these route
        // are pure narrative recap, the only code-karma is ProfTrap
        // -10 via TryApplyCh3Ripple).
        if (npcId == "bookworm") {
            // (a) Ch2 救回分支（Flag_BookwormRecovered=true）/
            // (b) 未救回分支（=false）.
            if (player.HasFlag(kFlagBookwormRecovered)) return 0;
            return 1;
        }
        if (npcId == "ta") {
            // (c) Flag_HelpedTA_Ch1 分支；否則 (a) 登記桌初次接觸.
            if (player.HasFlag("Flag_HelpedTA_Ch1")) return 2;
            return 0;
        }
        if (npcId == "victim") {
            // (a) Flag_PromisedVictim=true 且傘尚未歸還 /
            // (b) =false 或 Ch1 無承諾.
            if (player.HasFlag("Flag_PromisedVictim")) return 0;
            return 1;
        }
        if (npcId == "suit_senior") {
            // HelpedSenior -> (b) 物物交換鏈提示（省一步）. Else (a),
            // whose Helped=true/false condition lines are parser-
            // flattened (KNOWN OMISSION); ScoldedSenior「不觸發對話」
            // is likewise not expressible as a subState — accepted,
            // would need a chapter3.md edit.
            if (player.HasFlag("Flag_HelpedSenior")) return 1;
            return 0;
        }
        if (npcId == "vendor_sausage_a") {
            if (player.HasFlag(kFlagHasSausage) ||
                player.HasFlag(kFlagHasLoudspeaker) ||
                player.HasFlag(kFlagKnowsUmbrellaLoc)) return 1;
            return 0;
        }
        if (npcId == "loudspeaker_b") {
            if (player.HasFlag(kFlagHasLoudspeaker) ||
                player.HasFlag(kFlagKnowsUmbrellaLoc)) return 1;
            return 0;
        }
        if (npcId == "senior_c") {
            if (player.HasFlag(kFlagKnowsUmbrellaLoc)) return 1;
            return 0;
        }
    }
    if (state == SemesterState::Chapter4_Finals) {
        // S5e-2a peak ripple routing (line-only recap; chapter4.md
        // karma is `- \`// karma\`` bullet-doc, NOT a `>` blockquote,
        // so nothing is parser-applied — the Ch4 karma that matters is
        // landed by TryApplyCh4Ripple (S5e-2c) and the 助教 (d) 體諒
        // choice (S5e-2d)). 助教 (d) is NOT routed here — it is a
        // code-constructed choice-opener (S5e-2d).
        if (npcId == "suit_senior") {
            // chapter4.md L88: !HelpedSenior / ScoldedSenior → 學長
            // 不出場. Spawn-suppression is a KNOWN OMISSION (roster
            // keeps him); degrade to (a) 假笑面具. Otherwise karma
            // splits the arc: >70 崩潰坦白 (b), <30 翻臉 (c), the
            // 30..70 middle stays (a).
            if (!player.HasFlag("Flag_HelpedSenior")) return 0;
            if (player.GetKarma() > 70) return 1;   // (b)
            if (player.GetKarma() < 30) return 2;   // (c)
            return 0;                               // (a)
        }
        if (npcId == "bookworm") {
            // (b) Ch2 救他 callback / (c) 未救.
            if (player.HasFlag(kFlagBookwormRecovered)) return 1;
            return 2;
        }
        if (npcId == "ta") {
            // (b)/(c) 互斥, HelpedTA_Ch1 優先 (chapter4.md L235); the
            // (c) -15 still lands separately via TryApplyCh4Ripple.
            // (a) 巡考慌張 default. (d) 體諒 is the S5e-2d choice.
            if (player.HasFlag("Flag_HelpedTA_Ch1"))     return 1;  // (b)
            if (player.HasFlag("Flag_HasProfessorTrap")) return 2;  // (c)
            return 0;                                                // (a)
        }
        if (npcId == "victim") {
            // (b) 淡漠：承諾過但傘到 Ch4 仍未在手 (chapter4.md L338).
            // 否則 (a) 釋懷（已歸還 or Ch1 無承諾, L325). HasUmbrella()
            // is set by TrueUmbrella::beClaimed.
            if (player.HasFlag("Flag_PromisedVictim") &&
                !player.HasUmbrella()) return 1;
            return 0;
        }
        if (npcId == "shop_auntie") {
            // B3: the Ch1→Ch4 阿姨 ripple the GDD names
            // (Flag_BoughtCoffeeForAuntie_Ch1) but engine never read.
            // Ch1 請過咖啡情分 → (a) 直接情報（subState 0，主動說助教
            // 往哪跑）；否則 → (d) 間接情報（subState 3，只說「那個常
            // 來的助教很趕」）. The +3 (a)-route callback is path-b via
            // TryApplyCh4Ripple (chapter4.md karma is bullet-doc, not a
            // `>` blockquote — nothing here is parser-applied). (b)/(c)
            // 推銷綠傘/拒買 stay the Ending-C 集英樓 Vendor flavour beats.
            if (player.HasFlag(kFlagBoughtCoffeeForAuntie)) return 0;  // (a)
            return 3;                                                 // (d)
        }
    }
    return 0;
}

void OpenNpcDialog(DialogState& dlg, Player& player,
                   std::string_view npcId, SemesterState state) {
    // C.3(b): 西裝學長 is the ripple-critical choice-opener. Once the
    // player has committed a choice with him (Flag_SuitSeniorChoiceMade
    // is set by GameController when a suit_senior choice is confirmed),
    // re-talking must NOT re-present the branch menu — otherwise the
    // player could stack mutually-exclusive ripple flags (pick (d)
    // Flag_HelpedSenior, then re-talk and pick (c) Flag_ScoldedSenior).
    // Recap = the subState-0 opener lines, line-only (no menu, no
    // re-applied karma/flag). shop_auntie / victim stay re-enterable
    // (low impact; after C.1 a shop_auntie re-entry is harmless).
    if (state == SemesterState::Chapter1_AddDrop && npcId == "suit_senior" &&
        player.HasFlag("Flag_SuitSeniorChoiceMade")) {
        OpenNpcDialogSub(dlg, npcId, state, 0);   // opener lines, NO choices
        dlg.SetNpcContext(std::string(npcId));
        return;
    }

    // S5e-2d: 助教 (d) 結算 — the moral choice that gates Ending A.
    // chapter4.md (d)'s 「體諒」/「質問」are parser-flattened bold
    // sub-blocks (same class as S5c-2 (c)/(c-fail)), so the choice is
    // CODE-CONSTRUCTED (no 2nd content nod, not a fragile line-split —
    // payload is code, the .md owns flavour; path-b precedent). The
    // opener lines are the routed (a)/(b)/(c) reaction; the menu is the
    // 結算. One-shot like C.3(b): Flag_TaFinaleChoiceMade (set by
    // GameController on confirm) → line-only recap, never re-presented
    // (no double karma / no flipping the moral choice).
    if (state == SemesterState::Chapter4_Finals && npcId == "ta") {
        const int taSub = ResolveOpenerSubState(npcId, state, player);
        std::vector<std::string> openerLines;
        for (const auto& e : nccu::dialog::Entries(npcId, state))
            if (e.subState == taSub) { openerLines = e.lines; break; }

        if (player.HasFlag("Flag_TaFinaleChoiceMade")) {
            dlg.Open(std::move(openerLines));     // recap, NO menu
            dlg.SetNpcContext(std::string(npcId));
            return;
        }
        std::vector<DialogChoice> taChoices;
        taChoices.push_back(DialogChoice{
            "體諒助教的辛勞", 15, "Flag_ConsoledTA", true,
            {"（你選擇體諒——他愣了一下）",
             "「……你不追究？」",
             "（他把考卷整理了一下，低聲）",
             "「你這學期……做了蠻多的。我有看到。」",
             "（轉身，低聲）歐趴糖，之後找你。"}});
        taChoices.push_back(DialogChoice{
            "質問／強硬索回", -5, std::string{}, false,
            {"（你伸手，語氣不軟）",
             "「規定就是規定，我配合。」",
             "「傘是你的，我還你。」",
             "（他不再說話，繼續巡考）"}});
        // 1c: the no-commit exit. Appended LAST so 體諒/質問 keep indices
        // 0/1 (test_ch4_finale pins them). Zero karmaDelta + empty setsFlag
        // and NO nextLines → Advance() Close()s immediately (vendor-decline
        // shape), and GameController skips Flag_TaFinaleChoiceMade for this
        // label, so the finale menu is NOT consumed — the player can walk
        // off and re-approach 助教 to decide later (no soft-lock, no
        // accidental Ending). The 助教 keeps巡考 in the background.
        taChoices.push_back(DialogChoice{
            kDialogExitLabel, 0, std::string{}, false, {}});
        dlg.Open(std::move(openerLines), std::move(taChoices));
        dlg.SetNpcContext(std::string(npcId));
        return;
    }

    const int sub = ResolveOpenerSubState(npcId, state, player);
    if (sub == 0) { OpenNpcDialog(dlg, npcId, state); return; }  // 1b-2 path

    const nccu::dialog::SubEntry* hit = nullptr;
    for (const auto& e : nccu::dialog::Entries(npcId, state))
        if (e.subState == sub) {
            hit = &e; break;
        }
    if (hit == nullptr) { OpenNpcDialog(dlg, npcId, state); return; }  // fallback

    dlg.Open(hit->lines);  // line-only consequence / recap
    dlg.SetNpcContext(std::string(npcId));

    // Apply the entry's own side-effects ONCE — the Ch1 1b-3 reward
    // recap (ta 申請書 / victim 承諾): only when it sets a true flag the
    // player doesn't have yet. Scoped to Chapter1_AddDrop on purpose:
    // this is the Ch1 reward-recap mechanism. Ch2/3/4 ripple karma is
    // path-b (TryRescueBookworm / TryApplyChNRipple / trade hooks), and
    // a ripple subState's parsed setsFlag is an ARTIFACT of the
    // chapter*.md prose, not an intended reward — e.g. chapter4.md 助教
    // (c) L235's precedence note carries `Flag_HelpedTA_Ch1 = true`, so
    // an unscoped auto-apply would spuriously grant HelpedTA_Ch1 to a
    // player routed to (c). Ch2/Ch3 only avoided this by content luck
    // (empty setsFlag); the guard makes safety structural.
    const std::string flag(hit->setsFlag);
    if (state == SemesterState::Chapter1_AddDrop &&
        !flag.empty() && hit->flagValue && !player.HasFlag(flag)) {
        player.AddKarma(hit->karmaDelta);
        player.SetFlag(flag);
    }
}

} // namespace nccu
