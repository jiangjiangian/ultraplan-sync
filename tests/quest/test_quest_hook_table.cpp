#include "doctest/doctest.h"
#include "engine/events/EventBus.h"
#include "game/quest/QuestHookTable.h"
#include "game/entities/Player.h"
#include "engine/math/Vec2.h"

#include <string_view>
#include <vector>

// The E-interact quest hooks are now a registered table (QuestHookTable)
// instead of ~14 hard-coded TryXxx calls inlined in
// GameController::Update. These tests pin the contract the byte-identical
// state.jsonl gate depends on:
//   1. The table's CONTENT and ORDER match the original inline sequence
//      exactly (the order is load-bearing — a later hook may read a flag an
//      earlier hook set).
//   2. RunInteractHooks walks the whole table and every hook self-gates, so
//      running it for an npcId/state that matches NO hook mutates nothing.

using nccu::InteractQuestHooks;
using nccu::QuestHook;
using nccu::RunInteractHooks;
using nccu::SemesterState;

TEST_CASE("InteractQuestHooks: registered order matches the original inline sequence") {
    const std::vector<QuestHook>& hooks = InteractQuestHooks();
    REQUIRE(hooks.size() == 9);
    CHECK(hooks[0].name == std::string_view("TryReturnVictimUmbrella"));
    CHECK(hooks[1].name == std::string_view("TryRescueBookworm"));
    CHECK(hooks[2].name == std::string_view("TryMeetLibrarian"));
    CHECK(hooks[3].name == std::string_view("TryLendLibrarianUmbrella"));
    CHECK(hooks[4].name == std::string_view("TryReturnLibrarianUmbrella"));
    CHECK(hooks[5].name == std::string_view("TryApplyCh2Ripple"));
    CHECK(hooks[6].name == std::string_view("TryAdvanceCh3Trade"));
    CHECK(hooks[7].name == std::string_view("TryApplyCh3Ripple"));
    CHECK(hooks[8].name == std::string_view("TryApplyCh4Ripple"));
}

TEST_CASE("InteractQuestHooks: every entry carries a callable") {
    for (const QuestHook& h : InteractQuestHooks()) {
        CHECK(static_cast<bool>(h.fn));
    }
}

TEST_CASE("RunInteractHooks: a non-matching (npcId, state) mutates nothing") {
    // In Ch1 with an unknown npcId none of the hooks' (state, npcId) guards
    // fire, so the whole table is a no-op — karma / money / flags untouched.
    Player p{nccu::engine::math::Vec2{0, 0}};
    const int    karma0 = p.GetKarma();
    const int    money0 = p.GetMoney();
    const bool   umb0   = p.HasUmbrella();
    RunInteractHooks(EventBus::Instance(), p, "no_such_npc", SemesterState::Chapter1_AddDrop,
                     SemesterState::Chapter2_Midterms);
    CHECK(p.GetKarma()    == karma0);
    CHECK(p.GetMoney()    == money0);
    CHECK(p.HasUmbrella() == umb0);
}

TEST_CASE("RunInteractHooks: the table is a stable singleton (same instance)") {
    // Built once on first use; repeat calls hand back the same vector so
    // the order can never drift between frames.
    CHECK(&InteractQuestHooks() == &InteractQuestHooks());
}
