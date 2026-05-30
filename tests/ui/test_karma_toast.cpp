/**
 * @file test_karma_toast.cpp
 * @brief 驗證業力變動的事件管線：每次 AddKarma 都發佈帶正負號差值的
 *        KarmaChanged，WireKarmaToastSubscriber 將其轉成以「業力」開頭的
 *        ShowMessage 並反映到 HUD，且詛咒傘拾取不會重複發佈。
 */
//
// 此處固定三項保證：
//   1. 每次 AddKarma 都會發佈 KarmaChanged，文字為帶正負號的差值（"+5"、"-3"）。
//   2. WireKarmaToastSubscriber 消費者會把它轉成以「業力」開頭的 ShowMessage，
//      最終反映到 HUD 橫幅。
//   3. CursedUmbrella::BeClaimed（經由 decreaseKarma → AddKarma）不會重複發佈
//      KarmaChanged。
//
// eventbus_isolation 監聽器會在每個 case 之間清空匯流排，故即使全部都觸及全域
// EventBus 單例，每個測試仍從乾淨狀態開始。

#include "doctest/doctest.h"
#include "game/entities/CursedUmbrella.h"
#include "engine/events/EventBus.h"
#include "game/controller/EventWiring.h"
#include "game/entities/Player.h"
#include "game/world/World.h"
#include "engine/math/Vec2.h"

#include <string>
#include <vector>

using nccu::engine::math::Vec2;

namespace {

// 擷取測試本體期間所發佈的每一筆 KarmaChanged 內容。
struct KarmaCapture {
    std::vector<std::string> deltas;
    EventBus::Subscription   sub;
};

KarmaCapture CaptureKarma() {
    KarmaCapture cap;
    cap.sub = EventBus::Instance().ScopedSubscribe(
        EventType::KarmaChanged,
        [&cap](const Event& e) { cap.deltas.push_back(e.text); });
    return cap;
}

} // namespace

// 每次 AddKarma 都發佈帶正負號差值文字的 KarmaChanged。
TEST_CASE("AddKarma 發佈帶正負號差值文字的 KarmaChanged") {
    Player p{Vec2{0, 0}};
    auto cap = CaptureKarma();

    p.AddKarma(5);
    REQUIRE(cap.deltas.size() == 1);
    CHECK(cap.deltas[0] == "+5");

    p.AddKarma(-3);
    REQUIRE(cap.deltas.size() == 2);
    CHECK(cap.deltas[1] == "-3");

    // 格式為 %+d，故正差值一律帶明確的 '+' —— 業力訂閱者依此直接組出「業力 +5」
    // 或「業力 -3」，不需另設分支。
    CHECK(p.GetKarma() == 50 + 5 - 3);
}

// decreaseKarma 經由 AddKarma 轉發，只發佈一次（不重複）。
TEST_CASE("decreaseKarma 經由 AddKarma 轉發——只發佈一次") {
    Player p{Vec2{0, 0}};
    auto cap = CaptureKarma();

    p.decreaseKarma(10);
    // 一次呼叫只有一個事件：來自 AddKarma 的發佈，而非 decreaseKarma 另行重複。
    // 固定詛咒傘修正所依賴的「不重複發佈」不變量。
    REQUIRE(cap.deltas.size() == 1);
    CHECK(cap.deltas[0] == "-10");
}

// 詛咒傘的拾取本身不影響業力，故不發佈 KarmaChanged。
TEST_CASE("CursedUmbrella::BeClaimed 不發佈 KarmaChanged（拾取不影響業力）") {
    // 業力代價已從拾取移到每章的 ApplyCursedTaintDecay（SceneRouter 進第二／三／
    // 四章時），故拾取本身發佈零個 KarmaChanged —— 可見的「業力 -5」橫幅改在下一個
    // 章節邊界出現，也就是道德污點真正累積之處。
    Player p{Vec2{0, 0}};
    auto cap = CaptureKarma();

    CursedUmbrella umb{Vec2{0, 0}};
    umb.BeClaimed(&p);

    CHECK(cap.deltas.empty());                       // 拾取時不發佈 KarmaChanged
    CHECK(p.GetKarma() == 50);                       // 業力未受影響
    CHECK(p.GetCursedTaint() == 1);                  // 改為累加污點

    // 接著觸發每章衰減：發佈一次 KarmaChanged "-5"。
    p.ApplyCursedTaintDecay();
    REQUIRE(cap.deltas.size() == 1);
    CHECK(cap.deltas[0] == "-5");
    CHECK(p.GetKarma() == 45);
}

// WireKarmaToastSubscriber 把 KarmaChanged 轉成 HUD 提示。
TEST_CASE("WireKarmaToastSubscriber 把 KarmaChanged 轉成 HUD 提示") {
    // 端到端：AddKarma -> KarmaChanged -> WireKarmaToastSubscriber
    // -> ShowMessage -> WireHudMessageSubscriber -> World.HudMessage()。
    // 這是在單元測試行程內走過的完整正式接線（不需建構 GameController）。
    //
    // 註：EventBus 隔離報告器會在每個 subcase 邊界清空匯流排，故 Wire... 呼叫
    // 必須放在每個 SUBCASE 內 —— 在 TEST_CASE 層級的訂閱會在本體執行前就被清掉。

    SUBCASE("正差值 → 業力 +N") {
        nccu::World w{"", /*loadSprites=*/false};
        nccu::WireHudMessageSubscriber(EventBus::Instance(), w);
        nccu::WireKarmaToastSubscriber(EventBus::Instance());
        Player p{Vec2{0, 0}};

        p.AddKarma(5);
        // HUD 訊息必須同時含有「業力」前綴與帶正負號的差值。
        CHECK(w.HudMessage().find("業力") != std::string::npos);
        CHECK(w.HudMessage().find("+5") != std::string::npos);
    }

    SUBCASE("負差值 → 業力 -N") {
        nccu::World w{"", /*loadSprites=*/false};
        nccu::WireHudMessageSubscriber(EventBus::Instance(), w);
        nccu::WireKarmaToastSubscriber(EventBus::Instance());
        Player p{Vec2{0, 0}};

        p.AddKarma(-3);
        CHECK(w.HudMessage().find("業力") != std::string::npos);
        // 實際呈現會原樣帶上正負號，玩家能毫無歧義地讀到「業力 -3」。
        CHECK(w.HudMessage().find("-3") != std::string::npos);
    }

    SUBCASE("AddKarma(0) 不發出 HUD 提示") {
        nccu::World w{"", /*loadSprites=*/false};
        nccu::WireHudMessageSubscriber(EventBus::Instance(), w);
        nccu::WireKarmaToastSubscriber(EventBus::Instance());
        Player p{Vec2{0, 0}};

        // 0 差值的呼叫偶爾發生（防禦性呼叫點）。訂閱者會濾掉 "+0" / "-0"，因此
        // 不會用無意義的讀數佔用 HUD 橫幅。World 起始時 HUD 為空。
        REQUIRE(w.HudMessage().empty());
        p.AddKarma(0);
        CHECK(w.HudMessage().empty());
    }
}
