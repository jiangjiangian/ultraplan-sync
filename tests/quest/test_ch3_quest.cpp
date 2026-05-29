/**
 * @file test_ch3_quest.cpp
 * @brief 驗證 Ch3 物物交換鏈：依序推進、背包行隨之更換、操場跑圈閘門、指示燈逐一點亮，以及清關轉場。
 */
#include "doctest/doctest.h"
#include "game/quest/Flags.h"
#include "game/quest/Chapter3Quest.h"
#include "game/quest/ItemCatalog.h"
#include "game/dialog/DialogOpener.h"
#include "engine/events/EventBus.h"
#include "game/controller/EventWiring.h"
#include "game/entities/Player.h"
#include "game/state/SemesterStateMachine.h"
#include "game/world/World.h"
#include "engine/math/Vec2.h"

#include <cmath>
#include <string>

using nccu::SemesterState;

namespace {
Player MakePlayer() { return Player{nccu::engine::math::Vec2{0.0f, 0.0f}}; }
constexpr auto kCh3 = SemesterState::Chapter3_SportsDay;
}  // namespace

// 物物交換鏈必須依序推進，每次對話只前進一環；亂序、章節不符、未跑操場圈都應為無操作，且各環不重複給獎。
TEST_CASE("TryAdvanceCh3Trade: 物物交換鏈 advances one link per talk, in order") {
    EventBus::Instance().Clear();
    Player p = MakePlayer();
    const int k0 = p.GetKarma();

    // 亂序：先找 B / C 都是無操作（還沒有香腸）。
    nccu::TryAdvanceCh3Trade(EventBus::Instance(), p, "loudspeaker_b", kCh3);
    nccu::TryAdvanceCh3Trade(EventBus::Instance(), p, "senior_c", kCh3);
    CHECK_FALSE(p.HasFlag(nccu::kFlagHasLoudspeaker));
    CHECK_FALSE(p.HasFlag(nccu::kFlagKnowsUmbrellaLoc));
    CHECK(p.GetKarma() == k0);

    // 章節不符永遠不會推進。
    nccu::TryAdvanceCh3Trade(EventBus::Instance(), p, "vendor_sausage_a",
                             SemesterState::Chapter1_AddDrop);
    CHECK_FALSE(p.HasFlag(nccu::kFlagHasSausage));

    // 校慶跑圈閘門：玩家跑完操場一圈前，A 不會交易。
    nccu::TryAdvanceCh3Trade(EventBus::Instance(), p, "vendor_sausage_a", kCh3);
    CHECK_FALSE(p.HasFlag(nccu::kFlagHasSausage));   // 還沒跑圈 -> 無操作
    p.SetFlag(nccu::kFlagSportsLapDone);

    // 第 1 環：A -> 香腸，+3。重複對話具冪等性。
    nccu::TryAdvanceCh3Trade(EventBus::Instance(), p, "vendor_sausage_a", kCh3);
    CHECK(p.HasFlag(nccu::kFlagHasSausage));
    CHECK(p.GetKarma() == k0 + 3);
    nccu::TryAdvanceCh3Trade(EventBus::Instance(), p, "vendor_sausage_a", kCh3);
    CHECK(p.GetKarma() == k0 + 3);                  // 不會加倍

    // 第 2 環：持香腸找 B -> 消耗香腸、取得大聲公，+3。
    nccu::TryAdvanceCh3Trade(EventBus::Instance(), p, "loudspeaker_b", kCh3);
    CHECK_FALSE(p.HasFlag(nccu::kFlagHasSausage));   // 已消耗
    CHECK(p.HasFlag(nccu::kFlagHasLoudspeaker));
    CHECK(p.GetKarma() == k0 + 6);
    nccu::TryAdvanceCh3Trade(EventBus::Instance(), p, "loudspeaker_b", kCh3);
    CHECK(p.GetKarma() == k0 + 6);

    // 鏈已往前後，A 不得再次給香腸。
    nccu::TryAdvanceCh3Trade(EventBus::Instance(), p, "vendor_sausage_a", kCh3);
    CHECK_FALSE(p.HasFlag(nccu::kFlagHasSausage));
    CHECK(p.GetKarma() == k0 + 6);

    // 第 3 環：持大聲公找 C -> 消耗、得知雨傘位置，+5。
    nccu::TryAdvanceCh3Trade(EventBus::Instance(), p, "senior_c", kCh3);
    CHECK_FALSE(p.HasFlag(nccu::kFlagHasLoudspeaker));   // 已消耗
    CHECK(p.HasFlag(nccu::kFlagKnowsUmbrellaLoc));
    CHECK(p.GetKarma() == k0 + 11);
    nccu::TryAdvanceCh3Trade(EventBus::Instance(), p, "senior_c", kCh3);
    CHECK(p.GetKarma() == k0 + 11);                 // 整條鏈已用盡
    EventBus::Instance().Clear();
}

// 交換鏈在背包中必須可見，且每次交易要乾淨地換行：被交易出去的物品立刻消失（無殘留行），
// 下一件出現，而「情報」（知識）永遠不會是一行。
TEST_CASE("B2.4: the Ch3 trade chain swaps bag rows cleanly (sausage -> 大聲公 -> none)") {
    EventBus::Instance().Clear();
    Player p = MakePlayer();
    auto has = [](const std::vector<nccu::InventoryRow>& rows, const char* id) {
        for (const auto& r : rows) if (r.itemId == id) return true;
        return false;
    };

    // 鏈開始前：兩件可攜物品都不在背包。
    {
        const auto rows = nccu::BuildInventoryRows(p);
        CHECK_FALSE(has(rows, nccu::kItemSausage));
        CHECK_FALSE(has(rows, nccu::kItemLoudspeaker));
    }

    // 第 1 環：拿到香腸 -> 恰好出現香腸行，沒有大聲公行。
    p.SetFlag(nccu::kFlagSportsLapDone);
    nccu::TryAdvanceCh3Trade(EventBus::Instance(), p, "vendor_sausage_a", kCh3);
    {
        const auto rows = nccu::BuildInventoryRows(p);
        CHECK(has(rows, nccu::kItemSausage));
        CHECK_FALSE(has(rows, nccu::kItemLoudspeaker));
    }

    // 第 2 環：以香腸換大聲公 -> 香腸行消失（已消耗），大聲公行出現；
    // 轉移後不留殘餘行。
    nccu::TryAdvanceCh3Trade(EventBus::Instance(), p, "loudspeaker_b", kCh3);
    {
        const auto rows = nccu::BuildInventoryRows(p);
        CHECK_FALSE(has(rows, nccu::kItemSausage));    // 沒有殘留的可攜物品
        CHECK(has(rows, nccu::kItemLoudspeaker));
    }

    // 第 3 環：以大聲公換情報 -> 大聲公行消失；情報屬知識，因此背包此時兩件物品都沒有。
    nccu::TryAdvanceCh3Trade(EventBus::Instance(), p, "senior_c", kCh3);
    {
        const auto rows = nccu::BuildInventoryRows(p);
        CHECK_FALSE(has(rows, nccu::kItemSausage));
        CHECK_FALSE(has(rows, nccu::kItemLoudspeaker));
        CHECK(p.HasFlag(nccu::kFlagKnowsUmbrellaLoc));  // 取得知識
    }
    EventBus::Instance().Clear();
}

// 跑圈前找 A 時，應在劇情中教玩家「先去操場跑一圈」：發出提示訊息且尚未給香腸。
TEST_CASE("Item 4a: talking A pre-lap surfaces the 操場 lap hint in-fiction") {
    // A 上的「!」（跑圈前即點亮）會把玩家引向 A；此時對話必須教導第 1 步——
    // 去跑一圈——而非默默不做事。TryAdvanceCh3Trade 會發出導向用的提示訊息，
    // 並在 Flag_SportsLapDone 之前都不給香腸。
    EventBus::Instance().Clear();
    std::string lastMsg;
    int msgs = 0;
    EventBus::Instance().Subscribe(           // 存活到下方的 Clear() 為止
        EventType::ShowMessage,
        [&](const Event& e) { lastMsg = e.text; ++msgs; });

    Player p = MakePlayer();
    nccu::TryAdvanceCh3Trade(EventBus::Instance(), p, "vendor_sausage_a", kCh3);   // 跑圈前的對話
    CHECK_FALSE(p.HasFlag(nccu::kFlagHasSausage));           // 還沒有香腸
    CHECK(msgs == 1);                                        // 發出了一則提示
    CHECK(lastMsg.find("操場") != std::string::npos);        // 指向操場跑圈
    EventBus::Instance().Clear();
}

// A→B→C 的「!」一次只點亮一環，依交換鏈進度移動，整條完成後全部熄滅。
TEST_CASE("Ch3IndicatorVisible: the A->B->C `!` reveals one link at a time") {
    Player p = MakePlayer();
    // A 從進入章節起（跑操場圈之前）就是可見的鏈頭——這修正了過去「第一步無線索」
    // 的問題（先前要到 Flag_SportsLapDone 才亮，玩家一進 Ch3 看不到任何「!」而迷路）。
    // 跑圈前走到 A 會觸發導向用的「先去操場跑一圈」提示；同一個「!」在跑完圈後交出香腸。
    CHECK(nccu::Ch3IndicatorVisible("vendor_sausage_a", p));   // 跑圈前已點亮
    p.SetFlag(nccu::kFlagSportsLapDone);
    // 跑完圈後：A 仍亮（鏈頭），B/C 兩環維持熄滅。
    CHECK(nccu::Ch3IndicatorVisible("vendor_sausage_a", p));
    CHECK_FALSE(nccu::Ch3IndicatorVisible("loudspeaker_b", p));
    CHECK_FALSE(nccu::Ch3IndicatorVisible("senior_c", p));
    // 非鏈上的 NPC 不受影響（只要是任務給予者就一直亮）。
    CHECK(nccu::Ch3IndicatorVisible("ta", p));

    // 在 A 交易後：只有 B 亮。
    p.SetFlag(nccu::kFlagHasSausage);
    CHECK_FALSE(nccu::Ch3IndicatorVisible("vendor_sausage_a", p));
    CHECK(nccu::Ch3IndicatorVisible("loudspeaker_b", p));
    CHECK_FALSE(nccu::Ch3IndicatorVisible("senior_c", p));

    // 在 B 交易後：只有 C 亮。
    p.ClearFlag(nccu::kFlagHasSausage);
    p.SetFlag(nccu::kFlagHasLoudspeaker);
    CHECK_FALSE(nccu::Ch3IndicatorVisible("loudspeaker_b", p));
    CHECK(nccu::Ch3IndicatorVisible("senior_c", p));

    // C 揭露雨傘位置後：整條鏈熄滅（任務完成）。
    p.ClearFlag(nccu::kFlagHasLoudspeaker);
    p.SetFlag(nccu::kFlagKnowsUmbrellaLoc);
    CHECK_FALSE(nccu::Ch3IndicatorVisible("senior_c", p));
}

// 沿著操場跑完整一圈會設下 Flag_SportsLapDone；站在中央（離開跑道帶）則永遠不會觸發。
TEST_CASE("World::UpdateSportsLap: a full 操場 lap sets Flag_SportsLapDone") {
    nccu::World w("", /*loadSprites=*/false);
    w.Semester().Transition(kCh3);
    REQUIRE(w.Semester().Current() == kCh3);
    Player* p = w.GetPlayer();
    REQUIRE(p != nullptr);
    REQUIRE_FALSE(p->HasFlag(nccu::kFlagSportsLapDone));
    CHECK(w.SportsLapProgress() == doctest::Approx(0.0f));

    // 讓玩家繞跑道中心走完整一圈。
    const float cx = nccu::kSportsTrackCx, cy = nccu::kSportsTrackCy;
    const float r = nccu::kSportsTrackR;
    constexpr float kTwoPi = 6.2831853f;
    for (int i = 0; i <= 40; ++i) {
        const float a = static_cast<float>(i) * (kTwoPi / 40.0f);
        p->SetPosition(nccu::engine::math::Vec2{cx + r * std::cos(a),
                                       cy + r * std::sin(a)});
        w.UpdateSportsLap();
    }
    CHECK(p->HasFlag(nccu::kFlagSportsLapDone));
    CHECK(w.SportsLapProgress() == doctest::Approx(1.0f));

    // 站在中央（離開跑道帶）永遠不會累積跑圈進度。
    nccu::World w2("", /*loadSprites=*/false);
    w2.Semester().Transition(kCh3);
    Player* p2 = w2.GetPlayer();
    for (int i = 0; i < 40; ++i) {
        p2->SetPosition(nccu::engine::math::Vec2{cx, cy});   // 正中央
        w2.UpdateSportsLap();
    }
    CHECK_FALSE(p2->HasFlag(nccu::kFlagSportsLapDone));
}

// Ch3 鏈上各 NPC 的開場依各自的持有旗標由 (a) 切換到 (b)。
TEST_CASE("ResolveOpenerSubState: Ch3 chain NPCs route (a)->(b) on their flag") {
    Player p = MakePlayer();
    CHECK(nccu::ResolveOpenerSubState("vendor_sausage_a", kCh3, p) == 0);
    CHECK(nccu::ResolveOpenerSubState("loudspeaker_b", kCh3, p) == 0);
    CHECK(nccu::ResolveOpenerSubState("senior_c", kCh3, p) == 0);

    p.SetFlag(nccu::kFlagHasSausage);
    CHECK(nccu::ResolveOpenerSubState("vendor_sausage_a", kCh3, p) == 1);
    CHECK(nccu::ResolveOpenerSubState("loudspeaker_b", kCh3, p) == 0);

    p.ClearFlag(nccu::kFlagHasSausage);
    p.SetFlag(nccu::kFlagHasLoudspeaker);
    CHECK(nccu::ResolveOpenerSubState("vendor_sausage_a", kCh3, p) == 1);
    CHECK(nccu::ResolveOpenerSubState("loudspeaker_b", kCh3, p) == 1);
    CHECK(nccu::ResolveOpenerSubState("senior_c", kCh3, p) == 0);

    p.ClearFlag(nccu::kFlagHasLoudspeaker);
    p.SetFlag(nccu::kFlagKnowsUmbrellaLoc);
    CHECK(nccu::ResolveOpenerSubState("vendor_sausage_a", kCh3, p) == 1);
    CHECK(nccu::ResolveOpenerSubState("loudspeaker_b", kCh3, p) == 1);
    CHECK(nccu::ResolveOpenerSubState("senior_c", kCh3, p) == 1);
}

// Ch3 清關：領取道具箱中的 TrueUmbrella 應推進到幕間市集，且 returnTo 設為 Ch4。
TEST_CASE("Ch3 clear: claiming the 道具箱 TrueUmbrella -> Interlude returnTo Ch4") {
    EventBus::Instance().Clear();
    nccu::SemesterStateMachine m;
    std::string name;
    nccu::WireStateTransitionSubscribers(EventBus::Instance(), m, name);

    m.Transition(SemesterState::Chapter3_SportsDay);
    REQUIRE(m.Current() == SemesterState::Chapter3_SportsDay);

    // 與 Ch1 同構：BeClaimed 發布 UmbrellaClaimed；Ch3 的同層 if 導向第三座市集，
    // 並設定回到 Ch4。
    EventBus::Instance().Publish(
        Event{EventType::UmbrellaClaimed, "TrueUmbrella"});
    CHECK(m.Current() == SemesterState::Interlude_Market);
    CHECK(m.InterludeReturnTo() == SemesterState::Chapter4_Finals);
    EventBus::Instance().Clear();
}
