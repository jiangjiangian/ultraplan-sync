#include "doctest/doctest.h"
#include "game/quest/Flags.h"
#include "game/dialog/DialogOpener.h"
#include "game/dialog/DialogState.h"
#include "game/controller/DialogChoiceApply.h"
#include "game/entities/Player.h"

using nccu::DialogState;
using nccu::SemesterState;

/**
 * @file test_dialog_opener.cpp
 * @brief 驗證 DialogOpener 系列函式：依 npcId／章節載入開場白與選項、未知 id 不啟用、
 *        選擇分支後續台詞、once-only 獎勵不重複給、以及 ResolveOpenerSubState 依旗標
 *        決定開場子狀態與 Player overload 套用 karma／旗標的行為。
 */

// OpenNpcDialogSub 載入苦主 Ch1 (a) 開場白（純台詞）。
TEST_CASE("OpenNpcDialogSub 載入苦主 Ch1 (a) 開場白（純台詞）") {
    DialogState d;
    nccu::OpenNpcDialogSub(d, "victim", SemesterState::Chapter1_AddDrop, 0);
    CHECK(d.Active());
    CHECK_FALSE(d.AtChoice());
    CHECK(d.CurrentLine() == "……我的傘也不見了。");
    for (int i = 0; i < 4; ++i) CHECK(d.Advance() == nullptr);
    CHECK(d.CurrentLine() == "你也是被偷的嗎？可以幫我把傘找回來嗎……");
    CHECK(d.Advance() == nullptr);
    CHECK_FALSE(d.Active());
}

// OpenNpcDialogSub 遇到未知 npcId 時對話維持未啟用。
TEST_CASE("OpenNpcDialogSub 遇到未知 npcId 時對話維持未啟用") {
    DialogState d;
    nccu::OpenNpcDialogSub(d, "nobody", SemesterState::Chapter1_AddDrop, 0);
    CHECK_FALSE(d.Active());
}

// OpenNpcDialogSub 載入西裝學長 Ch1 (a) 開場白的第一行。
TEST_CASE("OpenNpcDialogSub 載入 suit_senior Ch1 (a) 開場白第一行") {
    DialogState d;
    nccu::OpenNpcDialogSub(d, "suit_senior",
                           SemesterState::Chapter1_AddDrop, 0);
    CHECK(d.Active());
    CHECK(d.CurrentLine() == "欸，加退選也沒搶到嗎？");
}

// 3 參數版 OpenNpcDialog：苦主 Ch1 顯示開場白與 2 個選項，選 (b) 後續播放。
TEST_CASE("3 參數版 OpenNpcDialog 苦主 Ch1：開場白 + 2 選項，選 (b) 後續播放") {
    DialogState d;
    nccu::OpenNpcDialog(d, "victim", SemesterState::Chapter1_AddDrop);
    CHECK(d.Active());
    CHECK(d.CurrentLine() == "……我的傘也不見了。");      // (a) 開場白第 0 行
    // 走過 5 行開場白進入選項模式。
    for (int i = 0; i < 5; ++i) d.Advance();
    CHECK(d.AtChoice());
    REQUIRE(d.Choices().size() == 2);
    // 表格順序：subState 1 (b) 在前、subState 2 (c) 在後。
    // 第一人稱視角：(c) 忽略選項已不帶「玩家」主詞，改寫為「別過頭，當作沒看見」。
    CHECK(d.Choices()[0].label == "我去幫你追");
    CHECK(d.Choices()[1].label == "別過頭，當作沒看見");
    // 選擇幫忙分支（索引 0）-> 播放其 (b) 後果。
    const nccu::DialogChoice* c = d.Advance();
    REQUIRE(c != nullptr);
    CHECK(c->label == "我去幫你追");
    CHECK(c->setsFlag == nccu::kFlagPromisedVictim);
    CHECK(c->flagValue == true);
    CHECK_FALSE(d.AtChoice());
    CHECK(d.Active());
    CHECK(d.CurrentLine() == "真的？謝謝你……");          // (b) 第 0 行
    while (d.Active()) d.Advance();                       // 走完 -> 關閉
    CHECK_FALSE(d.Active());
}

// 3 參數版 OpenNpcDialog：福利社阿姨 Ch1 顯示開場白與買傘等選項。
TEST_CASE("3 參數版 OpenNpcDialog 福利社阿姨 Ch1：開場白 + 買傘選項") {
    DialogState d;
    nccu::OpenNpcDialog(d, "shop_auntie", SemesterState::Chapter1_AddDrop);
    CHECK(d.Active());
    CHECK_FALSE(d.AtChoice());
    // 走過 4 行 (a) 開場白進入選項模式。
    for (int i = 0; i < 4; ++i) d.Advance();
    CHECK(d.AtChoice());
    // 第三個 Ch1 選項——請阿姨喝咖啡——是 Ch1→Ch4 福利社阿姨支線的種子。
    REQUIRE(d.Choices().size() == 3);
    // 表格順序：subState 1, 2, 3（a<b<c<d，開場白是 subState 0）。
    CHECK(d.Choices()[0].label == "詢問雨傘");
    CHECK(d.Choices()[1].label == "購買醜綠傘");
    CHECK(d.Choices()[2].label == "請阿姨喝一杯熱咖啡");
    // Ch1 阿姨 (c) 購買分支現在純粹是敘事種子——它不設定任何 flag。
    // Ending C 真正的觸發點是 Ch4 集英樓的 Vendor（依 Flag_BoughtUglyUmbrella）。
    d.MoveChoice(1);
    const nccu::DialogChoice* c = d.Advance();
    REQUIRE(c != nullptr);
    CHECK(c->label == "購買醜綠傘");
    CHECK(c->setsFlag == "");     // 不設定 flag
    while (d.Active()) d.Advance();    // 走完 -> 關閉
    CHECK_FALSE(d.Active());
}

// Ch1 福利社阿姨 (d) 請咖啡 選項是 Flag_BoughtCoffeeForAuntie_Ch1 的種子
//（Ch1→Ch4 的支線）。選擇它必須設定該旗標並加 5 karma；少了 chapter1.md 的
// (d) 子狀態與選項開場路徑，此選項就不存在，對其存在的 REQUIRE 會失敗。
TEST_CASE("Ch1 福利社阿姨請咖啡選項種下 BoughtCoffeeForAuntie") {
    DialogState d;
    Player p{nccu::engine::math::Vec2{0, 0}};
    const int k0 = p.GetKarma();
    nccu::OpenNpcDialog(d, p, "shop_auntie", SemesterState::Chapter1_AddDrop);
    for (int i = 0; i < 4; ++i) d.Advance();          // (a) opener lines
    REQUIRE(d.AtChoice());
    REQUIRE(d.Choices().size() == 3);
    const nccu::DialogChoice& coffee = d.Choices()[2];
    CHECK(coffee.label == "請阿姨喝一杯熱咖啡");
    CHECK(coffee.setsFlag == nccu::kFlagBoughtCoffeeForAuntie);
    CHECK(coffee.flagValue == true);
    CHECK(coffee.karmaDelta == 5);
    // 透過 GameController 的套用器端到端確認此選項。
    d.MoveChoice(2);
    const nccu::DialogChoice* c = d.Advance();
    REQUIRE(c != nullptr);
    nccu::ApplyDialogChoice(p, *c);
    CHECK(p.HasFlag(nccu::kFlagBoughtCoffeeForAuntie));
    CHECK(p.GetKarma() == k0 + 5);
}

// Ch1 福利社阿姨 請咖啡 選項是「一次性」的。一旦
// Flag_BoughtCoffeeForAuntie_Ch1 被設定（首次選擇），再次對話重選同一選項
// 不得重複加 5 karma（不能刷分）。守門邏輯在 ApplyDialogChoice（玩家已滿足
// 的自設旗標選項是惰性的）。惰性的 (b)/(c) 風味選項（karma +0／無旗標）仍可
// 重選——也驗證它們同樣不會改動 karma。
TEST_CASE("福利社阿姨請咖啡是一次性的（不能重複刷 karma）") {
    Player p{nccu::engine::math::Vec2{0, 0}};
    const int k0 = p.GetKarma();

    // 第一次：選咖啡 -> +5、設定旗標。
    {
        DialogState d;
        nccu::OpenNpcDialog(d, p, "shop_auntie",
                            SemesterState::Chapter1_AddDrop);
        for (int i = 0; i < 4; ++i) d.Advance();    // 開場白
        REQUIRE(d.AtChoice());
        d.MoveChoice(2);                            // 請咖啡
        const nccu::DialogChoice* c = d.Advance();
        REQUIRE(c != nullptr);
        nccu::ApplyDialogChoice(p, *c);
    }
    CHECK(p.HasFlag(nccu::kFlagBoughtCoffeeForAuntie));
    CHECK(p.GetKarma() == k0 + 5);

    // 第二次：再選一次咖啡 -> karma 不得改動（已完成）。
    {
        DialogState d;
        nccu::OpenNpcDialog(d, p, "shop_auntie",
                            SemesterState::Chapter1_AddDrop);
        for (int i = 0; i < 4; ++i) d.Advance();
        REQUIRE(d.AtChoice());
        d.MoveChoice(2);
        const nccu::DialogChoice* c = d.Advance();
        REQUIRE(c != nullptr);
        nccu::ApplyDialogChoice(p, *c);
    }
    CHECK(p.GetKarma() == k0 + 5);                  // 仍是 +5，不是 +10

    // 惰性的 (b) 詢問雨傘 選項（karma +0、無旗標）可重選且不改動任何值——
    // 確認守門只攔截獎勵型選項。
    {
        DialogState d;
        nccu::OpenNpcDialog(d, p, "shop_auntie",
                            SemesterState::Chapter1_AddDrop);
        for (int i = 0; i < 4; ++i) d.Advance();
        REQUIRE(d.AtChoice());
        d.MoveChoice(0);                            // 詢問雨傘
        const nccu::DialogChoice* c = d.Advance();
        REQUIRE(c != nullptr);
        nccu::ApplyDialogChoice(p, *c);
    }
    CHECK(p.GetKarma() == k0 + 5);                  // 不變
}

// ResolveOpenerSubState：助教的開場子狀態由跑腿任務旗標決定。
TEST_CASE("ResolveOpenerSubState：助教的開場子狀態由跑腿任務旗標決定") {
    const auto Ch1 = SemesterState::Chapter1_AddDrop;
    Player p{nccu::engine::math::Vec2{0, 0}};
    CHECK(nccu::ResolveOpenerSubState("ta", Ch1, p) == 0);   // 初始
    p.SetFlag(nccu::kFlagFoundForm);
    CHECK(nccu::ResolveOpenerSubState("ta", Ch1, p) == 1);   // 獎勵
    p.SetFlag(nccu::kFlagHelpedTACh1);
    CHECK(nccu::ResolveOpenerSubState("ta", Ch1, p) == 1);   // 維持 1
}

// ResolveOpenerSubState：苦主的回顧開場由承諾／給傘旗標決定。
TEST_CASE("ResolveOpenerSubState：苦主的回顧開場由承諾／給傘旗標決定") {
    const auto Ch1 = SemesterState::Chapter1_AddDrop;
    Player p{nccu::engine::math::Vec2{0, 0}};
    CHECK(nccu::ResolveOpenerSubState("victim", Ch1, p) == 0);  // (a) 初始
    p.SetFlag(nccu::kFlagPromisedVictim);
    CHECK(nccu::ResolveOpenerSubState("victim", Ch1, p) == 1);  // (b) 已承諾
    // 善有善報：一旦真傘已歸還（返還發生過），苦主就導向 (d) 重逢致謝 回顧，
    // 其優先序高於承諾回顧。
    p.SetFlag(nccu::kFlagHasTrueUmbrella);
    CHECK(nccu::ResolveOpenerSubState("victim", Ch1, p) == 3);  // (d) 重逢
}

// ResolveOpenerSubState：非任務 NPC 永遠是 subState 0。
TEST_CASE("ResolveOpenerSubState：非任務 NPC 永遠是 subState 0") {
    const auto Ch1 = SemesterState::Chapter1_AddDrop;
    Player p{nccu::engine::math::Vec2{0, 0}};
    CHECK(nccu::ResolveOpenerSubState("bookworm", Ch1, p) == 0);
    p.SetFlag(nccu::kFlagFoundForm);
    p.SetFlag(nccu::kFlagPromisedVictim);
    CHECK(nccu::ResolveOpenerSubState("bookworm", Ch1, p) == 0);
}

// Player overload：助教獎勵的 karma／旗標只套用一次。
TEST_CASE("Player 重載：助教獎勵的 karma／旗標只套用一次") {
    const auto Ch1 = SemesterState::Chapter1_AddDrop;
    Player p{nccu::engine::math::Vec2{0, 0}};
    p.SetFlag(nccu::kFlagFoundForm);
    const int k0 = p.GetKarma();
    DialogState d;
    nccu::OpenNpcDialog(d, p, "ta", Ch1);
    CHECK(d.Active());
    CHECK_FALSE(d.AtChoice());
    CHECK(d.CurrentLine() == "謝謝你……那份表格要是不見我真的完了。");  // 助教 sub-1 第 0 行
    CHECK(p.HasFlag(nccu::kFlagHelpedTACh1));
    CHECK(p.GetKarma() == k0 + 5);
    // 以同一個玩家重開 -> 套用一次的守門會略過（不重複）。
    DialogState d2;
    nccu::OpenNpcDialog(d2, p, "ta", Ch1);
    CHECK(d2.Active());
    CHECK(d2.CurrentLine() == "謝謝你……那份表格要是不見我真的完了。");
    CHECK(p.GetKarma() == k0 + 5);
}

// Player overload：助教在無旗標時是純台詞的開場。
TEST_CASE("Player 重載：助教在無旗標時是純台詞的開場") {
    const auto Ch1 = SemesterState::Chapter1_AddDrop;
    Player p2{nccu::engine::math::Vec2{0, 0}};
    const int k0 = p2.GetKarma();
    DialogState d2;
    nccu::OpenNpcDialog(d2, p2, "ta", Ch1);
    CHECK(d2.Active());
    CHECK_FALSE(d2.AtChoice());
    CHECK(d2.CurrentLine() == "同學，加退選截止了，現在不受理。");  // 助教 sub-0 第 0 行
    CHECK(p2.GetKarma() == k0);
    CHECK_FALSE(p2.HasFlag(nccu::kFlagHelpedTACh1));
}

// Player overload：苦主在無旗標時仍會開啟 1b-2 的選項。
TEST_CASE("Player 重載：苦主在無旗標時仍會開啟 1b-2 的選項") {
    const auto Ch1 = SemesterState::Chapter1_AddDrop;
    Player p{nccu::engine::math::Vec2{0, 0}};
    DialogState d;
    nccu::OpenNpcDialog(d, p, "victim", Ch1);
    CHECK(d.Active());
    CHECK(d.CurrentLine() == "……我的傘也不見了。");      // (a) 開場白第 0 行
    for (int i = 0; i < 5; ++i) d.Advance();
    CHECK(d.AtChoice());
    CHECK(d.Choices().size() == 2);
}

// Player overload：苦主在已有承諾旗標時是純台詞的回顧。
TEST_CASE("Player 重載：苦主在已有承諾旗標時是純台詞的回顧") {
    const auto Ch1 = SemesterState::Chapter1_AddDrop;
    Player p{nccu::engine::math::Vec2{0, 0}};
    p.SetFlag(nccu::kFlagPromisedVictim);
    const int k0 = p.GetKarma();
    DialogState d;
    nccu::OpenNpcDialog(d, p, "victim", Ch1);
    CHECK(d.Active());
    CHECK_FALSE(d.AtChoice());
    CHECK(d.CurrentLine() == "真的？謝謝你……");        // 苦主 sub-1 第 0 行
    CHECK(p.GetKarma() == k0);   // +5 是選項的職責，旗標已預設 -> 略過
}

// ---------------------------------------------------------------------------
// Ch1 西裝學長 (b) 是 Flag_ScoldedSenior 支線的關鍵旗標。此旗標驅動跨章節的
// 「保持距離」支線——DialogOpener Ch2 西裝學長 → (c) 尷尬讓開、Ch3 距離、
// Ch4 不出場（World 生成過濾）——因此必須能從 Ch1 的選項抵達。
//
// (b) 選項已不再是帶敵意的斥責（-5），而改為理性而堅定的指正——
//「理性指出他品行不該，要回雨傘」、karma +3——且後續反應從怨懟軟化為輕微尷尬
//（Chapter2Quest 的 Ch2 ScoldedSenior 連鎖現在對 karma 中性）。關鍵旗標保留，
// 沒有任何分支變死；只有定調與 karma 正負號改變。第一人稱視角：選項標籤不帶
//「玩家」主詞。
TEST_CASE("Ch1 suit_senior 選項 0 (b) 種下 Flag_ScoldedSenior（+3）") {
    const auto Ch1 = SemesterState::Chapter1_AddDrop;
    DialogState d;
    Player p{nccu::engine::math::Vec2{0, 0}};
    const int k0 = p.GetKarma();

    // 硬性條件：西裝學長只有在玩家已遇見苦主並承諾追傘（Flag_PromisedVictim）
    // 之後，才會呈現選項選單。否則他只會把陌生人打發掉（純台詞的轉向，無選單）。
    // 設定承諾旗標，讓本測試演練真正的選項選單。
    p.SetFlag(nccu::kFlagPromisedVictim);

    nccu::OpenNpcDialog(d, p, "suit_senior", Ch1);
    CHECK(d.Active());
    CHECK(d.CurrentLine() == "欸，加退選也沒搶到嗎？");
    // 走過 5 行開場白進入選項模式。
    for (int i = 0; i < 5; ++i) d.Advance();
    REQUIRE(d.AtChoice());
    REQUIRE(d.Choices().size() == 3);

    // 選項索引 0 是 (b) 指正分支。現在是理性而堅定的指正（+3），而非帶敵意的
    // 斥責（-5）。它仍設定 Flag_ScoldedSenior——保留為「保持距離」支線的關鍵
    // 旗標——所以 Ch2/3/4 的路由不變；只有定調（尷尬而非怨懟）與 karma 正負號
    // 改變。第一人稱標籤，不帶「玩家」主詞。
    //（DialogOpener.cpp 把 subState ≥ 1 升序打包，所以 b→0、c→1、d→2——
    // ending_a.txt 中 suit_senior 的 choose 2 仍解析到 (d) HelpedSenior。）
    const nccu::DialogChoice& scolded = d.Choices()[0];
    CHECK(scolded.label == "理性指出他品行不該，要回雨傘");
    CHECK(scolded.setsFlag == nccu::kFlagScoldedSenior);
    CHECK(scolded.flagValue == true);
    CHECK(scolded.karmaDelta == 3);

    // 透過 GameController 套用器端到端——實際的確認路徑。
    d.MoveChoice(0);
    const nccu::DialogChoice* c = d.Advance();
    REQUIRE(c != nullptr);
    nccu::ApplyDialogChoice(p, *c);
    CHECK(p.HasFlag(nccu::kFlagScoldedSenior));
    CHECK_FALSE(p.HasFlag(nccu::kFlagHelpedSenior));   // 對照，未設定
    CHECK(p.GetKarma() == k0 + 3);
}

// Ch2 西裝學長在 Flag_ScoldedSenior 時導向 (c) 冷淡分支。
TEST_CASE("Ch2 suit_senior 在 Flag_ScoldedSenior 時導向 (c) 冷淡分支") {
    // 原本是只讀取而無設定者的旗標；Ch1 (b) 指正路徑會設定它，使此分支觸發。
    const auto Ch2 = SemesterState::Chapter2_Midterms;
    Player p{nccu::engine::math::Vec2{0, 0}};
    p.SetFlag(nccu::kFlagScoldedSenior);
    CHECK(nccu::ResolveOpenerSubState("suit_senior", Ch2, p) == 2);   // (c)

    Player q{nccu::engine::math::Vec2{0, 0}};
    // 無旗標 → (a) 開場白，即預設。（對照的合理性檢查。）
    CHECK(nccu::ResolveOpenerSubState("suit_senior", Ch2, q) == 0);
}
