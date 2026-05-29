/**
 * @file test_two_hud_channels.cpp
 * @brief 驗證雙 HUD 通道（Top / Bottom）：依 slot 路由不互相覆寫、同幀兩通道
 *        共存、章節→幕間轉場時兩行皆保留、Top 過期不外洩到 Bottom，以及
 *        DismissHud 的單通道與全通道語意；含向後相容的預設 slot 行為。
 */
//
// 在拆分前，World 只有單一 hudMessage_ 欄位；每次章節→幕間轉場的「章節清關」
// 提示只可見約 0.02 秒（1 幀），因為幕間抵達提示緊接著發佈並覆寫同一欄位。
// 把通道拆成兩個後，本檔固定以下不變量：
//
//   1. 發佈到 Top 只落在 Top，Bottom 維持空（反之亦然）。
//   2. 同一幀的 Top + Bottom 發佈彼此共存，互不覆寫。
//   3. 章節→幕間的轉場樣式（相鄰發佈：章節提示在 Top + 抵達提示在 Bottom）
//      會讓兩行都保留可見。
//   4. Top 過期不會把文字外洩到 Bottom。
//   5. DismissHud(slot) 只清除指定通道；無參數的 DismissHud() 兩者皆清。
//
// eventbus_isolation 監聽器會在 case 邊界清空匯流排，故每個測試都從乾淨開始。

#include "doctest/doctest.h"
#include "game/state/ChapterToast.h"
#include "engine/events/EventBus.h"
#include "game/controller/EventWiring.h"
#include "engine/events/HudSlot.h"
#include "ui/MessageView.h"   // kHudTtl
#include "game/entities/Player.h"
#include "game/state/SemesterState.h"
#include "game/state/SemesterStateMachine.h"
#include "game/world/World.h"
#include "engine/math/Vec2.h"

using nccu::HudSlot;
using nccu::SemesterState;
using nccu::World;
using nccu::engine::math::Vec2;

// World::SetHudMessage 依 slot 路由，且不會跨通道外洩。
TEST_CASE("World::SetHudMessage routes by slot, no cross-channel leak") {
    World w("", /*loadSprites=*/false);

    // 全新世界：兩通道皆空，皆未過期。
    REQUIRE(w.HudMessage(HudSlot::Top).empty());
    REQUIRE(w.HudMessage(HudSlot::Bottom).empty());
    REQUIRE_FALSE(w.HudExpired(HudSlot::Top));
    REQUIRE_FALSE(w.HudExpired(HudSlot::Bottom));

    SUBCASE("Top write does not touch Bottom") {
        w.SetHudMessage(HudSlot::Top, "✓ 章節清關 — 進入幕間市集");
        CHECK(w.HudMessage(HudSlot::Top) == "✓ 章節清關 — 進入幕間市集");
        CHECK(w.HudMessage(HudSlot::Bottom).empty());
        CHECK(w.HudAge(HudSlot::Top) == doctest::Approx(0.0f));
    }
    SUBCASE("Bottom write does not touch Top") {
        w.SetHudMessage(HudSlot::Bottom, "你撿到了 TrueUmbrella，雨停了。");
        CHECK(w.HudMessage(HudSlot::Bottom)
              == "你撿到了 TrueUmbrella，雨停了。");
        CHECK(w.HudMessage(HudSlot::Top).empty());
        CHECK(w.HudAge(HudSlot::Bottom) == doctest::Approx(0.0f));
    }
    SUBCASE("Backward-compat SetHudMessage(text) writes to Bottom") {
        // 拆分前的呼叫點（測試、臨時探針）仍如拆分前一樣落在 Bottom 通道。
        w.SetHudMessage("legacy");
        CHECK(w.HudMessage(HudSlot::Bottom) == "legacy");
        CHECK(w.HudMessage(HudSlot::Top).empty());
        // 預設的 HudMessage() / HudAge() 存取器也指向 Bottom —— 讀取面同樣保有
        // 向後相容的性質。
        CHECK(w.HudMessage() == "legacy");
        CHECK(w.HudAge() == doctest::Approx(0.0f));
    }
}

// 同一幀的 Top + Bottom 發佈彼此共存，不會互相覆寫。
TEST_CASE("Same-frame Top + Bottom publishes coexist (no clobber)") {
    // 關鍵契約：同一幀發佈的章節清關（Top）與抵達提示（Bottom）最終必須都可見。
    // 斷言的是 World.HudMessage(Top) 與 HudMessage(Bottom) 的最終狀態 ——
    // 拆分前第二次發佈會覆寫第一次。
    //
    // 註：eventbus_isolation 監聽器會在每個 SUBCASE 邊界清空匯流排，故 World 與
    // WireHudMessageSubscriber 必須放在每個 SUBCASE 內（與其他以參考捕捉探針的
    // 轉場／業力測試相同）。

    SUBCASE("Top then Bottom") {
        World w("", /*loadSprites=*/false);
        nccu::WireHudMessageSubscriber(EventBus::Instance(), w);

        EventBus::Instance().Publish(Event{
            EventType::ShowMessage, "✓ 章節清關 — 進入幕間市集",
            HudSlot::Top});
        EventBus::Instance().Publish(Event{
            EventType::ShowMessage, "市集中央。逛完後往南離開",
            HudSlot::Bottom});

        CHECK(w.HudMessage(HudSlot::Top) == "✓ 章節清關 — 進入幕間市集");
        CHECK(w.HudMessage(HudSlot::Bottom) == "市集中央。逛完後往南離開");
    }

    // 順序不影響結果。
    SUBCASE("Bottom then Top — order does not matter") {
        World w("", /*loadSprites=*/false);
        nccu::WireHudMessageSubscriber(EventBus::Instance(), w);

        EventBus::Instance().Publish(Event{
            EventType::ShowMessage, "市集中央。逛完後往南離開",
            HudSlot::Bottom});
        EventBus::Instance().Publish(Event{
            EventType::ShowMessage, "✓ 章節清關 — 進入幕間市集",
            HudSlot::Top});

        CHECK(w.HudMessage(HudSlot::Top) == "✓ 章節清關 — 進入幕間市集");
        CHECK(w.HudMessage(HudSlot::Bottom) == "市集中央。逛完後往南離開");
    }
}

// 章節→幕間轉場樣式：兩行都會存活到下一幀。
TEST_CASE("Ch->IL transition pattern: both lines survive into next frame") {
    // 完全模擬正式環境的順序（GameController 加上 EventWiring 的 UmbrellaClaimed
    // 處理器）：
    //
    //   1. 章節清關提示發佈（Top）           — 第 N 幀
    //   2. 名冊重生 -> 幕間抵達提示發佈（Bottom） — 第 N 幀（或 N+1）
    //   3. TickHud 把兩者的存活時間各推進一個 60fps 的步進。
    //
    // 步驟 3 後玩家仍看見兩行（兩者存活時間都遠低於 kHudTtl）。拆分前步驟 2 會
    // 抹掉步驟 1 的文字 —— 這正是整個通道拆分要消滅的缺陷。
    EventBus::Instance().Clear();
    World w("", /*loadSprites=*/false);
    nccu::WireHudMessageSubscriber(EventBus::Instance(), w);

    nccu::PublishChapterTransitionToast(EventBus::Instance(), SemesterState::Interlude_Market);
    EventBus::Instance().Publish(Event{
        EventType::ShowMessage, nccu::kInterludeArrivalHint,
        HudSlot::Bottom});
    w.TickHud(1.0f / 60.0f);

    CHECK(w.HudMessage(HudSlot::Top) == "✓ 章節清關 — 進入幕間市集");
    CHECK(w.HudMessage(HudSlot::Bottom) == nccu::kInterludeArrivalHint);
    CHECK_FALSE(w.HudExpired(HudSlot::Top));
    CHECK_FALSE(w.HudExpired(HudSlot::Bottom));
    // 兩者存活時間以相同 dt 推進 —— 皆未過時。
    CHECK(w.HudAge(HudSlot::Top) == doctest::Approx(1.0f / 60.0f));
    CHECK(w.HudAge(HudSlot::Bottom) == doctest::Approx(1.0f / 60.0f));

    EventBus::Instance().Clear();
}

// Top 通道過期不會把文字外洩到 Bottom。
TEST_CASE("Top channel expiry never leaks text into Bottom") {
    // Top 提示存活時間超過 kHudTtl 後，往 Bottom 的發佈絕不可繼承到過期文字 ——
    // 兩通道真正獨立。
    World w("", /*loadSprites=*/false);

    w.SetHudMessage(HudSlot::Top, "expiring");
    // 讓存活時間超過 TTL。
    w.TickHud(nccu::kHudTtl + 0.5f);
    REQUIRE(w.HudExpired(HudSlot::Top));
    REQUIRE_FALSE(w.HudExpired(HudSlot::Bottom));  // 空，不算過期

    // 往 Bottom 的寫入乾淨落定。
    w.SetHudMessage(HudSlot::Bottom, "fresh");
    CHECK(w.HudMessage(HudSlot::Bottom) == "fresh");
    CHECK(w.HudAge(HudSlot::Bottom) == doctest::Approx(0.0f));
    // Top 仍是「已過期但保留」（View 的淡出契約會保留緩衝 —— HudExpired 標記它供
    // 工具抑制，不自動清除）。
    CHECK(w.HudMessage(HudSlot::Top) == "expiring");
    CHECK(w.HudExpired(HudSlot::Top));
}

// DismissHud 的單通道與全通道語意。
TEST_CASE("DismissHud per-slot vs default both-slot semantics") {
    World w("", /*loadSprites=*/false);
    w.SetHudMessage(HudSlot::Top,    "top");
    w.SetHudMessage(HudSlot::Bottom, "bottom");
    REQUIRE_FALSE(w.HudExpired(HudSlot::Top));
    REQUIRE_FALSE(w.HudExpired(HudSlot::Bottom));

    SUBCASE("DismissHud(Top) only kills Top") {
        w.DismissHud(HudSlot::Top);
        CHECK(w.HudExpired(HudSlot::Top));
        CHECK_FALSE(w.HudExpired(HudSlot::Bottom));
    }
    SUBCASE("DismissHud(Bottom) only kills Bottom") {
        w.DismissHud(HudSlot::Bottom);
        CHECK_FALSE(w.HudExpired(HudSlot::Top));
        CHECK(w.HudExpired(HudSlot::Bottom));
    }
    SUBCASE("DismissHud() kills both slots") {
        // 不論有幾個通道在運作，略過提示的輸入都仍能以一次按鍵完成。
        w.DismissHud();
        CHECK(w.HudExpired(HudSlot::Top));
        CHECK(w.HudExpired(HudSlot::Bottom));
    }
}

// 未指定 slot 的 Event 落在 Bottom（向後相容）。
TEST_CASE("Default-slot Event lands on Bottom (backward compat)") {
    // 建立 Event{type, text} 而未指定 slot 的舊發佈者必須仍落在 Bottom ——
    // 每個既有呼叫點（教授陷阱傘、撿錢、NPC 對話、商人……）都屬此類。
    // 透過現存的 HUD 訂閱者接線驗證。
    EventBus::Instance().Clear();
    World w("", /*loadSprites=*/false);
    nccu::WireHudMessageSubscriber(EventBus::Instance(), w);

    EventBus::Instance().Publish(Event{EventType::ShowMessage, "legacy"});
    CHECK(w.HudMessage(HudSlot::Bottom) == "legacy");
    CHECK(w.HudMessage(HudSlot::Top).empty());

    EventBus::Instance().Clear();
}

// 章節提示的發佈在 Event 中帶有 HudSlot::Top。
TEST_CASE("Chapter toast publish carries HudSlot::Top in the Event") {
    // 擷取每一筆發佈的 ShowMessage 並檢查其 slot 欄位 —— 這是發佈端的契約：
    // ChapterToast.h 讓章節／結局轉場改用 Top 通道。
    EventBus::Instance().Clear();
    HudSlot lastSlot = HudSlot::Bottom;
    std::string lastText;
    auto sub = EventBus::Instance().ScopedSubscribe(
        EventType::ShowMessage,
        [&](const Event& e) { lastSlot = e.slot; lastText = e.text; });

    nccu::PublishChapterTransitionToast(EventBus::Instance(), SemesterState::Interlude_Market);
    CHECK(lastSlot == HudSlot::Top);
    CHECK(lastText == "✓ 章節清關 — 進入幕間市集");

    nccu::PublishChapterTransitionToast(EventBus::Instance(), SemesterState::Ending_A);
    CHECK(lastSlot == HudSlot::Top);
    CHECK(lastText == "✓ 抵達結局");

    EventBus::Instance().Clear();
}
