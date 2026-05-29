/**
 * @file test_ch2_quest.cpp
 * @brief 驗證 Ch2 主線：先見圖書館管理員→喝飲料叫醒學霸→撿筆記→換回，含硬性關卡、背包行清理與清關轉場。
 */
#include "doctest/doctest.h"
#include "game/quest/Flags.h"
#include "game/quest/Chapter2Quest.h"
#include "game/quest/ChapterGate.h"
#include "game/quest/ItemCatalog.h"
#include "game/dialog/DialogOpener.h"
#include "game/dialog/DialogSource.h"
#include "game/dialog/DialogState.h"
#include "engine/events/EventBus.h"
#include "game/entities/Player.h"
#include "game/state/SemesterStateMachine.h"
#include "engine/math/Vec2.h"

#ifndef TEST_CONTENT_DIR
#error "TEST_CONTENT_DIR must be defined by the build system"
#endif

using nccu::SemesterState;

namespace {

Player MakePlayer() { return Player{nccu::engine::math::Vec2{0.0f, 0.0f}}; }

void GiveNotes(Player& p) {
    p.SetFlag(nccu::kFlagFoundNote1);
    p.SetFlag(nccu::kFlagFoundNote2);
    p.SetFlag(nccu::kFlagFoundNote3);
}

}  // namespace

// Ch2 librarian 的 (b) 受叫醒旗標守門：學霸被叫醒後才切到 (b)（而非撿到筆記之後）。
TEST_CASE("ResolveOpenerSubState: Ch2 librarian (b) gated on the wake flag") {
    Player p = MakePlayer();
    CHECK(nccu::ResolveOpenerSubState(
              "librarian", SemesterState::Chapter2_Midterms, p) == 0);
    // librarian 現在會把玩家指向學霸；(b) 在學霸被叫醒後才確認，
    // 而非在筆記到手後（筆記要叫醒後才存在）。
    p.SetFlag(nccu::kFlagBookworm);
    CHECK(nccu::ResolveOpenerSubState(
              "librarian", SemesterState::Chapter2_Midterms, p) == 1);
}

// Ch2 bookworm 開場依進度推進：(a) 沉睡 → (c) 已叫醒 → (d) 已換回；Ch1 路由不受影響。
TEST_CASE("ResolveOpenerSubState: Ch2 bookworm (a)->(c) woken->(d) recovered") {
    Player p = MakePlayer();
    CHECK(nccu::ResolveOpenerSubState(
              "bookworm", SemesterState::Chapter2_Midterms, p) == 0);  // (a)
    p.SetFlag(nccu::kFlagBookworm);
    CHECK(nccu::ResolveOpenerSubState(
              "bookworm", SemesterState::Chapter2_Midterms, p) == 2);  // (c)
    p.SetFlag(nccu::kFlagBookwormRecovered);
    CHECK(nccu::ResolveOpenerSubState(
              "bookworm", SemesterState::Chapter2_Midterms, p) == 3);  // (d)

    // Ch1 路由不受影響（新分支限定在 Ch2）。
    Player q = MakePlayer();
    CHECK(nccu::ResolveOpenerSubState(
              "bookworm", SemesterState::Chapter1_AddDrop, q) == 0);
}

// 叫醒步驟會消耗一瓶飲料；換回步驟需要三張筆記。完整走一遍叫醒→換回流程並驗證冪等性。
TEST_CASE("TryRescueBookworm: wake step consumes drink; exchange needs notes") {
    EventBus::Instance().Clear();
    Player p = MakePlayer();
    // 硬性關卡：學霸在見過圖書館管理員之前無法被叫醒。本案測的是叫醒／換回
    // 流程，故先見過她（她自己的關卡由下方專門的案例涵蓋）。
    p.SetFlag(nccu::kFlagMetLibrarian);

    // 章節不符／對象不符 -> 無操作。
    nccu::TryRescueBookworm(EventBus::Instance(), p, "bookworm", SemesterState::Chapter1_AddDrop);
    CHECK_FALSE(p.HasFlag(nccu::kFlagBookworm));
    nccu::TryRescueBookworm(EventBus::Instance(), p, "ta", SemesterState::Chapter2_Midterms);
    CHECK_FALSE(p.HasFlag(nccu::kFlagBookworm));

    // 階段 1，沉睡且沒有飲料 -> 只提示，不設旗標也不消耗。
    const int k0 = p.GetKarma();
    nccu::TryRescueBookworm(EventBus::Instance(), p, "bookworm", SemesterState::Chapter2_Midterms);
    CHECK_FALSE(p.HasFlag(nccu::kFlagBookworm));
    CHECK(p.GetKarma() == k0);

    // 階段 1，沉睡且持有飲料 -> 消耗並叫醒。尚未換回，且此處筆記無關
    //（叫醒才是筆記任務的起點）。
    p.AddConsumable("EnergyDrink");
    nccu::TryRescueBookworm(EventBus::Instance(), p, "bookworm", SemesterState::Chapter2_Midterms);
    CHECK(p.HasFlag(nccu::kFlagBookworm));
    CHECK(p.ConsumableCount("EnergyDrink") == 0);   // 在叫醒步驟用掉
    CHECK_FALSE(p.HasFlag(nccu::kFlagBookwormRecovered));
    CHECK(p.GetKarma() == k0);                       // +5 是在換回步驟

    // 階段 2，已叫醒但筆記不齊 -> 只提醒、不換回，且關鍵在於不需要也不消耗額外飲料。
    nccu::TryRescueBookworm(EventBus::Instance(), p, "bookworm", SemesterState::Chapter2_Midterms);
    CHECK_FALSE(p.HasFlag(nccu::kFlagBookwormRecovered));

    // 階段 2，已叫醒且三張筆記齊全 -> 換回：+5、已換回、不消耗飲料（本來就沒持有）。
    // 重複對話具冪等性。
    GiveNotes(p);
    nccu::TryRescueBookworm(EventBus::Instance(), p, "bookworm", SemesterState::Chapter2_Midterms);
    CHECK(p.HasFlag(nccu::kFlagBookwormRecovered));
    CHECK(p.GetKarma() == k0 + 5);
    CHECK(p.ConsumableCount("EnergyDrink") == 0);
    nccu::TryRescueBookworm(EventBus::Instance(), p, "bookworm", SemesterState::Chapter2_Midterms);
    CHECK(p.GetKarma() == k0 + 5);   // 不重複給獎
    EventBus::Instance().Clear();
}

// 叫醒學霸時提神飲料是交出去的：背包數量恰好減一（不是檢查旗標、也不是全清）。持有兩瓶可證明只扣一瓶。
TEST_CASE("B2.2: waking the 學霸 spends exactly one 提神飲料 from the bag") {
    EventBus::Instance().Clear();
    Player p = MakePlayer();
    p.SetFlag(nccu::kFlagMetLibrarian);                            // 已見鏈頭
    p.AddConsumable("EnergyDrink").AddConsumable("EnergyDrink");   // 持有 2 瓶
    CHECK(p.ConsumableCount("EnergyDrink") == 2);

    nccu::TryRescueBookworm(EventBus::Instance(), p, "bookworm", SemesterState::Chapter2_Midterms);
    CHECK(p.HasFlag(nccu::kFlagBookworm));
    CHECK(p.ConsumableCount("EnergyDrink") == 1);   // 2 -> 1，恰好交出一瓶

    // 第二次對話（已叫醒、筆記不齊）不得再扣掉另一瓶。
    nccu::TryRescueBookworm(EventBus::Instance(), p, "bookworm", SemesterState::Chapter2_Midterms);
    CHECK(p.ConsumableCount("EnergyDrink") == 1);   // 不額外扣減
    EventBus::Instance().Clear();
}

// 學霸被叫醒後（她的 (b) 狀態），圖書館管理員會借出管理員的傘：玩家持有借來的傘
//（背包出現一行＋自動擋雨），但那不是真傘，因此 Ending A 的 Flag_HasTrueUmbrella 仍未設。重複對話不疊加。
TEST_CASE("B2.3: 圖書館管理員 lends 管理員的傘 (held + shelter, NOT the true umbrella)") {
    EventBus::Instance().Clear();
    Player p = MakePlayer();

    // 章節不符／對象不符 -> 不授予。
    nccu::TryLendLibrarianUmbrella(p, "librarian",
                                   SemesterState::Chapter1_AddDrop);
    CHECK(p.HeldUmbrellaKind() == HeldUmbrella::None);
    nccu::TryLendLibrarianUmbrella(p, "ta", SemesterState::Chapter2_Midterms);
    CHECK(p.HeldUmbrellaKind() == HeldUmbrella::None);

    // 學霸被叫醒前（她的 (a) 線索狀態）-> 還不會借傘。
    nccu::TryLendLibrarianUmbrella(p, "librarian",
                                   SemesterState::Chapter2_Midterms);
    CHECK(p.HeldUmbrellaKind() == HeldUmbrella::None);
    CHECK_FALSE(p.HasUmbrella());

    // 已叫醒 -> 她的 (b) 交付：玩家現在持有管理員的傘。
    p.SetFlag(nccu::kFlagBookworm);
    nccu::TryLendLibrarianUmbrella(p, "librarian",
                                   SemesterState::Chapter2_Midterms);
    CHECK(p.HeldUmbrellaKind() == HeldUmbrella::Loaner);
    CHECK(p.HasUmbrella());                              // 自動擋雨已啟動
    CHECK(p.HasFlag(nccu::kFlagLibrarianUmbrella));
    CHECK_FALSE(p.HasFlag(nccu::kFlagHasTrueUmbrella));      // 不是真傘

    // 背包顯示一行管理員的傘，沒有真傘那一行。
    const auto rows = nccu::BuildInventoryRows(p);
    int loanerRows = 0;
    for (const auto& r : rows)
        if (r.itemId == nccu::kItemLoanerUmbrella) ++loanerRows;
    CHECK(loanerRows == 1);

    // 冪等：重複對話不疊加、不重複授予。
    nccu::TryLendLibrarianUmbrella(p, "librarian",
                                   SemesterState::Chapter2_Midterms);
    CHECK(p.HeldUmbrellaKind() == HeldUmbrella::Loaner);
    EventBus::Instance().Clear();
}

// 叫醒前即使持有三張筆記也不得觸發換回：沉睡的學霸（未設叫醒旗標）無法換回。
TEST_CASE("TryRescueBookworm: notes BEFORE waking never trigger the exchange") {
    // 守門驗證：即使三張筆記齊全，沉睡（未設叫醒旗標）的學霸也不能換回。
    // 沒有飲料時首次對話只會提示；在設下叫醒旗標前都無法換回。（正式版中
    // 筆記在叫醒前根本不會存在——World 守住其生成——但任務邏輯本身也必須拒絕換回。）
    EventBus::Instance().Clear();
    Player p = MakePlayer();
    GiveNotes(p);                              // 假設筆記莫名已在手上
    nccu::TryRescueBookworm(EventBus::Instance(), p, "bookworm", SemesterState::Chapter2_Midterms);
    CHECK_FALSE(p.HasFlag(nccu::kFlagBookwormRecovered));  // 沉睡 -> 不可換回
    CHECK_FALSE(p.HasFlag(nccu::kFlagBookworm));      // 沒飲料 -> 仍沉睡
    EventBus::Instance().Clear();
}

// LiftChapter2Clear 延後到「已換回且收尾對話關閉」之後才生效；章節不符時永不生效。
TEST_CASE("LiftChapter2Clear: deferred behind recovery + a closed dialog") {
    Player p = MakePlayer();
    nccu::DialogState d;

    nccu::LiftChapter2Clear(p, SemesterState::Chapter2_Midterms, d);
    CHECK_FALSE(p.HasFlag(nccu::kFlagCh2Cleared));   // 尚未換回

    p.SetFlag(nccu::kFlagBookwormRecovered);
    d.Open({"那個……算我欠你一個。"});
    REQUIRE(d.Active());
    nccu::LiftChapter2Clear(p, SemesterState::Chapter2_Midterms, d);
    CHECK_FALSE(p.HasFlag(nccu::kFlagCh2Cleared));   // (d) 還在畫面上

    d.Close();
    nccu::LiftChapter2Clear(p, SemesterState::Chapter2_Midterms, d);
    CHECK(p.HasFlag(nccu::kFlagCh2Cleared));          // 已生效

    // 章節不符永不生效。
    Player q = MakePlayer();
    q.SetFlag(nccu::kFlagBookwormRecovered);
    nccu::LiftChapter2Clear(q, SemesterState::Chapter3_SportsDay, d);
    CHECK_FALSE(q.HasFlag(nccu::kFlagCh2Cleared));
}

// 硬性關卡：見過圖書館管理員（Flag_MetLibrarian）前學霸無法被叫醒——即使持飲料也只得到導向提示、不消耗也不叫醒；見過後同一個對話即可叫醒。
TEST_CASE("A2: 學霸 cannot be woken before the 圖書館管理員 is met") {
    EventBus::Instance().Clear();
    Player p = MakePlayer();
    p.AddConsumable("EnergyDrink");                       // 持有飲料……

    // ……但尚未見過管理員 -> 導向提示，什麼都不消耗、也不叫醒。
    nccu::TryRescueBookworm(EventBus::Instance(), p, "bookworm", SemesterState::Chapter2_Midterms);
    CHECK_FALSE(p.HasFlag(nccu::kFlagBookworm));
    CHECK(p.ConsumableCount("EnergyDrink") == 1);        // 飲料未動

    // TryMeetLibrarian 對象／章節不符時為無操作。
    nccu::TryMeetLibrarian(p, "bookworm", SemesterState::Chapter2_Midterms);
    CHECK_FALSE(p.HasFlag(nccu::kFlagMetLibrarian));
    nccu::TryMeetLibrarian(p, "librarian", SemesterState::Chapter1_AddDrop);
    CHECK_FALSE(p.HasFlag(nccu::kFlagMetLibrarian));

    // 見過管理員（Ch2 鏈頭）-> 關卡開啟。
    nccu::TryMeetLibrarian(p, "librarian", SemesterState::Chapter2_Midterms);
    CHECK(p.HasFlag(nccu::kFlagMetLibrarian));

    // 現在同一個叫醒對話成功（飲料消耗、已叫醒）。
    nccu::TryRescueBookworm(EventBus::Instance(), p, "bookworm", SemesterState::Chapter2_Midterms);
    CHECK(p.HasFlag(nccu::kFlagBookworm));
    CHECK(p.ConsumableCount("EnergyDrink") == 0);
    EventBus::Instance().Clear();
}

// 對話面的同一關卡：見過管理員前，學霸對話被導向純台詞提示（無分支、無正常 (a) 重述）；見過後恢復正常路由。
TEST_CASE("A2: 學霸 dialog redirects to the 管理員 until she is met") {
    nccu::dialog::SetContentDir(TEST_CONTENT_DIR);
    nccu::dialog::Reload();
    const auto Ch2 = SemesterState::Chapter2_Midterms;

    // 尚未見過管理員 -> 顯示導向提示（首行是趴著的橋段，而非 chapter2.md
    // 學霸 (a) 的「……嗯？你說什麼。」）。
    Player p = MakePlayer();
    nccu::DialogState d;
    nccu::OpenNpcDialog(d, p, "bookworm", Ch2);
    REQUIRE(d.Active());
    CHECK(d.CurrentLine() != "……嗯？你說什麼。");          // 不是 (a) 開場
    CHECK_FALSE(d.AtChoice());                             // 純台詞

    // 已見過管理員 -> 顯示正常的 (a) 首次接觸開場。
    Player q = MakePlayer();
    q.SetFlag(nccu::kFlagMetLibrarian);
    nccu::DialogState d2;
    nccu::OpenNpcDialog(d2, q, "bookworm", Ch2);
    REQUIRE(d2.Active());
    CHECK(d2.CurrentLine() == "……嗯？你說什麼。");          // (a) 開場
}

// 換回學霸筆記（設下 Flag_BookwormRecovered 的那次交換）必須清掉三個筆記旗標，讓背包的任務紙張那一行消失。
TEST_CASE("A3: returning the 學霸's notes clears the bag 任務紙張 row") {
    EventBus::Instance().Clear();
    Player p = MakePlayer();
    p.SetFlag(nccu::kFlagMetLibrarian);                  // 鏈頭

    // 叫醒他，再收齊三張筆記——背包顯示筆記那一行。
    p.AddConsumable("EnergyDrink");
    nccu::TryRescueBookworm(EventBus::Instance(), p, "bookworm", SemesterState::Chapter2_Midterms);
    REQUIRE(p.HasFlag(nccu::kFlagBookworm));
    GiveNotes(p);
    {
        const auto rows = nccu::BuildInventoryRows(p);
        int noteRows = 0;
        for (const auto& r : rows)
            if (r.itemId == nccu::kItemNotes) ++noteRows;
        REQUIRE(noteRows == 1);                          // 任務紙張存在
    }

    // 把筆記交回（換回）-> 已換回，且筆記旗標被清掉。
    nccu::TryRescueBookworm(EventBus::Instance(), p, "bookworm", SemesterState::Chapter2_Midterms);
    REQUIRE(p.HasFlag(nccu::kFlagBookwormRecovered));
    CHECK_FALSE(p.HasFlag(nccu::kFlagFoundNote1));
    CHECK_FALSE(p.HasFlag(nccu::kFlagFoundNote2));
    CHECK_FALSE(p.HasFlag(nccu::kFlagFoundNote3));

    // 背包的任務紙張那一行已消失（洩漏已修正）。
    const auto rows = nccu::BuildInventoryRows(p);
    int noteRows = 0;
    for (const auto& r : rows)
        if (r.itemId == nccu::kItemNotes) ++noteRows;
    CHECK(noteRows == 0);
    EventBus::Instance().Clear();
}

// Ch2 主線端到端：見管理員→喝飲料叫醒→撿筆記→換回→清關，最後經閘門進入幕間市集並把 returnTo 設為 Ch3。
TEST_CASE("Ch2 quest reaches the Interlude via the existing spine") {
    EventBus::Instance().Clear();
    nccu::SemesterStateMachine m;
    Player p = MakePlayer();
    nccu::DialogState d;
    m.Transition(SemesterState::Chapter2_Midterms);

    // 新流程：先見管理員（鏈頭）、喝飲料叫醒，再收筆記，最後換回。
    p.SetFlag(nccu::kFlagMetLibrarian);
    p.AddConsumable("EnergyDrink");
    nccu::TryRescueBookworm(EventBus::Instance(), p, "bookworm", m.Current());   // 叫醒（階段 1）
    REQUIRE(p.HasFlag(nccu::kFlagBookworm));
    GiveNotes(p);
    nccu::TryRescueBookworm(EventBus::Instance(), p, "bookworm", m.Current());   // 換回（階段 2）
    REQUIRE(p.HasFlag(nccu::kFlagBookwormRecovered));
    nccu::LiftChapter2Clear(p, m.Current(), d);       // 對話已關閉
    REQUIRE(p.HasFlag(nccu::kFlagCh2Cleared));

    nccu::CheckChapterGates(EventBus::Instance(), p, m, d);
    CHECK(m.Current() == SemesterState::Interlude_Market);
    CHECK(m.InterludeReturnTo() == SemesterState::Chapter3_SportsDay);
    EventBus::Instance().Clear();
}
