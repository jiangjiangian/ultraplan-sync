/**
 * @file test_ch2_reactive_substates.cpp
 * @brief 驗證 Ch2 四段反應式台詞已改寫為真正的旗標分支子狀態，且各台詞只在其對應旗標下載入並可達。
 */
#include "game/quest/Flags.h"
// Ch2 原本以行內標註撰寫的四段反應式台詞（DialogLoader 會默默丟棄的
// 「*（若 Flag_X = true）*」這類行）已改寫為 chapter2.md 中真正受旗標守門的
// 獨立子狀態，並由 ResolveOpenerSubState 負責路由。以下案例證明每段反應式
// 台詞確實會載入，且只在其守門旗標下才可達——其他情況不會洩漏。
#include "doctest/doctest.h"
#include "game/dialog/DialogOpener.h"
#include "game/dialog/DialogSource.h"
#include "game/dialog/DialogState.h"
#include "game/entities/Player.h"
#include "game/quest/Chapter2Quest.h"
#include "engine/math/Vec2.h"

#ifndef TEST_CONTENT_DIR
#error "TEST_CONTENT_DIR must be defined by the build system"
#endif

using nccu::SemesterState;

namespace {
Player MakePlayer() { return Player{nccu::engine::math::Vec2{0.0f, 0.0f}}; }
constexpr auto kCh2 = SemesterState::Chapter2_Midterms;

// 該子狀態的任一行台詞是否等於 needle？
bool SubHasLine(std::string_view npc, int sub, const std::string& needle) {
    for (const auto& e : nccu::dialog::Entries(npc, kCh2))
        if (e.subState == sub)
            for (const auto& l : e.lines)
                if (l == needle) return true;
    return false;
}
}  // namespace

// 改寫後的 chapter2.md 反應式子狀態確實載入了對應台詞。
TEST_CASE("chapter2.md 改寫後的反應式子狀態確實載入對應台詞") {
    nccu::dialog::SetContentDir(TEST_CONTENT_DIR);
    nccu::dialog::Reload();

    // 學霸 (b) = 對詛咒傘的冷反應（子狀態 1）。
    CHECK(SubHasLine("bookworm", 1, "……你今天感覺有點怪。"));
    // 福利社阿姨 (b) = 認出醜傘（子狀態 1）。
    CHECK(SubHasLine("shop_auntie", 1, "你那把螢光綠的沒帶來嗎？辨識度那麼高。"));
    // 苦主 (c) = 承諾回呼（子狀態 2）。
    CHECK(SubHasLine("victim", 2, "你說過你會找——你真的還在找。"));
    // 苦主 (d) = 認出醜傘（子狀態 3）。
    CHECK(SubHasLine("victim", 3, "那個螢光綠的是你的嗎？辨識度很高。"));
}

// Ch2 路由只在各自的旗標下才會抵達對應的反應式子狀態。
TEST_CASE("Ch2 路由只在各自旗標下才抵達對應的反應式子狀態") {
    nccu::dialog::SetContentDir(TEST_CONTENT_DIR);
    nccu::dialog::Reload();

    SUBCASE("學霸：持詛咒傘 -> (b)，否則 (a)") {
        Player p = MakePlayer();
        CHECK(nccu::ResolveOpenerSubState("bookworm", kCh2, p) == 0);  // (a)
        p.SetFlag(nccu::kFlagTookCursedUmbrella);
        CHECK(nccu::ResolveOpenerSubState("bookworm", kCh2, p) == 1);  // (b)
        // 救回狀態的優先序仍高於詛咒變體。
        p.SetFlag(nccu::kFlagBookwormRecovered);
        CHECK(nccu::ResolveOpenerSubState("bookworm", kCh2, p) == 3);  // (d)
    }
    SUBCASE("福利社阿姨：持醜傘 -> (b)，否則 (a)") {
        Player p = MakePlayer();
        CHECK(nccu::ResolveOpenerSubState("shop_auntie", kCh2, p) == 0);
        p.SetFlag(nccu::kFlagBoughtUglyUmbrella);
        CHECK(nccu::ResolveOpenerSubState("shop_auntie", kCh2, p) == 1);
    }
    SUBCASE("苦主：承諾 -> (c)；僅持醜傘 -> (d)；承諾勝出") {
        Player p = MakePlayer();
        CHECK(nccu::ResolveOpenerSubState("victim", kCh2, p) == 0);    // (a)
        p.SetFlag(nccu::kFlagBoughtUglyUmbrella);
        CHECK(nccu::ResolveOpenerSubState("victim", kCh2, p) == 3);    // (d)
        p.SetFlag(nccu::kFlagPromisedVictim);
        CHECK(nccu::ResolveOpenerSubState("victim", kCh2, p) == 2);    // (c)
    }
}

// OpenNpcDialog 端到端能開啟對應的反應式台詞（學霸詛咒 (b)、苦主承諾 (c)），且 (a) 預設不含詛咒台詞。
TEST_CASE("OpenNpcDialog 端到端開啟對應的反應式台詞") {
    nccu::dialog::SetContentDir(TEST_CONTENT_DIR);
    nccu::dialog::Reload();

    SUBCASE("學霸詛咒：首行是路由到的 (b) 開場，純台詞") {
        Player p = MakePlayer();
        // 硬性關卡：學霸在見到圖書館管理員（Flag_MetLibrarian）前是無法互動的——
        // 一開始找他只會看到他趴著，並有提示指向櫃台。先見過管理員，這個子案例
        // 才能測到真正的詛咒 (b) 反應（它被擋在鏈頭之後，而非首次接觸即可繞過）。
        p.SetFlag(nccu::kFlagMetLibrarian);
        p.SetFlag(nccu::kFlagTookCursedUmbrella);
        nccu::DialogState d;
        nccu::OpenNpcDialog(d, p, "bookworm", kCh2);
        REQUIRE(d.Active());
        CHECK_FALSE(d.AtChoice());
        CHECK(d.CurrentLine() == "……嗯？你說什麼。");        // (b) 第 0 行
        bool sawCursed = false;
        for (int i = 0; i < 16 && d.Active(); ++i) {
            if (d.CurrentLine() == "……你今天感覺有點怪。") sawCursed = true;
            d.Advance();
        }
        CHECK(sawCursed);                                     // 台詞有顯示
    }
    SUBCASE("苦主承諾：(c) 重述會抵達回呼台詞") {
        Player p = MakePlayer();
        p.SetFlag(nccu::kFlagPromisedVictim);
        nccu::DialogState d;
        nccu::OpenNpcDialog(d, p, "victim", kCh2);
        REQUIRE(d.Active());
        CHECK(d.CurrentLine() == "你也在圖書館備考嗎。");      // (c) 第 0 行
        bool sawPromise = false;
        for (int i = 0; i < 16 && d.Active(); ++i) {
            if (d.CurrentLine() == "你說過你會找——你真的還在找。")
                sawPromise = true;
            d.Advance();
        }
        CHECK(sawPromise);
    }
    SUBCASE("無旗標：學霸預設 (a) 不含詛咒台詞") {
        Player p = MakePlayer();
        nccu::DialogState d;
        nccu::OpenNpcDialog(d, p, "bookworm", kCh2);
        REQUIRE(d.Active());
        bool sawCursed = false;
        for (int i = 0; i < 16 && d.Active(); ++i) {
            if (d.CurrentLine() == "……你今天感覺有點怪。") sawCursed = true;
            d.Advance();
        }
        CHECK_FALSE(sawCursed);   // 詛咒台詞不得洩漏進 (a)
    }
}
