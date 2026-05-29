/**
 * @file test_chapter4_senior_skip.cpp
 * @brief 驗證 Ch4 名冊過濾：Ch1 翻臉的西裝學長不再出現於期末考，除非 Ch2 修補關係讓他回歸。
 */
#include "doctest/doctest.h"
#include "game/quest/Flags.h"
#include "game/quest/ChapterSpawns.h"
#include "engine/core/GameObject.h"
#include "game/entities/NPC.h"
#include "game/entities/Player.h"
#include "game/state/SemesterState.h"
#include "game/world/World.h"

#include <string>
#include <string_view>

using nccu::SemesterState;
using nccu::World;

// 敘事設定：在 Ch1 對西裝學長翻臉（Flag_ScoldedSenior）的玩家，期末考時
// 不會再見到他——除非 Ch2 的回呼（Flag_HelpedSenior）修補了關係，那他就會
// 回來。此過濾發生在 World::SpawnChapterNpcs 的生成階段，讓缺席的 NPC 與
// 「librarian 不在 Ch4」採同一種建模方式：不在 objects_ 中、沒有特殊對話
// 開場路徑，也不會在下一次 Transition 時造成名冊殘留。
//
// 名冊過濾必須同時觀察兩個旗標：只翻臉而未在 Ch2 修補關係 → 隱藏學長；
// 翻臉之後又幫助他 → 學長回歸。

namespace {

bool HasNpcId(const World& w, const char* id) {
    for (const auto& o : w.Objects())
        if (o->NpcId() == std::string_view(id)) return true;
    return false;
}

} // namespace

// Flag_ScoldedSenior 應把西裝學長從 Ch4 名冊隱藏；若另有 Flag_HelpedSenior 則他回歸。
TEST_CASE("M7: Flag_ScoldedSenior hides suit_senior from Ch4") {
    World w("", /*loadSprites=*/false);
    Player* p = w.GetPlayer();
    REQUIRE(p != nullptr);

    SUBCASE("ScoldedSenior alone removes suit_senior from Ch4") {
        p->SetFlag(nccu::kFlagScoldedSenior);
        w.RespawnChapterRoster(SemesterState::Chapter4_Finals);

        // 其餘 4 個原型仍在，只有 suit_senior 消失。
        CHECK(HasNpcId(w, "victim"));
        CHECK(HasNpcId(w, "bookworm"));
        CHECK(HasNpcId(w, "ta"));
        CHECK(HasNpcId(w, "shop_auntie"));
        CHECK_FALSE(HasNpcId(w, "suit_senior"));
    }

    SUBCASE("ScoldedSenior + HelpedSenior re-introduces suit_senior") {
        // 玩家在 Ch2 修補了關係（回呼）。
        p->SetFlag(nccu::kFlagScoldedSenior);
        p->SetFlag(nccu::kFlagHelpedSenior);
        w.RespawnChapterRoster(SemesterState::Chapter4_Finals);

        CHECK(HasNpcId(w, "suit_senior"));
        CHECK(HasNpcId(w, "victim"));
        CHECK(HasNpcId(w, "bookworm"));
        CHECK(HasNpcId(w, "ta"));
        CHECK(HasNpcId(w, "shop_auntie"));
    }

    SUBCASE("Neither flag: default Ch4 roster has every archetype") {
        // 沒翻臉也沒回呼——完整 5 個原型的高張力 Ch4。
        w.RespawnChapterRoster(SemesterState::Chapter4_Finals);

        CHECK(HasNpcId(w, "suit_senior"));
        CHECK(HasNpcId(w, "victim"));
        CHECK(HasNpcId(w, "bookworm"));
        CHECK(HasNpcId(w, "ta"));
        CHECK(HasNpcId(w, "shop_auntie"));
    }

    SUBCASE("HelpedSenior alone is the default — suit_senior stays") {
        // 從未對學長翻臉的玩家應一直能見到他。
        p->SetFlag(nccu::kFlagHelpedSenior);
        w.RespawnChapterRoster(SemesterState::Chapter4_Finals);

        CHECK(HasNpcId(w, "suit_senior"));
    }
}

// 過濾僅限 Ch4：Ch2／Ch3 仍保留 suit_senior，且來回切換章節不會造成名冊殘留。
TEST_CASE("M7: filter is Ch4-only — other chapters keep suit_senior") {
    // Ch2／Ch3 名冊為了漣漪敘事仍包含 suit_senior（isQuestGiver=false）；
    // 即使旗標條件成立，此過濾也不得外溢到那些章節。
    World w("", /*loadSprites=*/false);
    Player* p = w.GetPlayer();
    REQUIRE(p != nullptr);
    p->SetFlag(nccu::kFlagScoldedSenior);   // 只會在 Ch4 跳過他

    w.RespawnChapterRoster(SemesterState::Chapter2_Midterms);
    CHECK(HasNpcId(w, "suit_senior"));

    w.RespawnChapterRoster(SemesterState::Chapter3_SportsDay);
    CHECK(HasNpcId(w, "suit_senior"));

    // 一路走到 Ch4——消失。
    w.RespawnChapterRoster(SemesterState::Chapter4_Finals);
    CHECK_FALSE(HasNpcId(w, "suit_senior"));

    // ……再回到 Ch3——又出現（沒有名冊殘留）。
    w.RespawnChapterRoster(SemesterState::Chapter3_SportsDay);
    CHECK(HasNpcId(w, "suit_senior"));
}
