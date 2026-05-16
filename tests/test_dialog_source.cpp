#include "doctest/doctest.h"
#include "DialogSource.h"
#include "SemesterState.h"

#include <vector>

#ifndef TEST_CONTENT_DIR
#error "TEST_CONTENT_DIR must be defined by the build system"
#endif

// Verifies the runtime DialogSource provider: the English npcId ->
// Chinese section mapping, the SemesterState -> chapter file mapping,
// the per-state cache, the no-throw empty fallback, and the Reload()
// cache-rebuild path. Expected metadata is the Chapter1_AddDrop
// payload parsed from docs/content/chapter1.md.

using nccu::SemesterState;

namespace {

const nccu::dialog::SubEntry* Find(
    const std::vector<nccu::dialog::SubEntry>& subs, int subState) {
    for (const auto& s : subs) {
        if (s.subState == subState) return &s;
    }
    return nullptr;
}

}  // namespace

TEST_CASE("DialogSource: Ch1 suit_senior parity with codegen golden") {
    nccu::dialog::SetContentDir(TEST_CONTENT_DIR);

    const auto& senior =
        nccu::dialog::Entries("suit_senior",
                              SemesterState::Chapter1_AddDrop);
    REQUIRE(senior.size() == 3);

    const auto* s0 = Find(senior, 0);
    const auto* s1 = Find(senior, 1);
    const auto* s2 = Find(senior, 2);
    REQUIRE(s0 != nullptr);
    REQUIRE(s1 != nullptr);
    REQUIRE(s2 != nullptr);

    // subState 0: opener. choiceLabel "初次接觸", no karma / flag.
    CHECK(s0->choiceLabel == "初次接觸");
    CHECK(s0->karmaDelta == 0);
    CHECK(s0->setsFlag == "");
    CHECK(s0->flagValue == false);
    REQUIRE(s0->lines.size() == 5);
    CHECK(s0->lines[0] == "欸，加退選也沒搶到嗎？");

    // subState 2: karma -5, Flag_ScoldedSenior = false.
    CHECK(s2->karmaDelta == -5);
    CHECK(s2->setsFlag == "Flag_ScoldedSenior");
    CHECK(s2->flagValue == false);
    CHECK(s2->choiceLabel == "玩家接受，取傘後交給學長");
}

TEST_CASE("DialogSource: Ch1 ta reward substate parity") {
    nccu::dialog::SetContentDir(TEST_CONTENT_DIR);

    const auto& ta =
        nccu::dialog::Entries("ta", SemesterState::Chapter1_AddDrop);
    REQUIRE(ta.size() == 3);

    const auto* t1 = Find(ta, 1);
    REQUIRE(t1 != nullptr);
    // subState 1: karma +5, Flag_HelpedTA_Ch1 = true.
    CHECK(t1->karmaDelta == 5);
    CHECK(t1->setsFlag == "Flag_HelpedTA_Ch1");
    CHECK(t1->flagValue == true);
    CHECK(t1->choiceLabel == "玩家完成助教的跑腿請求後");
}

TEST_CASE("DialogSource: unknown npcId / unknown section -> empty, no throw") {
    nccu::dialog::SetContentDir(TEST_CONTENT_DIR);

    // Unknown English id on a valid, fully-authored chapter.
    CHECK(nccu::dialog::Entries("does_not_exist",
                                SemesterState::Chapter1_AddDrop)
              .empty());

    // Known id, but a state whose chapter file has no such section
    // (the ending files carry no NPC dialog sections).
    CHECK(nccu::dialog::Entries("suit_senior", SemesterState::Ending_A)
              .empty());
}

TEST_CASE("DialogSource: Reload() rebuilds the cache, data unchanged") {
    nccu::dialog::SetContentDir(TEST_CONTENT_DIR);

    // Populate the cache.
    const auto& before =
        nccu::dialog::Entries("suit_senior",
                              SemesterState::Chapter1_AddDrop);
    REQUIRE(before.size() == 3);
    const int karmaBefore = Find(before, 2)->karmaDelta;

    // Drop the cache; the next call must re-read from disk.
    nccu::dialog::Reload();

    const auto& after =
        nccu::dialog::Entries("suit_senior",
                              SemesterState::Chapter1_AddDrop);
    REQUIRE(after.size() == 3);

    // Compare by value (a fresh LoadedChapter, so addresses differ).
    const auto* a0 = Find(after, 0);
    const auto* a2 = Find(after, 2);
    REQUIRE(a0 != nullptr);
    REQUIRE(a2 != nullptr);
    CHECK(a0->choiceLabel == "初次接觸");
    CHECK(a2->karmaDelta == karmaBefore);
    CHECK(a2->karmaDelta == -5);
    CHECK(a2->setsFlag == "Flag_ScoldedSenior");
}
