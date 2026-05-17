#include "DialogOpener.h"
#include "Chapter2Quest.h"
#include "DialogState.h"
#include "DialogSource.h"
#include "Player.h"
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

    std::vector<DialogChoice> choices;
    for (const auto& sub : subs) {
        if (sub.subState >= 1) {
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
            if (player.HasFlag("Flag_PromisedVictim")) return 1;
            return 0;
        }
    }
    if (state == SemesterState::Chapter2_Midterms) {
        // S5c-3 ripple routing. Genuine flag-gated SEPARATE subStates
        // only — 學霸/苦主/阿姨's `*（若 Flag_X）*` are conditional
        // LINES inside one subState, which the parser flattens (same
        // class as (c)/(c-fail) in S5c-2; documented omission, not
        // routable without a chapter2.md edit). Line-only recap; the
        // once-per-Ch2 karma is landed by TryApplyCh2Ripple, not here.
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
            // (a) 詢問線索 -> (b) 確認三頁筆記、指向羅馬廣場。純資訊
            // 節點（chapter2.md：不給 karma、無 flag），故
            // OpenNpcDialog 的 once-apply 區對 (b) 是 no-op，純
            // line-only recap（ta/victim recap 先例）。
            if (Chapter2NotesComplete(player)) return 1;
            return 0;
        }
        if (npcId == "bookworm") {
            // 喚醒前皆 (a) 遊魂；TryRescueBookworm 設 Recovered 後
            // -> (d) 致謝 recap（line-only；+5 已在 rescue 套，(d)
            // blockquote 無 flag 註解，once-apply 不重套）。(c)/(c-fail)
            // 刻意不路由：解析器把 (c) 兩個粗體子塊併進同一 subState，
            // 無法分離（需 chapter2.md 切段，屬 C.1 以外的 content
            // gate，Phase 2 不做——已知省略，見計畫 §F.5）。
            if (player.HasFlag(kFlagBookwormRecovered)) return 3;
            return 0;
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

    // Apply the entry's own side-effects ONCE: only when it sets a true
    // flag the player doesn't have yet (ta reward). victim recap's flag
    // is already set by the 1b-2 choice, so this is skipped (no double
    // karma). flagValue==false / empty-flag entries are reached via the
    // 1b-2 choice path, not here.
    const std::string flag(hit->setsFlag);
    if (!flag.empty() && hit->flagValue && !player.HasFlag(flag)) {
        player.AddKarma(hit->karmaDelta);
        player.SetFlag(flag);
    }
}

} // namespace nccu
