#include "doctest/doctest.h"
#include "game/quest/Flags.h"
#include "game/quest/ChapterQuestItems.h"
#include "game/quest/Chapter2Quest.h"
#include "game/entities/QuestFlagPickup.h"
#include "game/world/World.h"
#include "game/entities/Player.h"
#include "engine/core/GameObject.h"
#include "engine/events/EventBus.h"
#include "engine/math/Vec2.h"

#include <cstddef>
#include <set>
#include <string>
#include <vector>

using nccu::SemesterState;
using nccu::World;

TEST_CASE("ChapterQuestItems: Ch2 is the 3 notes; every other state empty") {
    const auto& ch2 = nccu::ChapterQuestItems(SemesterState::Chapter2_Midterms);
    REQUIRE(ch2.size() == 3);

    std::set<std::string> flags;
    for (const auto& q : ch2) {
        flags.insert(q.flag);
        CHECK(q.completionKarma == 3);              // 學霸 (b) +3
        CHECK(q.completionFlags.size() == 3);       // closes on all 3
        CHECK_FALSE(q.message.empty());
        CHECK(q.countMessages.size() == 3);         // count-based 1st/2nd/3rd
    }
    CHECK(flags == std::set<std::string>{nccu::kFlagFoundNote1,
                                         nccu::kFlagFoundNote2,
                                         nccu::kFlagFoundNote3});

    // 善有善報 redesign: Ch1 now carries exactly the 苦主's umbrella pickup
    // (single flag, no completion set, karma 0 — the +5 is on the承諾 choice
    // and the grant is its own payoff). Every other non-Ch2 state stays empty.
    const auto& ch1 = nccu::ChapterQuestItems(SemesterState::Chapter1_AddDrop);
    REQUIRE(ch1.size() == 1);
    CHECK(ch1[0].flag == nccu::kFlagHasVictimUmbrella);
    CHECK(ch1[0].completionFlags.empty());
    CHECK(ch1[0].completionKarma == 0);
    CHECK_FALSE(ch1[0].message.empty());
    CHECK(nccu::ChapterQuestItems(SemesterState::Interlude_Market).empty());
    CHECK(nccu::ChapterQuestItems(SemesterState::Chapter3_SportsDay).empty());
    CHECK(nccu::ChapterQuestItems(SemesterState::Ending_A).empty());
}

TEST_CASE("QuestFlagPickup: COUNT-based message — Nth pickup -> Nth line "
          "regardless of pickup order") {
    EventBus::Instance().Clear();
    std::string lastMsg;
    auto sub = EventBus::Instance().ScopedSubscribe(
        EventType::ShowMessage,
        [&lastMsg](const Event& e) { lastMsg = e.text; });

    const std::vector<std::string> set = {nccu::kFlagFoundNote1,
                                          nccu::kFlagFoundNote2,
                                          nccu::kFlagFoundNote3};
    const std::vector<std::string> msgs = {"first", "second", "third"};
    auto makeNote = [&](const char* flag) {
        return QuestFlagPickup(nccu::engine::math::Vec2{0, 0}, flag, "fallback",
                               set, 3, msgs);
    };

    // Pick in REVERSE identity order (note3, note2, note1). The message
    // must follow the COUNT held (1st->first, 2nd->second, 3rd->third),
    // NOT the note identity — this is the exact bug: grabbing note3 first
    // used to print the "last page" line.
    Player p{nccu::engine::math::Vec2{0.0f, 0.0f}};
    auto n3 = makeNote(nccu::kFlagFoundNote3);
    auto n2 = makeNote(nccu::kFlagFoundNote2);
    auto n1 = makeNote(nccu::kFlagFoundNote1);
    n3.OnPickup(&p);
    CHECK(lastMsg == "first");     // 1st collected -> first line
    n2.OnPickup(&p);
    CHECK(lastMsg == "second");    // 2nd collected -> second line
    n1.OnPickup(&p);
    CHECK(lastMsg == "third");     // 3rd collected -> third line

    // A different order (note2 first) lands the same count-keyed lines.
    Player q{nccu::engine::math::Vec2{0.0f, 0.0f}};
    auto m2 = makeNote(nccu::kFlagFoundNote2);
    auto m1 = makeNote(nccu::kFlagFoundNote1);
    m2.OnPickup(&q);
    CHECK(lastMsg == "first");
    m1.OnPickup(&q);
    CHECK(lastMsg == "second");

    EventBus::Instance().Clear();
}

TEST_CASE("QuestFlagPickup: empty countMessages keeps the single message") {
    EventBus::Instance().Clear();
    std::string lastMsg;
    auto sub = EventBus::Instance().ScopedSubscribe(
        EventType::ShowMessage,
        [&lastMsg](const Event& e) { lastMsg = e.text; });

    Player p{nccu::engine::math::Vec2{0.0f, 0.0f}};
    QuestFlagPickup form(nccu::engine::math::Vec2{0, 0}, nccu::kFlagFoundForm, "撿到申請書");
    form.OnPickup(&p);
    CHECK(lastMsg == "撿到申請書");   // single-message path unchanged
    EventBus::Instance().Clear();
}

TEST_CASE("QuestFlagPickup: completion karma fires once when the set closes") {
    Player p{nccu::engine::math::Vec2{0.0f, 0.0f}};
    const std::vector<std::string> set = {nccu::kFlagFoundNote1,
                                          nccu::kFlagFoundNote2,
                                          nccu::kFlagFoundNote3};
    QuestFlagPickup n1(nccu::engine::math::Vec2{0, 0}, nccu::kFlagFoundNote1,
                       "a", set, 3);
    QuestFlagPickup n2(nccu::engine::math::Vec2{0, 0}, nccu::kFlagFoundNote2,
                       "b", set, 3);
    QuestFlagPickup n3(nccu::engine::math::Vec2{0, 0}, nccu::kFlagFoundNote3,
                       "c", set, 3);

    const int k0 = p.GetKarma();
    n1.OnPickup(&p);
    CHECK(p.GetKarma() == k0);          // set not yet closed
    n2.OnPickup(&p);
    CHECK(p.GetKarma() == k0);
    n3.OnPickup(&p);
    CHECK(p.GetKarma() == k0 + 3);      // last one closes it -> +3 once

    // The default (2-arg) ctor still works and grants no bonus.
    Player q{nccu::engine::math::Vec2{0.0f, 0.0f}};
    const int qk = q.GetKarma();
    QuestFlagPickup form(nccu::engine::math::Vec2{0, 0}, nccu::kFlagFoundForm);
    form.OnPickup(&q);
    CHECK(q.HasFlag(nccu::kFlagFoundForm));
    CHECK(q.GetKarma() == qk);
}

TEST_CASE("World defers the 3 Ch2 notes until the 學霸 is woken; then sweeps") {
    EventBus::Instance().Clear();
    World w("", /*loadSprites=*/false);

    auto countNotes = [&]() -> std::size_t {
        std::size_t n = 0;
        for (const auto& o : w.Objects())
            if (dynamic_cast<const QuestFlagPickup*>(o.get())) ++n;
        return n;
    };

    // A1: Ch1 entry now spawns ONLY the ctor 申請書 (1, NOT roster-tracked).
    // The 善有善報 苦主's-umbrella pickup is DEFERRED (it appears only after
    // Flag_SuitSeniorChoiceMade, via MaybeSpawnChapter1VictimUmbrella) — so at
    // chapter entry it does NOT exist in the world (hard-gating the spine).
    CHECK(countNotes() == 1);

    // Entering Ch2 must NOT spawn the notes (the bug to fix: a note must
    // not appear anywhere before the 學霸 asks for it). Drive the FSM the
    // way production does (Transition + RespawnChapterRoster) so the
    // deferred-spawn self-gate (semester==Ch2) sees the right state. The
    // ctor 申請書 is not roster-tracked, so it persists.
    w.Semester().Transition(SemesterState::Chapter2_Midterms);
    w.RespawnChapterRoster(SemesterState::Chapter2_Midterms);
    CHECK(countNotes() == 1);                  // 申請書 only; NO notes yet

    // Without the wake flag the deferred spawn is a no-op every frame.
    CHECK_FALSE(w.MaybeSpawnChapter2Notes());
    CHECK(countNotes() == 1);

    // Wake the 學霸 -> the deferred spawn fires ONCE, dropping the 3 notes.
    REQUIRE(w.GetPlayer() != nullptr);
    w.GetPlayer()->SetFlag(nccu::kFlagBookworm);
    CHECK(w.MaybeSpawnChapter2Notes());        // spawns this frame
    CHECK(countNotes() == 1 + 3);              // 申請書 + 3 notes
    CHECK_FALSE(w.MaybeSpawnChapter2Notes());  // one-shot: never re-spawns
    CHECK(countNotes() == 1 + 3);

    // Leaving Ch2 sweeps the (deferred) notes with the roster; 申請書 stays.
    w.RespawnChapterRoster(SemesterState::Ending_A);
    CHECK(countNotes() == 1);
    EventBus::Instance().Clear();
}
