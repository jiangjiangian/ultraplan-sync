#include "game/quest/QuestHookTable.h"
#include "game/quest/Chapter1Quest.h"
#include "game/quest/Chapter2Quest.h"
#include "game/quest/Chapter3Quest.h"
#include "game/quest/Chapter4Quest.h"

namespace nccu {

const std::vector<QuestHook>& InteractQuestHooks() {
    // 首次使用時建構一次（函式區域 static——具執行緒安全初始化，與 EventBus
    // 單例同一手法）。下方順序「完全等同」原本 GameController::Update 中內聯的
    // E 互動呼叫序列；此順序具關鍵作用，不得重排（後面的 hook 可能讀取前面
    // hook 設下的旗標，例如 TryMeetLibrarian 設的 Flag_MetLibrarian 會閘控
    // 後續圖書館員劇情）。每個轉接 lambda 會丟棄其底層自由函式用不到的參數，
    // 使整張表維持同質簽章。
    //
    // 每個 lambda 的第一個參數統一為 EventBus& bus（簽章一致）：會發布事件的
    // hook 將其往下傳遞，不發布的 hook 則忽略它。
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
