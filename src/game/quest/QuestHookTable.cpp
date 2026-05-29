#include "game/quest/QuestHookTable.h"
#include "game/quest/Chapter1Quest.h"
#include "game/quest/Chapter2Quest.h"
#include "game/quest/Chapter3Quest.h"
#include "game/quest/Chapter4Quest.h"

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
    //
    // Plan P2 step 2: each lambda now takes EventBus& bus as its first
    // parameter (uniform signature). Publishing hooks forward it; non-
    // publishing hooks ignore it.
    static const std::vector<QuestHook> kHooks = [] {
        std::vector<QuestHook> v;
        v.push_back({"TryReturnVictimUmbrella",
            [](EventBus& bus, Player& p, std::string_view id, SemesterState s,
               SemesterState) { TryReturnVictimUmbrella(bus, p, id, s); }});
        v.push_back({"TryRescueBookworm",
            [](EventBus& bus, Player& p, std::string_view id, SemesterState s,
               SemesterState) { TryRescueBookworm(bus, p, id, s); }});
        v.push_back({"TryMeetLibrarian",
            [](EventBus&, Player& p, std::string_view id, SemesterState s,
               SemesterState) { TryMeetLibrarian(p, id, s); }});
        v.push_back({"TryLendLibrarianUmbrella",
            [](EventBus&, Player& p, std::string_view id, SemesterState s,
               SemesterState) { TryLendLibrarianUmbrella(p, id, s); }});
        v.push_back({"TryReturnLibrarianUmbrella",
            [](EventBus& bus, Player& p, std::string_view id, SemesterState s,
               SemesterState ret) {
                TryReturnLibrarianUmbrella(bus, p, id, s, ret);
            }});
        v.push_back({"TryApplyCh2Ripple",
            [](EventBus&, Player& p, std::string_view id, SemesterState s,
               SemesterState) { TryApplyCh2Ripple(p, id, s); }});
        v.push_back({"TryAdvanceCh3Trade",
            [](EventBus& bus, Player& p, std::string_view id, SemesterState s,
               SemesterState) { TryAdvanceCh3Trade(bus, p, id, s); }});
        v.push_back({"TryApplyCh3Ripple",
            [](EventBus& bus, Player& p, std::string_view, SemesterState s,
               SemesterState) { TryApplyCh3Ripple(bus, p, s); }});
        v.push_back({"TryApplyCh4Ripple",
            [](EventBus&, Player& p, std::string_view id, SemesterState s,
               SemesterState) { TryApplyCh4Ripple(p, id, s); }});
        return v;
    }();
    return kHooks;
}

void RunInteractHooks(EventBus& bus, Player& player, std::string_view npcId,
                      SemesterState state, SemesterState returnTo) {
    for (const QuestHook& h : InteractQuestHooks())
        h.fn(bus, player, npcId, state, returnTo);
}

} // namespace nccu
