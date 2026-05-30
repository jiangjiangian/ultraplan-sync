#include "doctest/doctest.h"
#include "engine/events/EventBus.h"
#include "game/quest/Flags.h"
#include "game/state/EndingGate.h"
#include "game/state/SemesterStateMachine.h"
#include "game/dialog/DialogState.h"
#include "game/entities/Player.h"
#include "ui/EndingView.h"   // IsEndingState
#include "engine/math/Vec2.h"
using nccu::SemesterStateMachine;
using nccu::SemesterState;
using nccu::IsEndingState;

/**
 * @file test_ending_gate.cpp
 * @brief 驗證 CheckEndingGates 的結局判定：四種結局 A/B/C/D 的觸發條件、
 *        優先順序、僅限第四章生效、助教最終選擇後判定的「完全性」（不會卡關），
 *        以及對話進行中延後判定的行為。
 */

// 各結局都在 CheckEndingGates 中決定，且全部僅限第四章（Chapter4_Finals）生效。
// 結局 C 真正的觸發點是第四章集英樓商人 → Flag_BoughtUglyUmbrella；第一章阿姨那段
// 純屬敘事鋪陳，不影響判定。SemesterStateMachine 不可移動，故每個 case 都就地建構。

// 在第四章以外的任何章節，都不會觸發任何結局判定。
TEST_CASE("結局判定：在 Chapter4_Finals 以外不會觸發任何結局") {
    for (auto s : {SemesterState::Chapter1_AddDrop,
                   SemesterState::Interlude_Market,
                   SemesterState::Chapter2_Midterms,
                   SemesterState::Chapter3_SportsDay}) {
        SemesterStateMachine m;
        if (s != SemesterState::Chapter1_AddDrop) m.Transition(s);
        Player p{nccu::engine::math::Vec2{0, 0}};
        nccu::DialogState d;
        // 設好每種結局的旗標 — 在第四章之前都不該觸發。
        p.SetFlag(nccu::kFlagHasTrueUmbrella);
        p.SetFlag(nccu::kFlagConsoledTA);
        p.SetFlag(nccu::kFlagTookCursedUmbrella);
        p.SetFlag(nccu::kFlagBoughtUglyUmbrella);
        p.AddKarma(100);
        nccu::CheckEndingGates(EventBus::Instance(), p, m, d);
        CHECK(m.Current() == s);
    }
}

// 結局 A：業力>80 且持有真傘且體諒助教，三項皆滿足才到結局 A。
TEST_CASE("結局 A：karma>80 + TrueUmbrella + 體諒 -> Ending A") {
    SemesterStateMachine m; m.Transition(SemesterState::Chapter4_Finals);
    Player p{nccu::engine::math::Vec2{0, 0}};
    nccu::DialogState d;
    p.AddKarma(100);                         // > 80（上限夾為 100）
    p.SetFlag(nccu::kFlagHasTrueUmbrella);
    p.SetFlag(nccu::kFlagConsoledTA);
    nccu::CheckEndingGates(EventBus::Instance(), p, m, d);
    CHECK(m.Current() == SemesterState::Ending_A);
}

// 結局 A：缺任一條件就不會到 A（缺體諒時停留第四章，其餘缺項落到 D）。
TEST_CASE("結局 A：缺任一條件就不會到 Ending A") {
    SUBCASE("無體諒（無 Flag_ConsoledTA）-> 非 A，且因此停留 Ch4") {
        // 業力>80 + 持真傘但從未選擇體諒：A 需三項齊備，且沒有 Flag_ConsoledTA
        // 時 D 判定也不觸發，故此狀態（尚未做最終選擇）正確停留第四章直到玩家決定。
        SemesterStateMachine m; m.Transition(SemesterState::Chapter4_Finals);
        Player p{nccu::engine::math::Vec2{0, 0}}; nccu::DialogState d;
        p.AddKarma(100); p.SetFlag(nccu::kFlagHasTrueUmbrella);
        nccu::CheckEndingGates(EventBus::Instance(), p, m, d);
        CHECK(m.Current() == SemesterState::Chapter4_Finals);
    }
    SUBCASE("無 TrueUmbrella 但體諒+karma>80 -> Ending D，非 A") {
        // 已選體諒但缺少奪回的真傘：落到苦澀的 D（風雨同行），而非 A。
        SemesterStateMachine m; m.Transition(SemesterState::Chapter4_Finals);
        Player p{nccu::engine::math::Vec2{0, 0}}; nccu::DialogState d;
        p.AddKarma(100); p.SetFlag(nccu::kFlagConsoledTA);
        nccu::CheckEndingGates(EventBus::Instance(), p, m, d);
        CHECK(m.Current() == SemesterState::Ending_D);
    }
    SUBCASE("karma 未 > 80 但體諒+持真傘 -> Ending D，非 A") {
        // 已選體諒、持真傘，但業力落在 [0,80]：到 D。
        SemesterStateMachine m; m.Transition(SemesterState::Chapter4_Finals);
        Player p{nccu::engine::math::Vec2{0, 0}}; nccu::DialogState d;   // 預設約 50
        p.SetFlag(nccu::kFlagHasTrueUmbrella); p.SetFlag(nccu::kFlagConsoledTA);
        nccu::CheckEndingGates(EventBus::Instance(), p, m, d);
        CHECK(m.Current() == SemesterState::Ending_D);
    }
}

// 結局 B：拿了詛咒傘，或業力低於零。
TEST_CASE("結局 B：拿了詛咒傘或 karma<0 -> Ending B") {
    SUBCASE("詛咒傘") {
        SemesterStateMachine m; m.Transition(SemesterState::Chapter4_Finals);
        Player p{nccu::engine::math::Vec2{0, 0}}; nccu::DialogState d;
        p.SetFlag(nccu::kFlagTookCursedUmbrella);
        nccu::CheckEndingGates(EventBus::Instance(), p, m, d);
        CHECK(m.Current() == SemesterState::Ending_B);
    }
    SUBCASE("業力低於零") {
        SemesterStateMachine m; m.Transition(SemesterState::Chapter4_Finals);
        Player p{nccu::engine::math::Vec2{0, 0}}; nccu::DialogState d;
        p.AddKarma(-200);                    // 夾為 -100（< 0）
        nccu::CheckEndingGates(EventBus::Instance(), p, m, d);
        CHECK(m.Current() == SemesterState::Ending_B);
    }
}

// 結局 C：買了醜傘。
TEST_CASE("結局 C：買了醜傘 -> Ending C") {
    SemesterStateMachine m; m.Transition(SemesterState::Chapter4_Finals);
    Player p{nccu::engine::math::Vec2{0, 0}};
    nccu::DialogState d;
    p.SetFlag(nccu::kFlagBoughtUglyUmbrella);
    nccu::CheckEndingGates(EventBus::Instance(), p, m, d);
    CHECK(m.Current() == SemesterState::Ending_C);
}

// 第一章阿姨那段買傘純屬敘事鋪陳，不會單獨觸發結局 C。
TEST_CASE("結局判定：Ch1 阿姨買傘純屬敘事鋪陳，不會觸發 Ending C") {
    // 第一章那個橋段不會在玩家狀態留下任何旗標；唯有第四章集英樓商人設下的
    // Flag_BoughtUglyUmbrella 才會觸發結局 C。此處保證：單走第一章那段不會到 C。
    SemesterStateMachine m; m.Transition(SemesterState::Chapter4_Finals);
    Player p{nccu::engine::math::Vec2{0, 0}};
    nccu::DialogState d;
    // 確認沒有那個第一章旗標 — 鋪陳橋段並未改動玩家狀態。
    CHECK_FALSE(p.HasFlag(nccu::kFlagBoughtUglyUmbrella));
    nccu::CheckEndingGates(EventBus::Instance(), p, m, d);
    CHECK(m.Current() == SemesterState::Chapter4_Finals);   // 不觸發結局
}

// 結局判定的優先順序：A > B > C。
TEST_CASE("結局判定：優先順序 A > B > C") {
    SemesterStateMachine m; m.Transition(SemesterState::Chapter4_Finals);
    Player p{nccu::engine::math::Vec2{0, 0}};
    nccu::DialogState d;
    // 三者同時成立時：誠實路線（A）勝出。
    p.AddKarma(100);
    p.SetFlag(nccu::kFlagHasTrueUmbrella);
    p.SetFlag(nccu::kFlagConsoledTA);
    p.SetFlag(nccu::kFlagTookCursedUmbrella);
    p.SetFlag(nccu::kFlagBoughtUglyUmbrella);
    nccu::CheckEndingGates(EventBus::Instance(), p, m, d);
    CHECK(m.Current() == SemesterState::Ending_A);
}

// 沒有任何旗標時，停留在第四章。
TEST_CASE("結局判定：沒有任何旗標時停留在 Ch4") {
    SemesterStateMachine m; m.Transition(SemesterState::Chapter4_Finals);
    Player p{nccu::engine::math::Vec2{0, 0}};
    nccu::DialogState d;
    nccu::CheckEndingGates(EventBus::Instance(), p, m, d);
    CHECK(m.Current() == SemesterState::Chapter4_Finals);
}

// 助教最終橋段的判定必須是「完全的」：一旦做出最終選擇，無論其餘旗標如何，
// 都會落到某個結局，絕不卡在第四章。曾有的卡關情境：玩家走完整條誠實路線、
// 抵達第四章、在助教最終選擇選了「質問／強硬索回」而非體諒 —— 該選擇會設下
// Flag_TaFinaleChoiceMade 並永久鎖住選單（之後只能回顧、無法重選），卻又不符合
// A/B/C 任一條件，於是 CheckEndingGates 全部落空、永遠卡在第四章。一個無法通關的
// 狀態是嚴重缺陷。修正：做出最終選擇後判定即為完全 —— 冷漠選擇 → B（屠龍者終成
// 惡龍），體諒但不完美 → 視業力與真傘落到 C 或 D。
TEST_CASE("結局判定：助教最終橋段為完全判定，不會落空卡關") {
    SUBCASE("冷漠結尾（質問/強硬、誠實路線）-> Ending B") {
        // 完整誠實路線（高業力、已奪回真傘），但選擇不體諒助教。修正前此情境
        // 會落空於每一道判定而卡關。
        SemesterStateMachine m; m.Transition(SemesterState::Chapter4_Finals);
        Player p{nccu::engine::math::Vec2{0, 0}}; nccu::DialogState d;
        p.AddKarma(100);                          // 誠實路線，業力 95-100
        p.SetFlag(nccu::kFlagHasTrueUmbrella);        // 第四章奪回
        p.SetFlag(nccu::kFlagTaFinaleChoiceMade);     // 已做出最終選擇
        // 無 Flag_ConsoledTA（選了質問／強硬）、無詛咒傘、未買醜傘。
        nccu::CheckEndingGates(EventBus::Instance(), p, m, d);
        CHECK(m.Current() == SemesterState::Ending_B);   // 修正前：卡在第四章
    }
    SUBCASE("體諒但 karma<=80（不完美的誠實）-> Ending D") {
        // 體諒但業力落在 [0,80]：風雨同行（破傘），而非破財消災。仍為完全（會落到結局）。
        SemesterStateMachine m; m.Transition(SemesterState::Chapter4_Finals);
        Player p{nccu::engine::math::Vec2{0, 0}}; nccu::DialogState d;  // 業力約 50
        p.SetFlag(nccu::kFlagConsoledTA);             // 選了體諒
        p.SetFlag(nccu::kFlagTaFinaleChoiceMade);
        // 業力不 > 80 -> 非 A；無詛咒/非冷漠 -> 非 B；體諒 -> D。
        nccu::CheckEndingGates(EventBus::Instance(), p, m, d);
        CHECK(m.Current() == SemesterState::Ending_D);
    }
    SUBCASE("體諒 + karma>80 但無 TrueUmbrella -> Ending D") {
        // 體諒、高業力，但缺奪回的真傘：錯過 A，落到較溫暖的 D，而非買單的 C。
        SemesterStateMachine m; m.Transition(SemesterState::Chapter4_Finals);
        Player p{nccu::engine::math::Vec2{0, 0}}; nccu::DialogState d;
        p.AddKarma(100);
        p.SetFlag(nccu::kFlagConsoledTA);
        p.SetFlag(nccu::kFlagTaFinaleChoiceMade);
        // 缺 Flag_HasTrueUmbrella -> 非 A；已體諒 -> 非冷漠 B -> D。
        nccu::CheckEndingGates(EventBus::Instance(), p, m, d);
        CHECK(m.Current() == SemesterState::Ending_D);
    }
    SUBCASE("優先順序：冷漠結尾絕不凌駕已達成的 A") {
        // 已選體諒且完整達成 -> A 仍勝出（雖設了最終選擇旗標，但 Flag_ConsoledTA
        // 也設了，故 coldFinale=false）。
        SemesterStateMachine m; m.Transition(SemesterState::Chapter4_Finals);
        Player p{nccu::engine::math::Vec2{0, 0}}; nccu::DialogState d;
        p.AddKarma(100);
        p.SetFlag(nccu::kFlagHasTrueUmbrella);
        p.SetFlag(nccu::kFlagConsoledTA);
        p.SetFlag(nccu::kFlagTaFinaleChoiceMade);
        nccu::CheckEndingGates(EventBus::Instance(), p, m, d);
        CHECK(m.Current() == SemesterState::Ending_A);
    }
}

// 在和助教對話前的第四章自由探索期間，玩家不會被推進任何結局。
TEST_CASE("結局判定：助教對話前的 Ch4 自由探索不會被推進結局") {
    // C 的預設分支嚴格以 Flag_TaFinaleChoiceMade 為條件，因此在和助教對話之前
    // 探索第四章的玩家絕不會被硬塞進結局。只達成部分 A 條件、或完全空白且未設
    // 最終選擇旗標的狀態，都必須停留第四章（對照先前「缺任一條件」/「無旗標」
    // 的保證，這裡用新的完全判定邏輯再驗一次，鎖住關鍵的判定鑰匙而非僅快樂路徑）。
    SUBCASE("karma>80 + TrueUmbrella，尚未做最終選擇 -> 停留 Ch4") {
        SemesterStateMachine m; m.Transition(SemesterState::Chapter4_Finals);
        Player p{nccu::engine::math::Vec2{0, 0}}; nccu::DialogState d;
        p.AddKarma(100); p.SetFlag(nccu::kFlagHasTrueUmbrella);
        // 無 Flag_ConsoledTA、無 Flag_TaFinaleChoiceMade
        nccu::CheckEndingGates(EventBus::Instance(), p, m, d);
        CHECK(m.Current() == SemesterState::Chapter4_Finals);
    }
    SUBCASE("無旗標，尚未做最終選擇 -> 停留 Ch4") {
        SemesterStateMachine m; m.Transition(SemesterState::Chapter4_Finals);
        Player p{nccu::engine::math::Vec2{0, 0}}; nccu::DialogState d;
        nccu::CheckEndingGates(EventBus::Instance(), p, m, d);
        CHECK(m.Current() == SemesterState::Chapter4_Finals);
    }
}

// 結局不可突兀：對話／旁白還在畫面上時，CheckEndingGates 會延後判定（避免
// 「按下選項後突然跳結局」），等對話框關閉後的下一次輪詢才落到結局。
TEST_CASE("結局判定：對話進行中會延後，關閉後才落到結局") {
    SemesterStateMachine m; m.Transition(SemesterState::Chapter4_Finals);
    Player p{nccu::engine::math::Vec2{0, 0}};
    nccu::DialogState d;
    d.Open({"queued closing narration line"});
    REQUIRE(d.Active());
    p.SetFlag(nccu::kFlagBoughtUglyUmbrella);

    // 旁白還在時判定必須什麼都不做 — 讓玩家先讀完。
    nccu::CheckEndingGates(EventBus::Instance(), p, m, d);
    CHECK(m.Current() == SemesterState::Chapter4_Finals);   // 已延後
    CHECK(d.Active());                                       // 對話框未受影響

    // 玩家讀完並關閉對話框；下一次輪詢才落到（持久旗標決定的）結局，並關閉
    // 任何殘留對話。
    d.Close();
    nccu::CheckEndingGates(EventBus::Instance(), p, m, d);
    CHECK(m.Current() == SemesterState::Ending_C);
    CHECK_FALSE(d.Active());                  // 終局畫面：無殘留對話
}

// ====================================================================
// 四種結局的判定樹 A -> B -> D -> C，以及「判定為完全」的證明。
// ====================================================================

// 輔助函式：以全新的第四章玩家（無進行中對話）解析判定，回傳落到的狀態。
// 每一列對應一種最終結果。
namespace {
SemesterState ResolveCh4(int karma, bool trueUmb, bool consoled,
                         bool finaleMade, bool cursed, bool boughtUgly) {
    SemesterStateMachine m; m.Transition(SemesterState::Chapter4_Finals);
    Player p{nccu::engine::math::Vec2{0, 0}}; nccu::DialogState d;
    p.AddKarma(karma - 50);                   // 起始為 50；調整到目標 karma
    if (trueUmb)    p.SetFlag(nccu::kFlagHasTrueUmbrella);
    if (consoled)   p.SetFlag(nccu::kFlagConsoledTA);
    if (finaleMade) p.SetFlag(nccu::kFlagTaFinaleChoiceMade);
    if (cursed)     p.SetFlag(nccu::kFlagTookCursedUmbrella);
    if (boughtUgly) p.SetFlag(nccu::kFlagBoughtUglyUmbrella);
    nccu::CheckEndingGates(EventBus::Instance(), p, m, d);
    return m.Current();
}
}  // namespace

// 四種結局判定樹 A -> B -> D -> C 的逐項對照表。
TEST_CASE("A->B->D->C 結局判定樹的逐項對照") {
    using S = SemesterState;
    // A — 業力>80 + 持真傘 + 體諒。
    CHECK(ResolveCh4(100, true,  true,  true,  false, false) == S::Ending_A);
    // 體諒 + 業力<=80（-> D，非 C）。
    CHECK(ResolveCh4(50,  true,  true,  true,  false, false) == S::Ending_D);
    CHECK(ResolveCh4(80,  true,  true,  true,  false, false) == S::Ending_D); // 邊界：80 不算 >80
    // 體諒 + 業力>80 但無真傘（-> D，非 A、非 C）。
    CHECK(ResolveCh4(100, false, true,  true,  false, false) == S::Ending_D);
    // 質問（已做最終選擇但未體諒）-> 冷漠結局 -> B。
    CHECK(ResolveCh4(95,  true,  false, true,  false, false) == S::Ending_B);
    // 僅買醜傘（未做最終選擇）-> C。
    CHECK(ResolveCh4(50,  false, false, false, false, true)  == S::Ending_C);
    // 詛咒傘 -> B（凌駕 A 以下的所有條件）。
    CHECK(ResolveCh4(50,  false, false, false, true,  false) == S::Ending_B);
    CHECK(ResolveCh4(50,  true,  true,  true,  true,  false) == S::Ending_B); // 詛咒傘勝過本應的 D
    // 負業力 -> B。
    CHECK(ResolveCh4(-100, false, false, false, false, false) == S::Ending_B);
    // D 優先於 C：既體諒又買了醜傘的玩家，得到較溫暖的 D（道德選擇勝過購物決定）。
    CHECK(ResolveCh4(50,  true,  true,  true,  false, true)  == S::Ending_D);
}

// 一旦做出最終選擇，判定即為完全：必落到某結局、永不卡關。
TEST_CASE("做出最終選擇後判定即為完全：必落到某結局、永不卡關") {
    using S = SemesterState;
    // 在設定 Flag_TaFinaleChoiceMade（唯一可能「卡住」的前提）下，窮舉所有
    // 與最終結局相關旗標的組合：判定必定離開第四章。這是四結局判定樹的可通關性
    // 保證（把完全性證明延伸到 D）。
    for (int karma : {-100, 0, 50, 80, 81, 100}) {
        for (bool trueUmb : {false, true}) {
            for (bool consoled : {false, true}) {
                for (bool cursed : {false, true}) {
                    for (bool ugly : {false, true}) {
                        const S r = ResolveCh4(karma, trueUmb, consoled,
                                               /*finaleMade=*/true, cursed, ugly);
                        CHECK(r != S::Chapter4_Finals);   // 絕不卡住
                        CHECK(IsEndingState(r));          // 必為某個結局
                    }
                }
            }
        }
    }
}
