#include "doctest/doctest.h"
#include "ChapterQuestItems.h"
#include "Chapter2Quest.h"
#include "QuestFlagPickup.h"
#include "World.h"
#include "Player.h"
#include "GameObject.h"
#include "EventBus.h"
#include "gfx/Vec2.h"

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
    }
    CHECK(flags == std::set<std::string>{nccu::kFlagFoundNote1,
                                         nccu::kFlagFoundNote2,
                                         nccu::kFlagFoundNote3});

    CHECK(nccu::ChapterQuestItems(SemesterState::Chapter1_AddDrop).empty());
    CHECK(nccu::ChapterQuestItems(SemesterState::Interlude_Market).empty());
    CHECK(nccu::ChapterQuestItems(SemesterState::Chapter3_SportsDay).empty());
    CHECK(nccu::ChapterQuestItems(SemesterState::Ending_A).empty());
}

TEST_CASE("QuestFlagPickup: completion karma fires once when the set closes") {
    Player p{nccu::gfx::Vec2{0.0f, 0.0f}};
    const std::vector<std::string> set = {nccu::kFlagFoundNote1,
                                          nccu::kFlagFoundNote2,
                                          nccu::kFlagFoundNote3};
    QuestFlagPickup n1(nccu::gfx::Vec2{0, 0}, nccu::kFlagFoundNote1,
                       "a", set, 3);
    QuestFlagPickup n2(nccu::gfx::Vec2{0, 0}, nccu::kFlagFoundNote2,
                       "b", set, 3);
    QuestFlagPickup n3(nccu::gfx::Vec2{0, 0}, nccu::kFlagFoundNote3,
                       "c", set, 3);

    const int k0 = p.GetKarma();
    n1.OnPickup(&p);
    CHECK(p.GetKarma() == k0);          // set not yet closed
    n2.OnPickup(&p);
    CHECK(p.GetKarma() == k0);
    n3.OnPickup(&p);
    CHECK(p.GetKarma() == k0 + 3);      // last one closes it -> +3 once

    // The default (2-arg) ctor still works and grants no bonus.
    Player q{nccu::gfx::Vec2{0.0f, 0.0f}};
    const int qk = q.GetKarma();
    QuestFlagPickup form(nccu::gfx::Vec2{0, 0}, "Flag_FoundForm");
    form.OnPickup(&q);
    CHECK(q.HasFlag("Flag_FoundForm"));
    CHECK(q.GetKarma() == qk);
}

TEST_CASE("World spawns the 3 Ch2 notes; swept on the next state change") {
    EventBus::Instance().Clear();
    World w("", /*loadSprites=*/false);

    auto countNotes = [&]() -> std::size_t {
        std::size_t n = 0;
        for (const auto& o : w.Objects())
            if (dynamic_cast<const QuestFlagPickup*>(o.get())) ++n;
        return n;
    };

    // Ch1 ctor spawns exactly the 申請書 (1); ChapterQuestItems(Ch1)
    // is empty so the new spawn loop adds nothing here.
    CHECK(countNotes() == 1);

    w.RespawnChapterRoster(SemesterState::Chapter2_Midterms);
    CHECK(countNotes() == 1 + 3);              // 申請書 + 3 notes

    w.RespawnChapterRoster(SemesterState::Ending_A);
    CHECK(countNotes() == 1);                  // 3 notes swept; 申請書 stays
    EventBus::Instance().Clear();
}
