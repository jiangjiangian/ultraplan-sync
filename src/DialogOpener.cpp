#include "DialogOpener.h"
#include "DialogState.h"
#include "DialogData.h"
#include <string>
#include <vector>

namespace nccu {

void OpenNpcDialog(DialogState& dlg, std::string_view npcId,
                   SemesterState state, int subState) {
    for (const auto& e : nccu::dialog::All()) {
        if (e.npcId == npcId && e.state == state && e.subState == subState) {
            std::vector<std::string> lines;
            lines.reserve(static_cast<std::size_t>(e.lineCount));
            for (int i = 0; i < e.lineCount; ++i)
                lines.emplace_back(e.lines[i]);
            dlg.Open(std::move(lines));
            return;
        }
    }
    dlg.Open({});  // no match -> stays inactive
}

} // namespace nccu
