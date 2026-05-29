#include "doctest/doctest.h"
#include "game/quest/Flags.h"
#include "ui/ChapterToast.h"
#include "engine/events/EventBus.h"
#include "game/controller/EventWiring.h"
#include "game/state/InterludeExit.h"
#include "game/entities/Player.h"
#include "game/controller/SceneRouter.h"
#include "game/quest/ItemCatalog.h"
#include "game/world/World.h"
#include "engine/math/Vec2.h"

#include <algorithm>
#include <string>
#include <string_view>
#include <vector>

using nccu::SceneRouter;
using nccu::SemesterState;
using nccu::World;

// Cycle 10.P0a (awsome_cpp.md §6) + 10.P0b (L8 fix): SceneRouter owns
// the chapter/interlude/ending transition observer that used to live
// inline on GameController. The fix splits the observation into two
// halves so the visible bug (npcs[] list lagging by 1 frame) closes
// without touching the harness state.jsonl observable timeline:
//
//   SettleRoster        — end-of-Update; roster swap ONLY.
//   SettleSideEffects   — top-of-Update; player pos / consumables / events.
//
// These tests pin the contract:
//   1. SettleRoster on a state change actually respawns the NPCs.
//   2. SettleRoster is idempotent on its own cursor.
//   3. SettleSideEffects on Interlude entry moves the player, wipes
//      consumables, publishes the arrival hint, resets the exit latch.
//   4. SettleSideEffects on Ch4 entry clears the umbrella state.
//   5. Calling SettleRoster THEN SettleSideEffects in the same tick
//      keeps the side-effect half byte-equivalent to the pre-Cycle 10
//      single-block behaviour — no double-spawn, no double-publish.

namespace {

bool HasNpcId(const World& w, const char* id) {
    for (const auto& o : w.Objects())
        if (o->NpcId() == std::string_view(id)) return true;
    return false;
}

// Latest ShowMessage payload — used to verify the arrival hint
// publishes (or doesn't) on the right call.
[[nodiscard]] EventBus::Subscription
SubscribeToLatest(std::string& latest) {
    return EventBus::Instance().ScopedSubscribe(
        EventType::ShowMessage,
        [&latest](const Event& e) { latest = e.text; });
}

} // namespace

TEST_CASE("SceneRouter ctor: cursors start at the initial state") {
    SceneRouter r{SemesterState::Chapter1_AddDrop};
    CHECK(r.LastRosterState() == SemesterState::Chapter1_AddDrop);
    CHECK(r.LastRosterRespawnState() == SemesterState::Chapter1_AddDrop);
    CHECK(r.InterludeExitLatchMut() == false);
}

TEST_CASE("SettleRoster: no-op when the FSM hasn't moved") {
    World w("", /*loadSprites=*/false);
    SceneRouter r{w.Semester().Current()};
    REQUIRE(HasNpcId(w, "victim"));        // Ch1 roster present

    r.SettleRoster(w);                     // FSM still Ch1
    CHECK(HasNpcId(w, "victim"));          // unchanged
    CHECK(r.LastRosterRespawnState() == SemesterState::Chapter1_AddDrop);
}

TEST_CASE("SettleRoster: respawns the new chapter's NPCs on a transition") {
    World w("", /*loadSprites=*/false);
    SceneRouter r{w.Semester().Current()};
    REQUIRE(HasNpcId(w, "victim"));        // Ch1 NPC

    // FSM advances WITHOUT going through the SceneRouter (mimics a
    // CheckChapterGates / EventWiring transition firing during the
    // controller's Update body).
    w.Semester().Transition(SemesterState::Chapter2_Midterms);
    r.SettleRoster(w);

    // Ch2 roster now visible to the View (and state.jsonl npcs[]) on
    // the SAME frame the FSM moved. Pre-fix this was 1 frame late.
    CHECK(HasNpcId(w, "librarian"));       // Ch2-only quest-giver
    CHECK(HasNpcId(w, "victim"));          // victim is Ch2 too
    CHECK(r.LastRosterRespawnState() == SemesterState::Chapter2_Midterms);
}

TEST_CASE("SettleRoster: SettleSideEffects cursor untouched (split is real)") {
    World w("", /*loadSprites=*/false);
    SceneRouter r{w.Semester().Current()};

    w.Semester().Transition(SemesterState::Chapter2_Midterms);
    r.SettleRoster(w);

    // The "side-effect cursor" must NOT have advanced — it tracks the
    // top-of-Update half, which the test hasn't invoked yet. If both
    // cursors moved together, the next SettleSideEffects call would
    // be a no-op and the player would never be repositioned.
    CHECK(r.LastRosterRespawnState() == SemesterState::Chapter2_Midterms);
    CHECK(r.LastRosterState()        == SemesterState::Chapter1_AddDrop);
}

TEST_CASE("SettleSideEffects Interlude entry: pos, consumables, hint, latch") {
    EventBus::Instance().Clear();
    std::string latestHud;
    auto sub = SubscribeToLatest(latestHud);

    World w("", /*loadSprites=*/false);
    SceneRouter r{w.Semester().Current()};

    Player* p = w.GetPlayer();
    REQUIRE(p != nullptr);

    // Pre-state: pretend the player wandered far from the IL entry,
    // carries a Ch1 consumable, and the south-band latch fired earlier.
    p->SetPosition(nccu::gfx::Vec2{100.0f, 100.0f});
    p->AddConsumable("EnergyDrink");
    p->AddConsumable("EnergyDrink");
    r.InterludeExitLatchMut() = true;
    REQUIRE(p->ConsumableCount("EnergyDrink") == 2);

    // FSM advances to Interlude (mimics the EventWiring Ch1→IL hop).
    w.Semester().Transition(SemesterState::Interlude_Market);
    r.SettleSideEffects(w);

    // All four observable side effects fire together:
    CHECK(p->GetPosition().x == doctest::Approx(nccu::kInterludeEntry.x));
    CHECK(p->GetPosition().y == doctest::Approx(nccu::kInterludeEntry.y));
    CHECK(p->ConsumableCount("EnergyDrink") == 0);     // wiped
    CHECK(latestHud == nccu::kInterludeArrivalHint);
    CHECK(r.InterludeExitLatchMut() == false);

    // Cursor stamped so a re-call is a no-op.
    CHECK(r.LastRosterState() == SemesterState::Interlude_Market);
    p->SetPosition(nccu::gfx::Vec2{42.0f, 42.0f});
    r.SettleSideEffects(w);                            // idempotent
    CHECK(p->GetPosition().x == doctest::Approx(42.0f));

    EventBus::Instance().Clear();
}

TEST_CASE("SettleSideEffects Ch4 entry: umbrella + TrueUmbrella flag reset") {
    EventBus::Instance().Clear();

    World w("", /*loadSprites=*/false);
    SceneRouter r{w.Semester().Current()};

    Player* p = w.GetPlayer();
    REQUIRE(p != nullptr);
    p->SetHasUmbrella(true);
    p->SetFlag(nccu::kFlagHasTrueUmbrella);

    // FSM jumps straight to Ch4 (mimics ChapterGate Interlude→Ch4).
    w.Semester().Transition(SemesterState::Chapter4_Finals);
    r.SettleSideEffects(w);

    // chapter4.md L6「傘再度失蹤」: the player walks out of 集英樓 with
    // no umbrella. The fix's Ch4 branch is the GDD enforcement.
    CHECK_FALSE(p->HasUmbrella());
    CHECK_FALSE(p->HasFlag(nccu::kFlagHasTrueUmbrella));
    CHECK(r.LastRosterState() == SemesterState::Chapter4_Finals);

    EventBus::Instance().Clear();
}

// B4: the per-chapter「傘又掉了」card is now MECHANICALLY TRUE. The held
// umbrella (the bug: 真傘 still in the Ch2 bag — it persisted because the
// reset was Ch4-ONLY) is now cleared on Ch2 AND Ch3 entry too, mirroring
// the Ch4 block. So entering ANY of Ch2/Ch3/Ch4 leaves the player
// umbrella-less and the bag's umbrella row gone (SetHasUmbrella(false) also
// empties the held-kind slot).
TEST_CASE("B4: SettleSideEffects clears the held umbrella on Ch2 entry") {
    EventBus::Instance().Clear();
    World w("", /*loadSprites=*/false);
    SceneRouter r{w.Semester().Current()};
    Player* p = w.GetPlayer();
    REQUIRE(p != nullptr);

    // Pretend the player carried a held umbrella out of Ch1 (e.g. the真傘
    // the 苦主 handed back, or the Ch1 阿姨 ugly umbrella) into the bag.
    p->SetHeldUmbrella(HeldUmbrella::True);
    p->SetFlag(nccu::kFlagHasTrueUmbrella);
    REQUIRE(p->HeldUmbrellaKind() == HeldUmbrella::True);

    w.Semester().Transition(SemesterState::Chapter2_Midterms);
    r.SettleSideEffects(w);

    CHECK_FALSE(p->HasUmbrella());
    CHECK(p->HeldUmbrellaKind() == HeldUmbrella::None);   // bag umbrella gone
    CHECK_FALSE(p->HasFlag(nccu::kFlagHasTrueUmbrella));
    EventBus::Instance().Clear();
}

TEST_CASE("B4: SettleSideEffects clears the held umbrella on Ch3 entry") {
    EventBus::Instance().Clear();
    World w("", /*loadSprites=*/false);
    SceneRouter r{w.Semester().Current()};
    Player* p = w.GetPlayer();
    REQUIRE(p != nullptr);

    // Ch3 starts after Ch2; pretend a Ch2 loaner umbrella lingered.
    p->SetHeldUmbrella(HeldUmbrella::Loaner);
    REQUIRE(p->HeldUmbrellaKind() == HeldUmbrella::Loaner);

    w.Semester().Transition(SemesterState::Chapter3_SportsDay);
    r.SettleSideEffects(w);

    CHECK_FALSE(p->HasUmbrella());
    CHECK(p->HeldUmbrellaKind() == HeldUmbrella::None);   // loaner gone
    EventBus::Instance().Clear();
}

TEST_CASE("L8 fix end-to-end: SettleRoster THEN SettleSideEffects on a tick") {
    // Pin the full Cycle 10.P0b flow: a transition fires mid-frame
    // (after dialog branch / E-probe / CheckChapterGates / etc.); the
    // controller calls SettleRoster at end-of-Update so the View's
    // upcoming Draw paints with the new chapter's NPCs. On the NEXT
    // frame's top-of-Update, SettleSideEffects fires the player-pos
    // teleport + arrival hint. Both halves run exactly once per
    // transition (idempotent under their own cursors).
    EventBus::Instance().Clear();
    std::string latestHud;
    auto sub = SubscribeToLatest(latestHud);

    World w("", /*loadSprites=*/false);
    SceneRouter r{w.Semester().Current()};
    Player* p = w.GetPlayer();
    REQUIRE(p != nullptr);

    // Simulate the umbrella-claim transition without driving the full
    // event bus: just move the FSM (the SceneRouter cares about the
    // FSM state only — it doesn't poll the EventBus).
    w.Semester().Transition(SemesterState::Interlude_Market);

    // ---- Tick N: end-of-Update SettleRoster ----
    REQUIRE(HasNpcId(w, "victim"));        // Ch1 NPCs still present pre-call
    r.SettleRoster(w);
    CHECK_FALSE(HasNpcId(w, "victim"));    // IL roster is empty
    // No player teleport yet — the harness observable is unchanged.
    // (The test doesn't move the player itself; we just verify the
    // SettleRoster pass did NOT publish the arrival hint or wipe
    // consumables — that's SettleSideEffects' job on the NEXT tick.)
    CHECK(latestHud.empty());

    // ---- Tick N+1: top-of-Update SettleSideEffects ----
    r.SettleSideEffects(w);
    CHECK(p->GetPosition().x == doctest::Approx(nccu::kInterludeEntry.x));
    CHECK(p->GetPosition().y == doctest::Approx(nccu::kInterludeEntry.y));
    CHECK(latestHud == nccu::kInterludeArrivalHint);

    // Both halves idempotent on a repeat call.
    p->SetPosition(nccu::gfx::Vec2{7.0f, 8.0f});
    latestHud.clear();
    r.SettleRoster(w);
    r.SettleSideEffects(w);
    CHECK(p->GetPosition().x == doctest::Approx(7.0f));  // not re-teleported
    CHECK(latestHud.empty());                            // no re-publish

    EventBus::Instance().Clear();
}

TEST_CASE("SettleSideEffects defensively respawns the roster if SettleRoster was skipped") {
    // The Cycle 10 split keeps both halves runnable individually so
    // callers that bypass SettleRoster (e.g. tests, or a future code
    // path that only goes through the top-of-Update branch) still get
    // a coherent roster. SettleSideEffects calls RespawnChapterRoster
    // when its respawn cursor disagrees with the FSM state.
    EventBus::Instance().Clear();

    World w("", /*loadSprites=*/false);
    SceneRouter r{w.Semester().Current()};
    REQUIRE(HasNpcId(w, "victim"));        // Ch1

    // FSM moves without going through SettleRoster first.
    w.Semester().Transition(SemesterState::Chapter2_Midterms);
    r.SettleSideEffects(w);

    // Roster is now Ch2 — the defensive RespawnChapterRoster fired.
    CHECK(HasNpcId(w, "librarian"));       // Ch2 NPC
    CHECK(r.LastRosterRespawnState() == SemesterState::Chapter2_Midterms);
    CHECK(r.LastRosterState() == SemesterState::Chapter2_Midterms);

    EventBus::Instance().Clear();
}

// B4: cross-Interlude bag survivors. After Ch1 → Interlude → Ch2, the ONLY
// rows that may remain are 金幣 (cross-chapter money) and 申請書
// (Flag_FoundForm — the cross-chapter carried item the TA arc relies on).
// Consumables are wiped on market entry (ClearConsumables) and the held
// umbrella is cleared on Ch2 entry (the B4 fix), so neither survives. This
// pins "no other stale carry-over" through the real SceneRouter resets +
// BuildInventoryRows.
TEST_CASE("B4: across the Interlude the bag carries only money + 申請書") {
    EventBus::Instance().Clear();
    World w("", /*loadSprites=*/false);
    SceneRouter r{w.Semester().Current()};
    Player* p = w.GetPlayer();
    REQUIRE(p != nullptr);

    // End-of-Ch1 bag: money (persists), the form (the carried item), a held
    // umbrella (the 苦主's真傘), and a Ch1 consumable bought in the market.
    p->SetFlag(nccu::kFlagFoundForm);
    p->SetHeldUmbrella(HeldUmbrella::True);
    p->SetFlag(nccu::kFlagHasTrueUmbrella);
    p->AddConsumable("EnergyDrink");
    {
        const auto rows = nccu::BuildInventoryRows(*p);
        // sanity: pre-transition the bag DOES hold the umbrella + consumable
        bool hasUmb = false, hasDrink = false;
        for (const auto& row : rows) {
            if (row.itemId == nccu::kItemTrueUmbrella) hasUmb = true;
            if (row.itemId == "EnergyDrink")           hasDrink = true;
        }
        REQUIRE(hasUmb);
        REQUIRE(hasDrink);
    }

    // Interlude entry wipes consumables (+ repositions, hint, latch).
    w.Semester().Transition(SemesterState::Interlude_Market);
    r.SettleSideEffects(w);
    // Ch2 entry clears the held umbrella (the B4 reset).
    w.Semester().Transition(SemesterState::Chapter2_Midterms);
    r.SettleSideEffects(w);

    const auto rows = nccu::BuildInventoryRows(*p);
    // EXACTLY two survivor categories, in the deterministic catalog order:
    // money first, then the form. Nothing else.
    std::vector<std::string> ids;
    for (const auto& row : rows) ids.push_back(row.itemId);
    CHECK(ids == std::vector<std::string>{nccu::kItemMoney, nccu::kItemForm});
    // Explicit negatives: no umbrella row, no consumable row.
    CHECK(std::none_of(rows.begin(), rows.end(), [](const nccu::InventoryRow& row) {
        return row.itemId.find("umbrella") != std::string::npos;
    }));
    CHECK(p->ConsumableCount("EnergyDrink") == 0);

    EventBus::Instance().Clear();
}
