/**
 * @file test_quest_indicator.cpp
 * @brief 驗證任務「!」的判定層 QuestIndicatorVisible：依各章規則點亮對應 NPC（即使其名冊 isQuestGiver=false）。
 */
#include "game/quest/Flags.h"
//
// QuestIndicatorVisible 是 View 用來決定某 NPC 是否畫出任務「!」的單一真實來源。
// 它把名冊的 isQuestGiver 旗標與各章規則綜合起來：
//   • Ch4：閘門驅動的名冊中每個 NPC 都 isQuestGiver=false，因此若不特例處理
//     Ch4 會完全不畫「!」、結局也沒有視覺提示。Ch4IndicatorVisible 只點亮助教，
//     直到結算選擇定案（Flag_TaFinaleChoiceMade）為止。
//   • Ch3：A 從進入章節起（跑圈前）就是鏈頭，因此第一步永遠找得到。
//   • Ch3：5 個原型在 Ch3 名冊中 isQuestGiver=false，因此不會與 A→B→C 鏈一起亮——
//     此處以 isQuestGiver=false 餵進判定式來確認。

#include "doctest/doctest.h"
#include "game/quest/QuestIndicator.h"
#include "game/quest/Chapter1Quest.h"
#include "game/quest/Chapter2Quest.h"
#include "game/quest/Chapter4Quest.h"
#include "game/quest/Chapter3Quest.h"
#include "game/entities/Player.h"
#include "engine/math/Vec2.h"

using nccu::SemesterState;

namespace {
Player MakePlayer() { return Player{nccu::engine::math::Vec2{0.0f, 0.0f}}; }
constexpr auto kCh3 = SemesterState::Chapter3_SportsDay;
constexpr auto kCh4 = SemesterState::Chapter4_Finals;
constexpr auto kCh1 = SemesterState::Chapter1_AddDrop;
constexpr auto kCh2 = SemesterState::Chapter2_Midterms;
}  // namespace

// Ch4 結局只點亮助教，直到結算選擇定案（Flag_TaFinaleChoiceMade）後連助教也熄滅。
TEST_CASE("Ch4IndicatorVisible：助教是結局的「!」，直到結算選擇定案") {
    Player p = MakePlayer();
    // 助教在進場時點亮——它是推進 Ending A/B/C 的那個 NPC。
    CHECK(nccu::Ch4IndicatorVisible("ta", p));
    // 其餘 Ch4 原型維持熄滅（結局由閘門驅動）。
    CHECK_FALSE(nccu::Ch4IndicatorVisible("victim", p));
    CHECK_FALSE(nccu::Ch4IndicatorVisible("suit_senior", p));
    CHECK_FALSE(nccu::Ch4IndicatorVisible("bookworm", p));
    CHECK_FALSE(nccu::Ch4IndicatorVisible("shop_auntie", p));
    // 結算選擇一旦提交，連助教也熄滅（已解決）。
    p.SetFlag(nccu::kFlagTaFinaleChoiceMade);
    CHECK_FALSE(nccu::Ch4IndicatorVisible("ta", p));
}

// 在 Ch4，助教即使名冊 isQuestGiver=false 也會亮（判定式以 npcId 為鍵，而非該旗標）。
TEST_CASE("QuestIndicatorVisible Ch4：助教不論名冊旗標都會點亮") {
    // 核心：Ch4 名冊把助教標為 isQuestGiver=FALSE，但結局的「!」仍必須亮——
    // 判定式以 npcId 為鍵，而非該旗標。
    Player p = MakePlayer();
    CHECK(nccu::QuestIndicatorVisible("ta", /*isQuestGiver=*/false, kCh4, p));
    CHECK_FALSE(
        nccu::QuestIndicatorVisible("victim", /*isQuestGiver=*/false, kCh4, p));
    // 空 id 的物件（Player／物品／路人學生）永不點亮。
    CHECK_FALSE(
        nccu::QuestIndicatorVisible("", /*isQuestGiver=*/false, kCh4, p));
    p.SetFlag(nccu::kFlagTaFinaleChoiceMade);
    CHECK_FALSE(nccu::QuestIndicatorVisible("ta", false, kCh4, p));
}

// 在 Ch3，鏈頭 A 在跑圈前就點亮，而 isQuestGiver=false 的原型維持熄滅，不會冒出多餘的「!」。
TEST_CASE("QuestIndicatorVisible Ch3：鏈頭跑圈前點亮，原型維持熄滅") {
    Player p = MakePlayer();
    // A（任務給予者）從進場、跑圈前就點亮。
    CHECK(nccu::QuestIndicatorVisible("vendor_sausage_a",
                                      /*isQuestGiver=*/true, kCh3, p));
    // 重新放置的原型在 Ch3 名冊中 isQuestGiver=false，因此即使與進行中的鏈
    // 同處一章也維持熄滅——不會冒出多餘的「!」（由 Ch3 分支中的 && 條件保證）。
    CHECK_FALSE(nccu::QuestIndicatorVisible("ta",
                                            /*isQuestGiver=*/false, kCh3, p));
    CHECK_FALSE(nccu::QuestIndicatorVisible("suit_senior",
                                            /*isQuestGiver=*/false, kCh3, p));
    // B/C 兩環在輪到它們之前維持熄滅。
    CHECK_FALSE(nccu::QuestIndicatorVisible("loudspeaker_b", true, kCh3, p));
    CHECK_FALSE(nccu::QuestIndicatorVisible("senior_c", true, kCh3, p));
}

// Ch1 主線是三步驟的「!」序列 苦主 → 西裝學長 → 苦主，一次只亮一個；學長雖在名冊中 isQuestGiver=false，仍必須在中間步驟點亮。
TEST_CASE("Ch1 的「!」序列 苦主 -> 西裝學長 -> 苦主") {
    Player p = MakePlayer();

    // --- 步驟 1：承諾前只有苦主亮（給出引子）。---
    CHECK(nccu::Ch1IndicatorVisible("victim", /*isQuestGiver=*/true, p));
    CHECK_FALSE(nccu::Ch1IndicatorVisible("suit_senior", /*isQuestGiver=*/false, p));
    CHECK(nccu::QuestIndicatorVisible("victim", /*isQuestGiver=*/true, kCh1, p));
    // 西裝學長即使名冊旗標為 FALSE，也會透過 View 包裝層點亮
    //（以 npcId 為鍵的路徑，與 Ch2 學霸／Ch4 助教相同）。
    CHECK_FALSE(
        nccu::QuestIndicatorVisible("suit_senior", /*isQuestGiver=*/false, kCh1, p));

    // --- 步驟 2：已承諾 -> 「!」移到西裝學長（對峙）。---
    p.SetFlag(nccu::kFlagPromisedVictim);
    CHECK_FALSE(nccu::Ch1IndicatorVisible("victim", /*isQuestGiver=*/true, p));
    CHECK(nccu::Ch1IndicatorVisible("suit_senior", /*isQuestGiver=*/false, p));
    CHECK_FALSE(
        nccu::QuestIndicatorVisible("victim", /*isQuestGiver=*/true, kCh1, p));
    CHECK(  // 關鍵斷言：學長在步驟 2 點亮
        nccu::QuestIndicatorVisible("suit_senior", /*isQuestGiver=*/false, kCh1, p));

    // --- 步驟 3：學長選擇已做 -> 「!」回到苦主（還傘）。---
    p.SetFlag(nccu::kFlagSuitSeniorChoiceMade);
    CHECK(nccu::Ch1IndicatorVisible("victim", /*isQuestGiver=*/true, p));
    CHECK_FALSE(nccu::Ch1IndicatorVisible("suit_senior", /*isQuestGiver=*/false, p));
    CHECK(nccu::QuestIndicatorVisible("victim", /*isQuestGiver=*/true, kCh1, p));
    CHECK_FALSE(
        nccu::QuestIndicatorVisible("suit_senior", /*isQuestGiver=*/false, kCh1, p));

    // --- 完成：授予（Flag_HasTrueUmbrella）使所有主線「!」熄滅。---
    p.SetFlag(nccu::kFlagHasTrueUmbrella);
    CHECK_FALSE(nccu::Ch1IndicatorVisible("victim", /*isQuestGiver=*/true, p));
    CHECK_FALSE(nccu::Ch1IndicatorVisible("suit_senior", /*isQuestGiver=*/false, p));
    CHECK_FALSE(
        nccu::QuestIndicatorVisible("victim", /*isQuestGiver=*/true, kCh1, p));

    // 助教／阿姨／學霸的支線是 isQuestGiver=false，因此在任何步驟都不會是主線「!」。
    Player q = MakePlayer();
    CHECK_FALSE(nccu::QuestIndicatorVisible("ta", /*isQuestGiver=*/false, kCh1, q));
    CHECK_FALSE(nccu::QuestIndicatorVisible("shop_auntie", /*isQuestGiver=*/false, kCh1, q));
    // 未來的 Ch1 任務給予者（isQuestGiver=true、非主線）仍會透過 fall-through 尊重其名冊旗標。
    CHECK(nccu::Ch1IndicatorVisible("some_future_giver", /*isQuestGiver=*/true, q));
}

// Ch2 主線「!」依主線順序在圖書館管理員與學霸之間移動；學霸雖在名冊中 isQuestGiver=false 仍會亮。
TEST_CASE("QuestIndicatorVisible Ch2 的「!」序列 管理員 -> 學霸") {
    Player p = MakePlayer();
    // 進場：管理員（鏈頭）亮，學霸暗（引導玩家先找管理員）。
    CHECK(nccu::QuestIndicatorVisible("librarian", /*isQuestGiver=*/true, kCh2, p));
    CHECK_FALSE(
        nccu::QuestIndicatorVisible("bookworm", /*isQuestGiver=*/false, kCh2, p));

    // 叫醒學霸：鏈頭熄滅、「!」移到學霸——即使他的名冊旗標為 FALSE。
    p.SetFlag(nccu::kFlagBookworm);
    CHECK_FALSE(
        nccu::QuestIndicatorVisible("librarian", /*isQuestGiver=*/true, kCh2, p));
    CHECK(nccu::QuestIndicatorVisible("bookworm", /*isQuestGiver=*/false, kCh2, p));

    // 學霸在撿筆記與換回的過程中持續亮著……
    p.SetFlag(nccu::kFlagFoundNote1);
    p.SetFlag(nccu::kFlagFoundNote2);
    p.SetFlag(nccu::kFlagFoundNote3);
    CHECK(nccu::QuestIndicatorVisible("bookworm", /*isQuestGiver=*/false, kCh2, p));

    // ……直到換回完成 -> 熄滅。
    p.SetFlag(nccu::kFlagBookwormRecovered);
    CHECK_FALSE(
        nccu::QuestIndicatorVisible("bookworm", /*isQuestGiver=*/false, kCh2, p));
    CHECK_FALSE(
        nccu::QuestIndicatorVisible("librarian", /*isQuestGiver=*/true, kCh2, p));
}

// 非主線的 Ch2 NPC 維持其名冊旗標（閘門只對兩個主線 NPC 特例處理，其餘直接沿用 isQuestGiver）。
TEST_CASE("Ch2 非主線 NPC 維持其 isQuestGiver 旗標") {
    Player p = MakePlayer();
    CHECK(nccu::Ch2IndicatorVisible("suit_senior", /*isQuestGiver=*/true, p));
    CHECK_FALSE(nccu::Ch2IndicatorVisible("suit_senior", /*isQuestGiver=*/false, p));
    // 空 id 的物件（玩家／物品／路人）永不點亮。
    CHECK_FALSE(
        nccu::QuestIndicatorVisible("", /*isQuestGiver=*/false, kCh2, p));
}
