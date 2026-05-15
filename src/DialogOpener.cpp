#include "DialogOpener.h"
#include "DialogState.h"
#include "DialogData.h"
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

// Scaffold: which NPCs present the branch menu in this state. 1b-3's
// ResolveOpenerSubState will replace this hardcoded allowlist with a
// per-NPC, flag-aware resolver. For Ch1 only 西裝學長 / 苦主 carry the
// genuine ripple A/B; everyone else stays line-only.
bool UsesChoiceOpener(std::string_view npcId, SemesterState s) {
    if (s != SemesterState::Chapter1_AddDrop) return false;
    return npcId == "suit_senior" || npcId == "victim";
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

} // namespace nccu
