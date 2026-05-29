#include "doctest/doctest.h"
#include "game/quest/Flags.h"
#include "game/dialog/DialogSource.h"
#include "game/state/SemesterState.h"

#include <vector>

#ifndef TEST_CONTENT_DIR
#error "TEST_CONTENT_DIR must be defined by the build system"
#endif

/**
 * @file test_dialog_source.cpp
 * @brief 驗證執行期的 DialogSource 供給層：英文 npcId 對應到中文區段、SemesterState
 *        對應到章節檔、各狀態的快取、找不到時回傳空且不丟例外，以及 Reload() 重建
 *        快取。預期 metadata 取自 docs/content/chapter1.md 的 Chapter1_AddDrop 內容。
 */

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

// Ch1 西裝學長解析出的四個子狀態與其 metadata 符合預期。
TEST_CASE("DialogSource: Ch1 suit_senior parity with codegen golden") {
    nccu::dialog::SetContentDir(TEST_CONTENT_DIR);

    const auto& senior =
        nccu::dialog::Entries("suit_senior",
                              SemesterState::Chapter1_AddDrop);
    REQUIRE(senior.size() == 4);

    const auto* s0 = Find(senior, 0);
    const auto* s1 = Find(senior, 1);
    const auto* s2 = Find(senior, 2);
    const auto* s3 = Find(senior, 3);
    REQUIRE(s0 != nullptr);
    REQUIRE(s1 != nullptr);
    REQUIRE(s2 != nullptr);
    REQUIRE(s3 != nullptr);

    // subState 0：開場白。choiceLabel「初次接觸」，無 karma／flag。
    CHECK(s0->choiceLabel == "初次接觸");
    CHECK(s0->karmaDelta == 0);
    CHECK(s0->setsFlag == "");
    CHECK(s0->flagValue == false);
    REQUIRE(s0->lines.size() == 5);
    CHECK(s0->lines[0] == "欸，加退選也沒搶到嗎？");

    // subState 2 (c) 接受取傘：karma 0（此舉本身中性，代價在後續 ProfTrap 的
    // 識破與連鎖），Flag_ScoldedSenior = false。
    CHECK(s2->karmaDelta == 0);
    CHECK(s2->setsFlag == nccu::kFlagScoldedSenior);
    CHECK(s2->flagValue == false);
    CHECK(s2->choiceLabel == "接受，取傘後交給學長");

    // subState 3 (d) 點破疑點：最佳的理性選項，karma +5，
    // Flag_HelpedSenior = true（第一個 Flag_ 行勝出）。
    CHECK(s3->karmaDelta == 5);
    CHECK(s3->setsFlag == nccu::kFlagHelpedSenior);
    CHECK(s3->flagValue == true);
    CHECK(s3->choiceLabel == "點破傘的疑點，轉而提供正規協助");
}

// Ch1 助教獎勵子狀態的 metadata 符合預期。
TEST_CASE("DialogSource: Ch1 ta reward substate parity") {
    nccu::dialog::SetContentDir(TEST_CONTENT_DIR);

    const auto& ta =
        nccu::dialog::Entries("ta", SemesterState::Chapter1_AddDrop);
    REQUIRE(ta.size() == 3);

    const auto* t1 = Find(ta, 1);
    REQUIRE(t1 != nullptr);
    // subState 1：karma +5，Flag_HelpedTA_Ch1 = true。
    CHECK(t1->karmaDelta == 5);
    CHECK(t1->setsFlag == nccu::kFlagHelpedTACh1);
    CHECK(t1->flagValue == true);
    CHECK(t1->choiceLabel == "玩家完成助教的跑腿請求後");
}

// 未知 npcId／未知區段時回傳空，且不丟例外。
TEST_CASE("DialogSource: unknown npcId / unknown section -> empty, no throw") {
    nccu::dialog::SetContentDir(TEST_CONTENT_DIR);

    // 在有效且內容完整的章節上查未知的英文 id。
    CHECK(nccu::dialog::Entries("does_not_exist",
                                SemesterState::Chapter1_AddDrop)
              .empty());

    // 已知 id，但該狀態的章節檔沒有此區段（結局檔不含 NPC 對話區段）。
    CHECK(nccu::dialog::Entries("suit_senior", SemesterState::Ending_A)
              .empty());
}

// Reload() 重建快取，資料不變。
TEST_CASE("DialogSource: Reload() rebuilds the cache, data unchanged") {
    nccu::dialog::SetContentDir(TEST_CONTENT_DIR);

    // 填入快取。
    const auto& before =
        nccu::dialog::Entries("suit_senior",
                              SemesterState::Chapter1_AddDrop);
    REQUIRE(before.size() == 4);
    const int karmaBefore = Find(before, 2)->karmaDelta;

    // 丟棄快取；下一次呼叫必須重新從磁碟讀取。
    nccu::dialog::Reload();

    const auto& after =
        nccu::dialog::Entries("suit_senior",
                              SemesterState::Chapter1_AddDrop);
    REQUIRE(after.size() == 4);

    // 以值比較（重新載入的章節，位址不同）。
    const auto* a0 = Find(after, 0);
    const auto* a2 = Find(after, 2);
    REQUIRE(a0 != nullptr);
    REQUIRE(a2 != nullptr);
    CHECK(a0->choiceLabel == "初次接觸");
    CHECK(a2->karmaDelta == karmaBefore);
    CHECK(a2->karmaDelta == 0);                  // (c) 現在為 0
    CHECK(a2->setsFlag == nccu::kFlagScoldedSenior);
}
