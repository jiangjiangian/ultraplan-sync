/**
 * @file test_ch1_quest.cpp
 * @brief 驗證 Ch1 互惠主線：苦主→學長→傘→還傘的授予與延後清關、學長選單硬性關卡、傘的延後生成，以及阿姨醜傘購買。
 */
#include "doctest/doctest.h"
#include "game/quest/Flags.h"
#include "game/quest/Chapter1Quest.h"
#include "game/quest/ChapterGate.h"
#include "game/quest/ChapterQuestItems.h"
#include "game/dialog/DialogState.h"
#include "game/dialog/DialogOpener.h"
#include "game/dialog/DialogSource.h"
#include "engine/events/EventBus.h"
#include "game/controller/EventWiring.h"
#include "game/entities/Player.h"
#include "game/entities/QuestFlagPickup.h"
#include "game/entities/CursedUmbrella.h"
#include "game/entities/TransparentUmbrella.h"
#include "game/state/SemesterStateMachine.h"
#include "game/world/World.h"
#include "engine/math/Vec2.h"

#include <cstddef>
#include <string>
#include <vector>

#ifndef TEST_CONTENT_DIR
#error "TEST_CONTENT_DIR must be defined by the build system"
#endif

using nccu::SemesterState;

namespace {

Player MakePlayer() { return Player{nccu::engine::math::Vec2{0.0f, 0.0f}}; }

// 擷取每次 UmbrellaClaimed 的內容，以及最後一次 ShowMessage。
// 以區域變數為鍵的 scoped 訂閱。
struct Capture {
    std::vector<std::string> umbrellaClaims;
    std::string              lastMessage;
    EventBus::Subscription   subUmb;
    EventBus::Subscription   subMsg;
};

[[nodiscard]] Capture MakeCapture() {
    Capture c;
    c.subUmb = EventBus::Instance().ScopedSubscribe(
        EventType::UmbrellaClaimed,
        [&c](const Event& e) { c.umbrellaClaims.push_back(e.text); });
    c.subMsg = EventBus::Instance().ScopedSubscribe(
        EventType::ShowMessage,
        [&c](const Event& e) { c.lastMessage = e.text; });
    return c;
}

}  // namespace

// 互惠授予：只有當玩家「既已承諾、又持有苦主的傘」時，把傘還給苦主才會設下
// Flag_HasTrueUmbrella + HasUmbrella；其餘所有狀態（章節／對象不符、未承諾、
// 已承諾但空手、已授予過）皆為無操作。授予不再就地發布 UmbrellaClaimed——
// 它延後到 LiftChapter1Clear（讓 (d) 交換對話先播）。
TEST_CASE("TryReturnVictimUmbrella: grants the真傘 only after promise + return") {
    EventBus::Instance().Clear();
    Capture cap = MakeCapture();
    Player p = MakePlayer();

    // 章節不符／對象不符 -> 無操作。
    nccu::TryReturnVictimUmbrella(EventBus::Instance(), p, "victim", SemesterState::Chapter2_Midterms);
    nccu::TryReturnVictimUmbrella(EventBus::Instance(), p, "ta", SemesterState::Chapter1_AddDrop);
    CHECK_FALSE(p.HasFlag(nccu::kFlagHasTrueUmbrella));
    CHECK(cap.umbrellaClaims.empty());

    // 尚未承諾 -> 無操作（承諾由 (a)/(b) 對話負責；還沒有東西可還）。不授予。
    nccu::TryReturnVictimUmbrella(EventBus::Instance(), p, "victim", SemesterState::Chapter1_AddDrop);
    CHECK_FALSE(p.HasFlag(nccu::kFlagHasTrueUmbrella));
    CHECK(cap.umbrellaClaims.empty());

    // 已承諾但未持有苦主的傘 -> 只提醒，不授予。
    p.SetFlag(nccu::kFlagPromisedVictim);
    nccu::TryReturnVictimUmbrella(EventBus::Instance(), p, "victim", SemesterState::Chapter1_AddDrop);
    CHECK_FALSE(p.HasFlag(nccu::kFlagHasTrueUmbrella));
    CHECK_FALSE(p.HasUmbrella());
    CHECK(cap.umbrellaClaims.empty());
    CHECK_FALSE(cap.lastMessage.empty());           // 有顯示一則提示

    // 已承諾且持有苦主的傘 -> 授予觸發（只設旗標：尚未發布 UmbrellaClaimed，(d) 交換須先播）。
    p.SetFlag(nccu::kFlagHasVictimUmbrella);
    nccu::TryReturnVictimUmbrella(EventBus::Instance(), p, "victim", SemesterState::Chapter1_AddDrop);
    CHECK(p.HasFlag(nccu::kFlagHasTrueUmbrella));        // Ending A 的條件
    CHECK(p.HasUmbrella());
    CHECK_FALSE(p.HasFlag(nccu::kFlagHasVictimUmbrella));  // 苦主收回了他的傘
    CHECK(cap.umbrellaClaims.empty());               // 清關被延後
    CHECK_FALSE(p.HasFlag(nccu::kFlagClearChapter1)); // 尚未觸發

    // 重複對話具冪等性：已授予 -> 仍不發布 UmbrellaClaimed。
    nccu::TryReturnVictimUmbrella(EventBus::Instance(), p, "victim", SemesterState::Chapter1_AddDrop);
    CHECK(cap.umbrellaClaims.empty());
    EventBus::Instance().Clear();
}

// 防禦性條件：玩家持有苦主的傘但從未承諾時，授予不得觸發（兩個條件缺一不可）。
TEST_CASE("TryReturnVictimUmbrella: umbrella without a promise never grants") {
    EventBus::Instance().Clear();
    Capture cap = MakeCapture();
    Player p = MakePlayer();
    p.SetFlag(nccu::kFlagHasVictimUmbrella);          // 有傘，但沒承諾
    nccu::TryReturnVictimUmbrella(EventBus::Instance(), p, "victim", SemesterState::Chapter1_AddDrop);
    CHECK_FALSE(p.HasFlag(nccu::kFlagHasTrueUmbrella));
    CHECK(cap.umbrellaClaims.empty());
    EventBus::Instance().Clear();
}

// 交換的順序正確：授予是靜默的（(d) 交換對話先播），唯有對話關閉後 LiftChapter1Clear
// 才發布 UmbrellaClaimed → Ch1 經事件接線轉到幕間市集（returnTo Ch2）。
// 對話仍在進行時，清關不得觸發（玩家必須先讀完這段）。
TEST_CASE("T2: victim exchange plays BEFORE the Ch1 clear (deferred)") {
    EventBus::Instance().Clear();
    nccu::SemesterStateMachine m;
    std::string buildingName;
    nccu::WireStateTransitionSubscribers(EventBus::Instance(), m, buildingName);
    Capture cap = MakeCapture();
    REQUIRE(m.Current() == SemesterState::Chapter1_AddDrop);

    Player p = MakePlayer();
    p.SetFlag(nccu::kFlagPromisedVictim);
    p.SetFlag(nccu::kFlagHasVictimUmbrella);

    // 1) 授予：旗標已設，但尚未轉場（UmbrellaClaimed 被壓住）。
    nccu::TryReturnVictimUmbrella(EventBus::Instance(), p, "victim", m.Current());
    CHECK(p.HasFlag(nccu::kFlagHasTrueUmbrella));
    CHECK(m.Current() == SemesterState::Chapter1_AddDrop);   // 仍在 Ch1
    CHECK(cap.umbrellaClaims.empty());

    // 2) (d) 交換對話在畫面上 -> 清關被擋住。
    nccu::DialogState d;
    d.Open({"這就是我的傘，太好了，謝謝你！", "還你。雨還沒停，路上小心。"});
    REQUIRE(d.Active());
    nccu::LiftChapter1Clear(EventBus::Instance(), p, m.Current(), d);
    CHECK(m.Current() == SemesterState::Chapter1_AddDrop);   // 尚未
    CHECK(cap.umbrellaClaims.empty());

    // 3) 玩家讀完、對話關閉 -> 現在才觸發。
    d.Close();
    nccu::LiftChapter1Clear(EventBus::Instance(), p, m.Current(), d);
    REQUIRE(cap.umbrellaClaims.size() == 1);
    CHECK(cap.umbrellaClaims[0] == "TrueUmbrella");
    CHECK(m.Current() == SemesterState::Interlude_Market);
    CHECK(m.InterludeReturnTo() == SemesterState::Chapter2_Midterms);

    // 4) 一次性守門：之後再輪詢不會重複發布／重複轉場。
    nccu::LiftChapter1Clear(EventBus::Instance(), p, SemesterState::Chapter1_AddDrop, d);
    CHECK(cap.umbrellaClaims.size() == 1);
    EventBus::Instance().Clear();
}

// 三結局架構不受影響：CursedUmbrella 仍可領取（其 BeClaimed 設 Flag_TookCursedUmbrella
// 並發布 UmbrellaClaimed → Ending B 路徑），且獨立於苦主的授予。
TEST_CASE("Ch1 morality umbrellas still claimable (Ending B path preserved)") {
    EventBus::Instance().Clear();
    Capture cap = MakeCapture();
    Player p = MakePlayer();
    const int k0 = p.GetKarma();

    // 詛咒傘的領取以承諾（TransparentUmbrella）為任務閘門，故先承諾再領取
    //（這與遊戲內的關卡一致）。
    p.SetFlag(nccu::kFlagPromisedVictim);
    CursedUmbrella cursed{nccu::engine::math::Vec2{0.0f, 0.0f}};
    cursed.BeClaimed(&p);

    CHECK(p.HasUmbrella());
    CHECK(p.HasFlag(nccu::kFlagTookCursedUmbrella));      // Ending B 的種子
    CHECK(p.GetCursedTaint() == 1);                   // 撿取時提升污染值
    CHECK(p.GetKarma() == k0);                        // 撿取當下 karma 中性
    REQUIRE(cap.umbrellaClaims.size() == 1);
    CHECK(cap.umbrellaClaims[0] == "CursedUmbrella"); // 它自己的清關路徑
    // 冪等。
    cursed.BeClaimed(&p);
    CHECK(cap.umbrellaClaims.size() == 1);
    EventBus::Instance().Clear();
}

// 可在世界中撿到的苦主之傘撿取物，撿起時會設下對應旗標。
TEST_CASE("Ch1 victim's-umbrella pickup sets Flag_HasVictimUmbrella") {
    EventBus::Instance().Clear();
    Capture cap = MakeCapture();

    // 它是 Ch1 唯一的任務物品，位於 (1450,1450)，沒有完成集合。
    const auto& ch1 = nccu::ChapterQuestItems(SemesterState::Chapter1_AddDrop);
    REQUIRE(ch1.size() == 1);
    const auto& qi = ch1[0];
    CHECK(qi.flag == nccu::kFlagHasVictimUmbrella);

    Player p = MakePlayer();
    QuestFlagPickup pickup{qi.pos, qi.flag, qi.message, qi.completionFlags,
                           qi.completionKarma, qi.countMessages};
    pickup.OnPickup(&p);
    CHECK(p.HasFlag(nccu::kFlagHasVictimUmbrella));
    CHECK(cap.lastMessage == qi.message);             // 它自己的撿取台詞
    EventBus::Instance().Clear();
}

// 硬性關卡：見到苦主（Flag_PromisedVictim）之前，西裝學長必須只用純台詞帶過、不開選單。
// 主線是硬性關卡 苦主 → 學長 → 傘 → 苦主；先找學長不得開啟影響漣漪的選擇選單
//（否則玩家會在章節第一拍之前就提交學長選擇、甚至領到道德傘）。承諾後才出現真正的選單。
TEST_CASE("A1: 西裝學長 redirects (no menu) until the 苦主 is met") {
    nccu::dialog::SetContentDir(TEST_CONTENT_DIR);
    nccu::dialog::Reload();
    const auto Ch1 = SemesterState::Chapter1_AddDrop;

    // 承諾前：只用純台詞帶過，絕不出現選擇選單。
    Player p = MakePlayer();
    nccu::DialogState d;
    nccu::OpenNpcDialog(d, p, "suit_senior", Ch1);
    REQUIRE(d.Active());
    bool reachedMenu = false;
    for (int i = 0; i < 32 && d.Active(); ++i) {
        if (d.AtChoice()) { reachedMenu = true; break; }
        d.Advance();
    }
    CHECK_FALSE(reachedMenu);                 // 被打發掉，從不出現選單
    CHECK(d.Choices().empty());

    // 承諾後：真正的 (b)/(c)/(d) 選擇選單開啟。
    Player q = MakePlayer();
    q.SetFlag(nccu::kFlagPromisedVictim);
    nccu::DialogState d2;
    nccu::OpenNpcDialog(d2, q, "suit_senior", Ch1);
    REQUIRE(d2.Active());
    bool reachedMenu2 = false;
    for (int i = 0; i < 32 && d2.Active(); ++i) {
        if (d2.AtChoice()) { reachedMenu2 = true; break; }
        d2.Advance();
    }
    CHECK(reachedMenu2);                       // 此時才出現分支選單
    CHECK_FALSE(d2.Choices().empty());
}

// 苦主之傘的撿取物是延後生成的：Ch1 進場時不存在於世界中，只有在
// Flag_SuitSeniorChoiceMade 之後才透過 World::MaybeSpawnChapter1VictimUmbrella
// 生成一次，並在狀態改變時隨名冊清除。因此玩家無法在學長步驟之前拿到傘——主線不可跳過。
namespace {
// 計算世界中所有的 QuestFlagPickup。Ch1 進場時正好有一個（建構子生成的申請書，
// Flag_FoundForm）；延後生成的苦主之傘撿取物是 Ch1 唯一的另一個任務物品，因此
// 數量由 1 變 2 正好代表「苦主之傘已生成」（申請書不受名冊追蹤，永不被清除）。
std::size_t CountQuestPickups(const nccu::World& w) {
    std::size_t n = 0;
    for (const auto& o : w.Objects())
        if (dynamic_cast<const QuestFlagPickup*>(o.get())) ++n;
    return n;
}
}  // namespace

// 苦主之傘延後到 Flag_SuitSeniorChoiceMade 之後才生成（且只一次），離開章節時隨名冊清除；申請書不受名冊追蹤而持續存在。
TEST_CASE("A1: 苦主's umbrella defers until Flag_SuitSeniorChoiceMade, then sweeps") {
    EventBus::Instance().Clear();
    nccu::World w("", /*loadSprites=*/false);

    // Ch1 進場時（建構子的預設狀態）苦主之傘撿取物不存在——只有建構子的申請書
    //（Flag_FoundForm）在場。
    CHECK(CountQuestPickups(w) == 1);

    // 沒有選擇旗標時，延後生成每幀都是無操作。
    CHECK_FALSE(w.MaybeSpawnChapter1VictimUmbrella());
    CHECK(CountQuestPickups(w) == 1);

    // 提交西裝學長的選擇（Flag_SuitSeniorChoiceMade）會讓苦主之傘出現一次
    //（數量 1 申請書 -> 2：多了傘）。
    REQUIRE(w.GetPlayer() != nullptr);
    w.GetPlayer()->SetFlag(nccu::kFlagSuitSeniorChoiceMade);
    CHECK(w.MaybeSpawnChapter1VictimUmbrella());        // 這一幀生成
    CHECK(CountQuestPickups(w) == 2);
    CHECK_FALSE(w.MaybeSpawnChapter1VictimUmbrella());  // 單次
    CHECK(CountQuestPickups(w) == 2);

    // 離開 Ch1 會清掉受名冊追蹤的傘（重新武裝單次旗標）；
    // 建構子的申請書不受名冊追蹤，因此持續存在（數量回到 1）。
    w.RespawnChapterRoster(SemesterState::Interlude_Market);
    CHECK(CountQuestPickups(w) == 1);
    EventBus::Instance().Clear();
}

// MaybeSpawnChapter1VictimUmbrella 在非 Ch1 時為無操作（它限定於 Ch1）。
TEST_CASE("A1: MaybeSpawnChapter1VictimUmbrella is a no-op outside Ch1") {
    EventBus::Instance().Clear();
    nccu::World w("", /*loadSprites=*/false);
    REQUIRE(w.GetPlayer() != nullptr);
    w.GetPlayer()->SetFlag(nccu::kFlagSuitSeniorChoiceMade);   // 旗標已設，但……

    // Ch2：不生成 Ch1 的傘（限定於 Ch1）。建構子的申請書不受名冊追蹤，因此跨
    // 轉場持續存在（數量維持 1）；Ch2 進場也不生成任務物品（筆記延後到叫醒旗標），
    // 所以 MaybeSpawnChapter1VictimUmbrella 不會新增任何東西。
    w.Semester().Transition(SemesterState::Chapter2_Midterms);
    w.RespawnChapterRoster(SemesterState::Chapter2_Midterms);
    CHECK_FALSE(w.MaybeSpawnChapter1VictimUmbrella());
    CHECK(CountQuestPickups(w) == 1);                   // 只有申請書
    EventBus::Instance().Clear();
}

// Ch1 福利社阿姨 (c) 購買醜綠傘是一筆真正的交易：必須扣 80 元、授予手持的醜傘、發出花費/餘額提示，
// 且關鍵在於不得設 Flag_BoughtUglyUmbrella（Ending C 的鎖由 Ch4 的 Vendor 負責）。錢不夠／重複／情境不符皆為乾淨的無操作。
TEST_CASE("B3: Ch1 阿姨 醜傘 buy deducts 80 + grants held Ugly, no Ending-C flag") {
    EventBus::Instance().Clear();
    Capture cap = MakeCapture();
    Player p = MakePlayer();
    const int money0 = p.GetMoney();                    // 建構子預設 100
    REQUIRE(money0 >= nccu::kCh1UglyUmbrellaPrice);      // 一開始就買得起
    const int karma0 = p.GetKarma();

    // 章節／對象／選項標籤不符 -> 全是無操作（不扣錢、不給傘）。
    CHECK_FALSE(nccu::TryBuyAuntieUglyUmbrella(EventBus::Instance(),
        p, "shop_auntie", "購買醜綠傘", SemesterState::Chapter4_Finals));
    CHECK_FALSE(nccu::TryBuyAuntieUglyUmbrella(EventBus::Instance(),
        p, "victim", "購買醜綠傘", SemesterState::Chapter1_AddDrop));
    CHECK_FALSE(nccu::TryBuyAuntieUglyUmbrella(EventBus::Instance(),
        p, "shop_auntie", "詢問雨傘", SemesterState::Chapter1_AddDrop));
    CHECK(p.GetMoney() == money0);
    CHECK(p.HeldUmbrellaKind() == HeldUmbrella::None);

    // 真正的購買：扣 80 元、醜傘到手、顯示提示。
    CHECK(nccu::TryBuyAuntieUglyUmbrella(EventBus::Instance(),
        p, "shop_auntie", "購買醜綠傘", SemesterState::Chapter1_AddDrop));
    CHECK(p.GetMoney() == money0 - nccu::kCh1UglyUmbrellaPrice);   // 100 -> 20
    CHECK(p.HeldUmbrellaKind() == HeldUmbrella::Ugly);
    CHECK(p.HasUmbrella());                              // 自動擋雨已啟動
    CHECK(p.GetKarma() == karma0);                       // karma +0
    CHECK_FALSE(p.HasFlag(nccu::kFlagBoughtUglyUmbrella));   // 不是 C 的鎖
    CHECK_FALSE(cap.lastMessage.empty());                // 花費/餘額 提示
    CHECK(cap.lastMessage.find("80") != std::string::npos);   // 花費
    CHECK(cap.lastMessage.find("20") != std::string::npos);   // 餘額

    // 冪等：再次對話會再選到選單，但既已持有醜傘就不得再扣一次 80。
    CHECK_FALSE(nccu::TryBuyAuntieUglyUmbrella(EventBus::Instance(),
        p, "shop_auntie", "購買醜綠傘", SemesterState::Chapter1_AddDrop));
    CHECK(p.GetMoney() == money0 - nccu::kCh1UglyUmbrellaPrice);   // 仍是 20
    EventBus::Instance().Clear();
}

// 醜傘購買有資金守門：錢不夠時不扣錢、不給傘，只顯示「你錢不夠」提示。
TEST_CASE("B3: Ch1 阿姨 醜傘 buy is fund-guarded (poor → no charge, no umbrella)") {
    EventBus::Instance().Clear();
    Capture cap = MakeCapture();
    Player p = MakePlayer();
    // 把錢包扣到低於售價（DeductMoney 回傳 false → 不成交）。
    REQUIRE(p.DeductMoney(p.GetMoney() - 10));           // 留 10 < 80
    REQUIRE(p.GetMoney() < nccu::kCh1UglyUmbrellaPrice);

    CHECK_FALSE(nccu::TryBuyAuntieUglyUmbrella(EventBus::Instance(),
        p, "shop_auntie", "購買醜綠傘", SemesterState::Chapter1_AddDrop));
    CHECK(p.GetMoney() == 10);                           // 錢包未動
    CHECK(p.HeldUmbrellaKind() == HeldUmbrella::None);   // 未授予雨傘
    CHECK(cap.lastMessage == "你錢不夠");                 // 錢不夠的提示
    EventBus::Instance().Clear();
}
