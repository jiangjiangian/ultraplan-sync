#include "doctest/doctest.h"
#include "quest/Flags.h"
#include "quest/ChapterGate.h"
#include "quest/ChapterSpawns.h"
#include "ui/ChapterToast.h"
#include "dialog/DialogState.h"
#include "state/EndingGate.h"
#include "engine/events/EventBus.h"
#include "controller/EventWiring.h"
#include "entities/GameObject.h"
#include "ui/HudSlot.h"
#include "entities/Player.h"
#include "controller/SceneRouter.h"
#include "state/SemesterStateMachine.h"
#include "entities/TrueUmbrella.h"
#include "world/World.h"
#include "engine/math/Vec2.h"
#include <algorithm>
#include <string>
#include <vector>

using nccu::SemesterState;
using nccu::SemesterStateMachine;
using nccu::World;
using nccu::gfx::Vec2;

// H2 (cycle9): every chapter / interlude / ending transition now publishes
// a ShowMessage so the player sees the FSM advance. Before this, the
// playtest log had `events=[]` at every Transition (Ch1 -> 市 -> Ch2 -> ...
// -> Ending) and the only visible HUD text was the unrelated leftover
// umbrella toast. These tests pin the exact strings emitted at each
// transition site so a future refactor that re-silences the FSM is caught.

namespace {

// Latest ShowMessage payload. A free std::string captured by ref keeps
// the lambda stable across moves (the Subscription token is movable but
// the captured ref is to the caller's stack-local string). The
// subscriber wired in production by GameController mirrors the same text
// into World::HudMessage(); using a direct subscriber here avoids
// constructing a GameController (which needs the full Input/Renderer
// stack the headless test cannot provide).
[[nodiscard]] EventBus::Subscription
SubscribeToLatest(std::string& latest) {
    return EventBus::Instance().ScopedSubscribe(
        EventType::ShowMessage,
        [&latest](const Event& e) { latest = e.text; });
}

} // namespace

TEST_CASE("ChapterTransitionToast string table covers every state") {
    using nccu::ChapterTransitionToast;
    CHECK(ChapterTransitionToast(SemesterState::Chapter1_AddDrop)   == "✓ 進入第一章 加退選");
    CHECK(ChapterTransitionToast(SemesterState::Interlude_Market)   == "✓ 章節清關 — 進入幕間市集");
    CHECK(ChapterTransitionToast(SemesterState::Chapter2_Midterms)  == "✓ 進入第二章 期中考");
    CHECK(ChapterTransitionToast(SemesterState::Chapter3_SportsDay) == "✓ 進入第三章 運動會");
    CHECK(ChapterTransitionToast(SemesterState::Chapter4_Finals)    == "✓ 進入第四章 期末考");
    CHECK(ChapterTransitionToast(SemesterState::Ending_A) == "✓ 抵達結局");
    CHECK(ChapterTransitionToast(SemesterState::Ending_B) == "✓ 抵達結局");
    CHECK(ChapterTransitionToast(SemesterState::Ending_D) == "✓ 抵達結局");   // G1
    CHECK(ChapterTransitionToast(SemesterState::Ending_C) == "✓ 抵達結局");
}

TEST_CASE("EventWiring Ch1 -> Interlude (UmbrellaClaimed) publishes toast") {
    EventBus::Instance().Clear();
    std::string last;
    auto sub = SubscribeToLatest(last);
    SemesterStateMachine m;
    std::string name;
    nccu::WireStateTransitionSubscribers(EventBus::Instance(), m, name);
    REQUIRE(m.Current() == SemesterState::Chapter1_AddDrop);

    EventBus::Instance().Publish(
        Event{EventType::UmbrellaClaimed, "TrueUmbrella"});
    CHECK(m.Current() == SemesterState::Interlude_Market);
    CHECK(last == "✓ 章節清關 — 進入幕間市集");

    EventBus::Instance().Clear();
}

TEST_CASE("ChapterGate Ch2 -> Interlude publishes toast") {
    EventBus::Instance().Clear();
    std::string last;
    auto sub = SubscribeToLatest(last);
    SemesterStateMachine m;
    Player p{Vec2{0, 0}};
    nccu::DialogState d;

    m.Transition(SemesterState::Chapter2_Midterms);
    p.SetFlag(nccu::kFlagCh2Cleared);
    nccu::CheckChapterGates(p, m, d);

    CHECK(m.Current() == SemesterState::Interlude_Market);
    CHECK(last == "✓ 章節清關 — 進入幕間市集");

    EventBus::Instance().Clear();
}

TEST_CASE("ChapterGate Ch3 -> Interlude (Flag_Ch3Cleared) publishes toast") {
    EventBus::Instance().Clear();
    std::string last;
    auto sub = SubscribeToLatest(last);
    SemesterStateMachine m;
    Player p{Vec2{0, 0}};
    nccu::DialogState d;

    m.Transition(SemesterState::Chapter3_SportsDay);
    p.SetFlag(nccu::kFlagCh3Cleared);
    nccu::CheckChapterGates(p, m, d);

    CHECK(m.Current() == SemesterState::Interlude_Market);
    CHECK(last == "✓ 章節清關 — 進入幕間市集");

    EventBus::Instance().Clear();
}

TEST_CASE("EventWiring Ch3 -> Interlude (TrueUmbrella) publishes toast") {
    EventBus::Instance().Clear();
    std::string last;
    auto sub = SubscribeToLatest(last);
    SemesterStateMachine m;
    std::string name;
    nccu::WireStateTransitionSubscribers(EventBus::Instance(), m, name);

    m.Transition(SemesterState::Chapter3_SportsDay);
    EventBus::Instance().Publish(
        Event{EventType::UmbrellaClaimed, "TrueUmbrella"});

    CHECK(m.Current() == SemesterState::Interlude_Market);
    CHECK(last == "✓ 章節清關 — 進入幕間市集");

    EventBus::Instance().Clear();
}

TEST_CASE("ChapterGate Interlude -> returnTo publishes destination toast") {
    // NOTE: the test suite's EventBus isolation reporter (see
    // test_eventbus_isolation.cpp) calls EventBus::Clear() at every
    // subcase boundary, so the subscription MUST be created inside each
    // SUBCASE — a Subscribe at TEST_CASE scope is wiped before the
    // SUBCASE body runs.

    SUBCASE("returnTo = Ch2") {
        std::string last;
        auto sub = SubscribeToLatest(last);
        SemesterStateMachine m;
        Player p{Vec2{0, 0}};
        nccu::DialogState d;
        m.Transition(SemesterState::Interlude_Market);
        m.SetInterludeReturnTo(SemesterState::Chapter2_Midterms);
        p.SetFlag(nccu::kFlagLeaveInterlude);
        nccu::CheckChapterGates(p, m, d);
        CHECK(m.Current() == SemesterState::Chapter2_Midterms);
        CHECK(last == "✓ 進入第二章 期中考");
    }
    SUBCASE("returnTo = Ch3") {
        std::string last;
        auto sub = SubscribeToLatest(last);
        SemesterStateMachine m;
        Player p{Vec2{0, 0}};
        nccu::DialogState d;
        m.Transition(SemesterState::Interlude_Market);
        m.SetInterludeReturnTo(SemesterState::Chapter3_SportsDay);
        p.SetFlag(nccu::kFlagLeaveInterlude);
        nccu::CheckChapterGates(p, m, d);
        CHECK(m.Current() == SemesterState::Chapter3_SportsDay);
        CHECK(last == "✓ 進入第三章 運動會");
    }
    SUBCASE("returnTo = Ch4") {
        std::string last;
        auto sub = SubscribeToLatest(last);
        SemesterStateMachine m;
        Player p{Vec2{0, 0}};
        nccu::DialogState d;
        m.Transition(SemesterState::Interlude_Market);
        m.SetInterludeReturnTo(SemesterState::Chapter4_Finals);
        p.SetFlag(nccu::kFlagLeaveInterlude);
        nccu::CheckChapterGates(p, m, d);
        CHECK(m.Current() == SemesterState::Chapter4_Finals);
        CHECK(last == "✓ 進入第四章 期末考");
    }
}

TEST_CASE("EndingGate Ch4 -> Ending A/B/C publishes 抵達結局 toast") {
    SUBCASE("Ending A path") {
        std::string last;
        auto sub = SubscribeToLatest(last);
        SemesterStateMachine m;
        nccu::DialogState d;
        Player p{Vec2{0, 0}};
        m.Transition(SemesterState::Chapter4_Finals);
        p.AddKarma(81 - p.GetKarma());            // > 80
        p.SetFlag(nccu::kFlagHasTrueUmbrella);
        p.SetFlag(nccu::kFlagConsoledTA);
        nccu::CheckEndingGates(p, m, d);
        CHECK(m.Current() == SemesterState::Ending_A);
        CHECK(last == "✓ 抵達結局");
    }
    SUBCASE("Ending B path (cursed umbrella)") {
        std::string last;
        auto sub = SubscribeToLatest(last);
        SemesterStateMachine m;
        nccu::DialogState d;
        Player p{Vec2{0, 0}};
        m.Transition(SemesterState::Chapter4_Finals);
        p.SetFlag(nccu::kFlagTookCursedUmbrella);
        nccu::CheckEndingGates(p, m, d);
        CHECK(m.Current() == SemesterState::Ending_B);
        CHECK(last == "✓ 抵達結局");
    }
    SUBCASE("Ending C path (ugly umbrella)") {
        std::string last;
        auto sub = SubscribeToLatest(last);
        SemesterStateMachine m;
        nccu::DialogState d;
        Player p{Vec2{0, 0}};
        m.Transition(SemesterState::Chapter4_Finals);
        p.SetFlag(nccu::kFlagBoughtUglyUmbrella);
        nccu::CheckEndingGates(p, m, d);
        CHECK(m.Current() == SemesterState::Ending_C);
        CHECK(last == "✓ 抵達結局");
    }
}

TEST_CASE("TrueUmbrella::beClaimed: chapter toast Top, pickup line Bottom") {
    // Cycle 9.G follow-up to 9.B / 9.A.2. Pre-9.G, both publishes fed a
    // single-slot HUD channel: the umbrella's own ShowMessage and the
    // chapter-clear ShowMessage emitted by the UmbrellaClaimed
    // subscriber (via PublishChapterTransitionToast). Whichever
    // published last was what the player saw — Plan A (9.B) swapped
    // the order so chapter-clear won, but the umbrella pickup line
    // became visible for 0 frames.
    //
    // Plan B (cycle9f §B): chapter-clear publishes on HudSlot::Top;
    // the umbrella pickup line stays on HudSlot::Bottom. Both bands
    // are live at once, and BOTH lines are visible. This SUBCASE walks
    // the live wiring (TrueUmbrella → bus' UmbrellaClaimed subscriber
    // → chapter-gate → HUD subscriber) and asserts each slot carries
    // its expected text — the regression net for re-collapsing the two
    // channels into one.
    EventBus::Instance().Clear();
    nccu::World w("", /*loadSprites=*/false);
    std::string name;
    nccu::WireStateTransitionSubscribers(EventBus::Instance(),
                                          w.Semester(), name);
    nccu::WireHudMessageSubscriber(EventBus::Instance(), w);

    // Pre-condition: semester is on Chapter 1 (the EventWiring
    // sibling-if only fires from Chapter1_AddDrop).
    REQUIRE(w.Semester().Current() == SemesterState::Chapter1_AddDrop);

    TrueUmbrella umb{Vec2{0, 0}};
    Player player{Vec2{0, 0}};
    umb.beClaimed(&player);

    // Two-channel post-conditions: Top carries the chapter-clear toast,
    // Bottom carries the umbrella pickup text. Neither overwrites the
    // other — a regression that re-merged the slots would surface as a
    // missing line in one of the two channels.
    CHECK(w.HudMessage(nccu::HudSlot::Top) == "✓ 章節清關 — 進入幕間市集");
    CHECK(w.HudMessage(nccu::HudSlot::Bottom).find("TrueUmbrella")
          != std::string::npos);
    CHECK(w.Semester().Current() == SemesterState::Interlude_Market);
    // Cross-channel non-leak: each slot is exactly one thing.
    CHECK(w.HudMessage(nccu::HudSlot::Top).find("TrueUmbrella")
          == std::string::npos);
    CHECK(w.HudMessage(nccu::HudSlot::Bottom).find("章節清關")
          == std::string::npos);

    EventBus::Instance().Clear();
}

TEST_CASE("HudMessage subscriber receives the transition toast end-to-end") {
    // Combined check: when GameController-style wiring is in place, the
    // ShowMessage from a transition arrives at the World's Top HUD slot
    // — the surface the View reads. The diagnostic playtest read this
    // exact field and saw stale Ch1-era text on every chapter change.
    // Cycle 9.G: chapter toasts route to HudSlot::Top so they survive
    // a same-frame Bottom-slot ShowMessage (arrival hint, karma, pickup).
    EventBus::Instance().Clear();
    World w("", /*loadSprites=*/false);
    nccu::WireHudMessageSubscriber(EventBus::Instance(), w);
    SemesterStateMachine m;
    Player p{Vec2{0, 0}};
    nccu::DialogState d;

    m.Transition(SemesterState::Chapter2_Midterms);
    p.SetFlag(nccu::kFlagCh2Cleared);
    nccu::CheckChapterGates(p, m, d);

    CHECK(w.HudMessage(nccu::HudSlot::Top) == "✓ 章節清關 — 進入幕間市集");
    // Bottom slot stays empty — no Bottom publisher fired in this path.
    CHECK(w.HudMessage(nccu::HudSlot::Bottom).empty());

    EventBus::Instance().Clear();
}

// Cycle 10.P0b L8 regression: every chapter/interlude/ending transition
// rolls the roster into the visible npcs[] list on the SAME frame the
// FSM moves. Pre-Cycle 10 the roster swap ran at the TOP of the next
// Update(), so the state.jsonl line for transition frame N had
// semester=NEW but npcs[]=OLD (the cycle9f §G.1 / §J diagnosis: all 7
// spine transitions affected, repro in every one of the three ending
// runs). The SceneRouter::SettleRoster end-of-Update call closes that
// window. This test pins the contract using SceneRouter directly (no
// View / Harness needed) so a future change that reverts to the
// 1-frame lag (or makes it 2-frame, etc.) fails here.
TEST_CASE("L8: roster follows FSM on the same frame as Transition()") {
    nccu::World w("", /*loadSprites=*/false);
    nccu::SceneRouter r{w.Semester().Current()};

    // Helper to count NPCs whose npcId is in a given chapter's roster.
    // Mimics state.jsonl's npcs[] field (only non-empty NpcIds).
    auto npcIds = [&w]() {
        std::vector<std::string> out;
        for (const auto& o : w.Objects()) {
            if (!o || !o->IsActive()) continue;
            const std::string id{o->NpcId()};
            if (!id.empty()) out.push_back(id);
        }
        return out;
    };

    // ----- Tick N-1: FSM at Ch1, roster is Ch1 (5 archetypes). -----
    auto ids = npcIds();
    CHECK(std::find(ids.begin(), ids.end(), "victim") != ids.end());

    // ----- Tick N: a Transition fires mid-frame; SettleRoster runs at
    // end-of-Update. Same call site as GameController's end-of-Update
    // call. -----
    w.Semester().Transition(SemesterState::Chapter2_Midterms);
    r.SettleRoster(w);

    // Within this tick's eventual View::Draw / state.jsonl dump, the
    // roster MUST already be the new chapter's. Pre-fix this was the
    // PREVIOUS chapter's, by design accident — the very bug L8.
    ids = npcIds();
    CHECK(std::find(ids.begin(), ids.end(), "librarian") != ids.end());
    // Ch1 and Ch2 share victim/suit_senior/bookworm/ta/shop_auntie;
    // librarian appears only in Ch2 (the chapter-specific marker).

    // Same one-call-per-transition contract: a second SettleRoster on
    // the same FSM state is a no-op (idempotent under its own cursor).
    const std::size_t countAfter = ids.size();
    r.SettleRoster(w);
    ids = npcIds();
    CHECK(ids.size() == countAfter);
}

TEST_CASE("L8: every transition closes its npcs[] lag (full spine)") {
    // Walk the seven-transition spine — Ch1 -> Interlude -> Ch2 ->
    // Interlude -> Ch3 -> Interlude -> Ch4 -> Ending_A — calling
    // SettleRoster at every step exactly as the production
    // end-of-Update path does. Each step must surface the destination
    // state's roster on the same tick.
    nccu::World w("", /*loadSprites=*/false);
    nccu::SceneRouter r{w.Semester().Current()};

    const SemesterState path[] = {
        SemesterState::Interlude_Market,
        SemesterState::Chapter2_Midterms,
        SemesterState::Interlude_Market,
        SemesterState::Chapter3_SportsDay,
        SemesterState::Interlude_Market,
        SemesterState::Chapter4_Finals,
        SemesterState::Ending_A,
    };

    for (SemesterState s : path) {
        w.Semester().Transition(s);
        r.SettleRoster(w);

        // Each transition lands its destination roster on this tick.
        const auto& expected = nccu::ChapterNpcSpawns(s);
        std::size_t hits = 0;
        for (const auto& o : w.Objects()) {
            if (!o || !o->IsActive()) continue;
            const std::string id{o->NpcId()};
            if (id.empty()) continue;
            for (const auto& sp : expected)
                if (sp.npcId == id) { ++hits; break; }
        }
        CHECK_MESSAGE(hits == expected.size(),
                      "transition to " << static_cast<int>(s)
                      << " left npcs[] missing roster entries");

        // No leftover NPCs from a previous chapter. For each observed
        // NPC, it must be in the new chapter's spawn table.
        for (const auto& o : w.Objects()) {
            if (!o || !o->IsActive()) continue;
            const std::string id{o->NpcId()};
            if (id.empty()) continue;
            bool inExpected = false;
            for (const auto& sp : expected)
                if (sp.npcId == id) { inExpected = true; break; }
            CHECK_MESSAGE(inExpected,
                          "transition to " << static_cast<int>(s)
                          << " leaked stale NPC: " << id);
        }
    }
}
