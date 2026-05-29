#ifndef QUEST_HOOK_TABLE_H_
#define QUEST_HOOK_TABLE_H_
#include "game/state/SemesterState.h"
#include <functional>
#include <string_view>
#include <vector>

class Player;
class EventBus;     // Plan P2 step 2: hooks may publish — bus threaded through

namespace nccu {

// The Ch1-Ch4 E-interact quest hooks, registered as a table instead of
// ~14 hard-coded TryXxx(npcId, state) calls inlined in
// GameController::Update (awsome_cpp.md §7 Open-Closed: "加新型別就要修現有
// switch" is the smell this removes). Each hook is run, IN REGISTERED
// ORDER, the moment the player presses E on a non-flavor NPC — BEFORE the
// dialog opener. Every hook SELF-GATES on (state, npcId) and is a cheap
// no-op outside its chapter / NPC, so running the whole table on every
// interact is correct and order-stable.
//
// Adding a chapter/NPC is now DATA (one RegisterHook line in
// QuestHookTable.cpp) rather than an edit to the controller's god-method.
// The registered order is load-bearing (it mirrors the original inline
// call order exactly, so the self-gating semantics and any cross-hook
// ordering — e.g. a librarian-meet flag that a later hook reads — are
// byte-preserved); RunInteractHooks walks the table front-to-back.
//
// Pure model logic: a hook mutates the Player (and reads the FSM state).
// No raylib, no input, no rendering — those stay in the controller/View.

// Uniform hook signature. The hooks have three original shapes:
//   * f(player, npcId, state)        — the common case
//   * f(player, npcId, state, ret)   — TryReturnLibrarianUmbrella needs the
//                                      Interlude return-target to scope
//                                      itself to the Ch2->Ch3 market
//   * f(player, state)               — TryApplyCh3Ripple is npcId-agnostic
// They are all adapted to ONE arity here so the table is homogeneous; an
// adapter lambda drops the args a given hook ignores. `returnTo` is
// World::Semester().InterludeReturnTo(), forwarded every call.
// Plan P2 step 2: `bus` is the first parameter so the publishing hooks
// (TryReturnVictimUmbrella / TryRescueBookworm / TryReturnLibrarianUmbrella /
// TryAdvanceCh3Trade / TryApplyCh3Ripple) can route their ShowMessage
// through the injected bus rather than the global Instance(). Non-
// publishing hooks ignore it (their adapter lambdas drop the arg).
using QuestHookFn = std::function<void(EventBus& bus,
                                       Player& player,
                                       std::string_view npcId,
                                       SemesterState state,
                                       SemesterState returnTo)>;

struct QuestHook {
    std::string_view name;   // for tests / future logging; not behavioural
    QuestHookFn       fn;
};

// Build the ordered hook table ONCE. The order is exactly the original
// inline E-interact sequence:
//   TryReturnVictimUmbrella, TryRescueBookworm, TryMeetLibrarian,
//   TryLendLibrarianUmbrella, TryReturnLibrarianUmbrella, TryApplyCh2Ripple,
//   TryAdvanceCh3Trade, TryApplyCh3Ripple, TryApplyCh4Ripple.
// (OpenNpcDialog is NOT a hook — it opens the dialog box AFTER the table
// runs, so it stays in the controller's interact dispatch.)
[[nodiscard]] const std::vector<QuestHook>& InteractQuestHooks();

// Run every hook in registered order. Self-gating inside each hook means
// only the ones whose (state, npcId) match do anything. Free function so a
// doctest can drive the full table against a Player without the controller.
void RunInteractHooks(EventBus& bus, Player& player, std::string_view npcId,
                      SemesterState state, SemesterState returnTo);

} // namespace nccu

#endif // QUEST_HOOK_TABLE_H_
