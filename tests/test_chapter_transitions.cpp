#include "doctest/doctest.h"
#include "ChapterGate.h"
#include "ChapterToast.h"
#include "DialogState.h"
#include "EndingGate.h"
#include "EventBus.h"
#include "EventWiring.h"
#include "Player.h"
#include "SemesterStateMachine.h"
#include "TrueUmbrella.h"
#include "World.h"
#include "gfx/Vec2.h"
#include <string>

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
    p.SetFlag("Flag_Ch2Cleared");
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
    p.SetFlag("Flag_Ch3Cleared");
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
        p.SetFlag("Flag_LeaveInterlude");
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
        p.SetFlag("Flag_LeaveInterlude");
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
        p.SetFlag("Flag_LeaveInterlude");
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
        p.SetFlag("Flag_HasTrueUmbrella");
        p.SetFlag("Flag_ConsoledTA");
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
        p.SetFlag("Flag_TookCursedUmbrella");
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
        p.SetFlag("Flag_BoughtUglyUmbrella");
        nccu::CheckEndingGates(p, m, d);
        CHECK(m.Current() == SemesterState::Ending_C);
        CHECK(last == "✓ 抵達結局");
    }
}

TEST_CASE("TrueUmbrella::beClaimed publishes ShowMessage BEFORE UmbrellaClaimed") {
    // Cycle 9.B follow-up to 9.A.2. Both publishes feed the same
    // single-slot HUD channel: the umbrella's own ShowMessage and the
    // chapter-clear ShowMessage emitted by the UmbrellaClaimed
    // subscriber (via PublishChapterTransitionToast). Whichever
    // publishes LAST is what the player reads. The fix swapped the
    // pair so the chapter-clear toast wins.
    //
    // This SUBCASE pattern walks the live wiring: TrueUmbrella, the
    // bus' UmbrellaClaimed subscriber (chapter-gate), and the HUD
    // subscriber that mirrors ShowMessage into World. The end state of
    // World.HudMessage() is the assertion.
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

    // After both publishes have dispatched, the HUD must carry the
    // chapter-clear toast — NOT the umbrella string. A regression that
    // re-swaps the publish order would leave the umbrella line stuck
    // on screen (the original 9.A.2 bug surfaced exactly this way).
    CHECK(w.HudMessage() == "✓ 章節清關 — 進入幕間市集");
    CHECK(w.Semester().Current() == SemesterState::Interlude_Market);
    // The umbrella line briefly transited through the channel but the
    // chapter toast overwrote it; the player saw the chapter banner.
    CHECK(w.HudMessage().find("TrueUmbrella") == std::string::npos);

    EventBus::Instance().Clear();
}

TEST_CASE("HudMessage subscriber receives the transition toast end-to-end") {
    // Combined check: when GameController-style wiring is in place, the
    // ShowMessage from a transition arrives at World.HudMessage() — the
    // surface the View reads. The diagnostic playtest read this exact
    // field and saw stale Ch1-era text on every chapter change.
    EventBus::Instance().Clear();
    World w("", /*loadSprites=*/false);
    nccu::WireHudMessageSubscriber(EventBus::Instance(), w);
    SemesterStateMachine m;
    Player p{Vec2{0, 0}};
    nccu::DialogState d;

    m.Transition(SemesterState::Chapter2_Midterms);
    p.SetFlag("Flag_Ch2Cleared");
    nccu::CheckChapterGates(p, m, d);

    CHECK(w.HudMessage() == "✓ 章節清關 — 進入幕間市集");

    EventBus::Instance().Clear();
}
