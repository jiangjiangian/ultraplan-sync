#include "quest/QuestHookTable.h"
#include "quest/Chapter1Quest.h"
#include "quest/Chapter2Quest.h"
#include "quest/Chapter3Quest.h"
#include "quest/Chapter4Quest.h"

namespace nccu {

const std::vector<QuestHook>& InteractQuestHooks() {
    // Built once on first use (function-local static — thread-safe init,
    // same idiom as the EventBus singleton). The order below is EXACTLY
    // the original inline E-interact call sequence in
    // GameController::Update; it is load-bearing and must not be reordered
    // (a later hook may read a flag an earlier hook set, e.g.
    // TryMeetLibrarian's Flag_MetLibrarian gating downstream librarian
    // beats). Each adapter lambda drops the args its underlying free
    // function ignores, so the table stays homogeneous.
    static const std::vector<QuestHook> kHooks = [] {
        std::vector<QuestHook> v;
        v.push_back({"TryReturnVictimUmbrella",
            [](Player& p, std::string_view id, SemesterState s,
               SemesterState) { TryReturnVictimUmbrella(p, id, s); }});
        v.push_back({"TryRescueBookworm",
            [](Player& p, std::string_view id, SemesterState s,
               SemesterState) { TryRescueBookworm(p, id, s); }});
        v.push_back({"TryMeetLibrarian",
            [](Player& p, std::string_view id, SemesterState s,
               SemesterState) { TryMeetLibrarian(p, id, s); }});
        v.push_back({"TryLendLibrarianUmbrella",
            [](Player& p, std::string_view id, SemesterState s,
               SemesterState) { TryLendLibrarianUmbrella(p, id, s); }});
        v.push_back({"TryReturnLibrarianUmbrella",
            [](Player& p, std::string_view id, SemesterState s,
               SemesterState ret) {
                TryReturnLibrarianUmbrella(p, id, s, ret);
            }});
        v.push_back({"TryApplyCh2Ripple",
            [](Player& p, std::string_view id, SemesterState s,
               SemesterState) { TryApplyCh2Ripple(p, id, s); }});
        v.push_back({"TryAdvanceCh3Trade",
            [](Player& p, std::string_view id, SemesterState s,
               SemesterState) { TryAdvanceCh3Trade(p, id, s); }});
        v.push_back({"TryApplyCh3Ripple",
            [](Player& p, std::string_view, SemesterState s,
               SemesterState) { TryApplyCh3Ripple(p, s); }});
        v.push_back({"TryApplyCh4Ripple",
            [](Player& p, std::string_view id, SemesterState s,
               SemesterState) { TryApplyCh4Ripple(p, id, s); }});
        return v;
    }();
    return kHooks;
}

void RunInteractHooks(Player& player, std::string_view npcId,
                      SemesterState state, SemesterState returnTo) {
    for (const QuestHook& h : InteractQuestHooks())
        h.fn(player, npcId, state, returnTo);
}

} // namespace nccu
