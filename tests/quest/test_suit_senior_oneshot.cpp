/**
 * @file test_suit_senior_oneshot.cpp
 * @brief 驗證西裝學長的分支選單只能進入一次：選擇定案後再對話只剩純台詞，防止疊加互斥漣漪旗標。
 */
#include "doctest/doctest.h"
#include "game/quest/Flags.h"
#include "game/dialog/DialogOpener.h"
#include "game/dialog/DialogState.h"
#include "game/dialog/DialogSource.h"
#include "game/entities/Player.h"
#include "game/state/SemesterState.h"
#include "engine/math/Vec2.h"

#ifndef TEST_CONTENT_DIR
#error "TEST_CONTENT_DIR must be defined by the build system"
#endif

using nccu::SemesterState;
using nccu::engine::math::Vec2;

// 西裝學長是影響漣漪的關鍵選擇開場者。一旦某個選擇被定案
//（GameController 設下 Flag_SuitSeniorChoiceMade），再對話就只能用純台詞
// 重述、不再出現分支選單，玩家因此無法疊加互斥的漣漪旗標
//（例如先選 (d) Flag_HelpedSenior 再選 (c) Flag_ScoldedSenior）。
// shop_auntie / victim 仍可重複進入選單。

namespace {

// 把 DialogState 推進到終點：若停在選擇選單回傳 true；若以純台詞收尾回傳 false。
bool DriveToChoiceOrClose(nccu::DialogState& dlg) {
    for (int guard = 0; guard < 64 && dlg.Active(); ++guard) {
        if (dlg.AtChoice()) return true;
        dlg.Advance();
    }
    return dlg.AtChoice();
}

}  // namespace

// 首次與西裝學長對話應出現 (b)/(c)/(d) 分支選單。
TEST_CASE("C.3(b): first 西裝學長 talk presents the branch menu") {
    nccu::dialog::SetContentDir(TEST_CONTENT_DIR);
    nccu::DialogState dlg;
    Player p{Vec2{0, 0}};

    // 硬性關卡：選單只有在見過苦主並承諾還傘（Flag_PromisedVictim）後才開啟；
    // 否則學長只會以陌生人口吻純台詞帶過。先設下承諾旗標讓真正的選單出現。
    p.SetFlag(nccu::kFlagPromisedVictim);

    nccu::OpenNpcDialog(dlg, p, "suit_senior",
                        SemesterState::Chapter1_AddDrop);
    REQUIRE(dlg.Active());
    CHECK(dlg.NpcId() == "suit_senior");
    CHECK(DriveToChoiceOrClose(dlg));            // 抵達選單
    CHECK_FALSE(dlg.Choices().empty());          // (b)/(c)/(d) 都在
}

// 選擇定案旗標設下後，再對話西裝學長只剩純台詞重述，且不重複套用 karma。
TEST_CASE("C.3(b): after the choice flag, 西裝學長 recaps line-only") {
    nccu::dialog::SetContentDir(TEST_CONTENT_DIR);
    nccu::DialogState dlg;
    Player p{Vec2{0, 0}};
    const int karma0 = p.GetKarma();

    // suit_senior 的選擇被確認時，GameController 會設下此旗標。
    p.SetFlag(nccu::kFlagSuitSeniorChoiceMade);

    nccu::OpenNpcDialog(dlg, p, "suit_senior",
                        SemesterState::Chapter1_AddDrop);
    REQUIRE(dlg.Active());                        // 開場台詞仍會顯示
    CHECK(dlg.NpcId() == "suit_senior");
    CHECK_FALSE(DriveToChoiceOrClose(dlg));       // 永遠不會進入選單
    CHECK(dlg.Choices().empty());                 // 純台詞重述
    CHECK_FALSE(dlg.Active());                    // 播完最後一行後關閉
    CHECK(p.GetKarma() == karma0);                // 不重複套用 karma
}

// 守門旗標只作用於西裝學長：苦主等其他選擇開場者仍可正常出現分支選單。
TEST_CASE("C.3(b): the guard is scoped to 西裝學長 — 苦主 still branches") {
    nccu::dialog::SetContentDir(TEST_CONTENT_DIR);
    nccu::DialogState dlg;
    Player p{Vec2{0, 0}};

    // suit_senior 的守門旗標不得外溢到其他選擇開場者。
    p.SetFlag(nccu::kFlagSuitSeniorChoiceMade);

    nccu::OpenNpcDialog(dlg, p, "victim",
                        SemesterState::Chapter1_AddDrop);
    REQUIRE(dlg.Active());
    CHECK(DriveToChoiceOrClose(dlg));             // 苦主仍會出現 A/B 選項
    CHECK_FALSE(dlg.Choices().empty());
}
