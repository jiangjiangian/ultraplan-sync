#include "DialogOpener.h"
#include "DialogState.h"
#include "DialogData.h"
#include "Player.h"
#include <string>
#include <vector>

namespace nccu {

namespace {

std::vector<std::string> EntryLines(const nccu::dialog::Entry& e) {
    std::vector<std::string> lines;
    lines.reserve(static_cast<std::size_t>(e.lineCount));
    for (int i = 0; i < e.lineCount; ++i)
        lines.emplace_back(e.lines[i]);
    return lines;
}

// Scaffold: which NPCs present the branch menu in this state. The Ch1
// choice-opener set is now {suit_senior, victim, shop_auntie}.
// 西裝學長 / 苦主 carry the genuine ripple A/B; 福利社阿姨's (c)
// branch sets Flag_BoughtUglyUmbrella -> Ending C (the buy-umbrella
// path — its subState 1/2 already live in DialogData). Everyone else
// stays line-only.
bool UsesChoiceOpener(std::string_view npcId, SemesterState s) {
    if (s != SemesterState::Chapter1_AddDrop) return false;
    return npcId == "suit_senior" || npcId == "victim" ||
           npcId == "shop_auntie";
}

}  // namespace

void OpenNpcDialogSub(DialogState& dlg, std::string_view npcId,
                      SemesterState state, int subState) {
    for (const auto& e : nccu::dialog::All()) {
        if (e.npcId == npcId && e.state == state && e.subState == subState) {
            dlg.Open(EntryLines(e));
            return;
        }
    }
    dlg.Open({});  // no match -> stays inactive
}

void OpenNpcDialog(DialogState& dlg, std::string_view npcId,
                   SemesterState state) {
    const nccu::dialog::Entry* opener = nullptr;
    for (const auto& e : nccu::dialog::All()) {
        if (e.npcId == npcId && e.state == state && e.subState == 0) {
            opener = &e;
            break;
        }
    }
    if (opener == nullptr) { dlg.Open({}); return; }  // no opener -> inactive

    std::vector<std::string> openerLines = EntryLines(*opener);

    if (!UsesChoiceOpener(npcId, state)) {
        dlg.Open(std::move(openerLines));  // line-only
        return;
    }

    std::vector<DialogChoice> choices;
    for (const auto& e : nccu::dialog::All()) {
        if (e.npcId == npcId && e.state == state && e.subState >= 1) {
            choices.push_back(DialogChoice{
                std::string(e.choiceLabel), e.karmaDelta,
                std::string(e.setsFlag), e.flagValue, EntryLines(e)});
        }
    }
    dlg.Open(std::move(openerLines), std::move(choices));
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
    return 0;
}

void OpenNpcDialog(DialogState& dlg, Player& player,
                   std::string_view npcId, SemesterState state) {
    const int sub = ResolveOpenerSubState(npcId, state, player);
    if (sub == 0) { OpenNpcDialog(dlg, npcId, state); return; }  // 1b-2 path

    const nccu::dialog::Entry* hit = nullptr;
    for (const auto& e : nccu::dialog::All())
        if (e.npcId == npcId && e.state == state && e.subState == sub) {
            hit = &e; break;
        }
    if (hit == nullptr) { OpenNpcDialog(dlg, npcId, state); return; }  // fallback

    dlg.Open(EntryLines(*hit));  // line-only consequence / recap

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
