/**
 * @file test_chapter_transitions.cpp
 * @brief 驗證每次章節/幕間/結局轉場發出的提示字串、HUD 上下兩槽分流，以及名冊在轉場同幀更新、不殘留前一章。
 */
#include "doctest/doctest.h"
#include "game/quest/Flags.h"
#include "game/quest/ChapterGate.h"
#include "game/quest/ChapterSpawns.h"
#include "game/state/ChapterToast.h"
#include "game/dialog/DialogState.h"
#include "game/state/EndingGate.h"
#include "engine/events/EventBus.h"
#include "game/controller/EventWiring.h"
#include "engine/core/GameObject.h"
#include "engine/events/HudSlot.h"
#include "game/entities/Player.h"
#include "game/controller/SceneRouter.h"
#include "game/state/SemesterStateMachine.h"
#include "game/entities/TrueUmbrella.h"
#include "game/world/World.h"
#include "engine/math/Vec2.h"
#include <algorithm>
#include <string>
#include <vector>

using nccu::SemesterState;
using nccu::SemesterStateMachine;
using nccu::World;
using nccu::engine::math::Vec2;

// 每次章節／幕間／結局的轉場都會發布一則 ShowMessage，讓玩家看到狀態機在前進。
// 在此之前，每次 Transition（Ch1 → 市 → Ch2 → … → 結局）都沒有事件，HUD 上
// 唯一看得到的文字是無關的殘留雨傘提示。以下測試釘住每個轉場點實際發出的字串，
// 以便日後若重構又讓狀態機「靜音」時能被抓到。

namespace {

// 最新一則 ShowMessage 的內容。以 ref 擷取一個自由的 std::string，可讓 lambda
// 在被移動時仍穩定（Subscription token 可移動，但擷取的是呼叫端堆疊上字串的
// 參考）。正式版中由 GameController 接上的訂閱者會把同樣的文字鏡射到
// World::HudMessage()；這裡直接用一個訂閱者，可避免建構 GameController
//（後者需要無頭測試提供不了的完整 Input/Renderer 堆疊）。
[[nodiscard]] EventBus::Subscription
SubscribeToLatest(std::string& latest) {
    return EventBus::Instance().ScopedSubscribe(
        EventType::ShowMessage,
        [&latest](const Event& e) { latest = e.text; });
}

} // namespace

// 轉場提示字串表涵蓋每一個狀態（各章、幕間、四個結局）。
TEST_CASE("ChapterTransitionToast 字串表涵蓋每一個狀態") {
    using nccu::ChapterTransitionToast;
    CHECK(ChapterTransitionToast(SemesterState::Chapter1_AddDrop)   == "✓ 進入第一章 加退選");
    CHECK(ChapterTransitionToast(SemesterState::Interlude_Market)   == "✓ 章節清關 — 進入幕間市集");
    CHECK(ChapterTransitionToast(SemesterState::Chapter2_Midterms)  == "✓ 進入第二章 期中考");
    CHECK(ChapterTransitionToast(SemesterState::Chapter3_SportsDay) == "✓ 進入第三章 運動會");
    CHECK(ChapterTransitionToast(SemesterState::Chapter4_Finals)    == "✓ 進入第四章 期末考");
    CHECK(ChapterTransitionToast(SemesterState::Ending_A) == "✓ 抵達結局");
    CHECK(ChapterTransitionToast(SemesterState::Ending_B) == "✓ 抵達結局");
    CHECK(ChapterTransitionToast(SemesterState::Ending_D) == "✓ 抵達結局");
    CHECK(ChapterTransitionToast(SemesterState::Ending_C) == "✓ 抵達結局");
}

// Ch1 經 UmbrellaClaimed 轉到幕間市集時，會發布清關提示。
TEST_CASE("EventWiring：Ch1 經 UmbrellaClaimed -> 幕間市集會發布提示") {
    EventBus::Instance().Clear();
    std::string last;
    auto sub = SubscribeToLatest(last);
    SemesterStateMachine m;
    std::string name;
    nccu::WireStateTransitionSubscribers(EventBus::Instance(), m, name);
    REQUIRE(m.Current() == SemesterState::Chapter1_AddDrop);

    EventBus::Instance().Publish(
        Event{EventType::UmbrellaClaimed, "TrueUmbrella"});
    CHECK(m.Current() == SemesterState::Interlude_Market);
    CHECK(last == "✓ 章節清關 — 進入幕間市集");

    EventBus::Instance().Clear();
}

// Ch2 清關經閘門轉到幕間市集時，會發布清關提示。
TEST_CASE("ChapterGate：Ch2 -> 幕間市集會發布提示") {
    EventBus::Instance().Clear();
    std::string last;
    auto sub = SubscribeToLatest(last);
    SemesterStateMachine m;
    Player p{Vec2{0, 0}};
    nccu::DialogState d;

    m.Transition(SemesterState::Chapter2_Midterms);
    p.SetFlag(nccu::kFlagCh2Cleared);
    nccu::CheckChapterGates(EventBus::Instance(), p, m, d);

    CHECK(m.Current() == SemesterState::Interlude_Market);
    CHECK(last == "✓ 章節清關 — 進入幕間市集");

    EventBus::Instance().Clear();
}

// Ch3 清關（Flag_Ch3Cleared）經閘門轉到幕間市集時，會發布清關提示。
TEST_CASE("ChapterGate：Ch3 經 Flag_Ch3Cleared -> 幕間市集會發布提示") {
    EventBus::Instance().Clear();
    std::string last;
    auto sub = SubscribeToLatest(last);
    SemesterStateMachine m;
    Player p{Vec2{0, 0}};
    nccu::DialogState d;

    m.Transition(SemesterState::Chapter3_SportsDay);
    p.SetFlag(nccu::kFlagCh3Cleared);
    nccu::CheckChapterGates(EventBus::Instance(), p, m, d);

    CHECK(m.Current() == SemesterState::Interlude_Market);
    CHECK(last == "✓ 章節清關 — 進入幕間市集");

    EventBus::Instance().Clear();
}

// Ch3 經 TrueUmbrella 轉到幕間市集時，會發布清關提示。
TEST_CASE("EventWiring：Ch3 經 TrueUmbrella -> 幕間市集會發布提示") {
    EventBus::Instance().Clear();
    std::string last;
    auto sub = SubscribeToLatest(last);
    SemesterStateMachine m;
    std::string name;
    nccu::WireStateTransitionSubscribers(EventBus::Instance(), m, name);

    m.Transition(SemesterState::Chapter3_SportsDay);
    EventBus::Instance().Publish(
        Event{EventType::UmbrellaClaimed, "TrueUmbrella"});

    CHECK(m.Current() == SemesterState::Interlude_Market);
    CHECK(last == "✓ 章節清關 — 進入幕間市集");

    EventBus::Instance().Clear();
}

// 離開幕間市集時，會依 returnTo 發布對應目的章節的提示（Ch2/Ch3/Ch4）。
TEST_CASE("ChapterGate：幕間市集 -> returnTo 會發布目的章節提示") {
    // 注意：測試套件的 EventBus 隔離機制會在每個 subcase 邊界呼叫
    // EventBus::Clear()，因此訂閱必須建立在每個 SUBCASE 內——在 TEST_CASE
    // 範圍建立的 Subscribe 會在 SUBCASE 主體執行前就被清掉。

    SUBCASE("returnTo = Ch2（前往期中考）") {
        std::string last;
        auto sub = SubscribeToLatest(last);
        SemesterStateMachine m;
        Player p{Vec2{0, 0}};
        nccu::DialogState d;
        m.Transition(SemesterState::Interlude_Market);
        m.SetInterludeReturnTo(SemesterState::Chapter2_Midterms);
        p.SetFlag(nccu::kFlagLeaveInterlude);
        nccu::CheckChapterGates(EventBus::Instance(), p, m, d);
        CHECK(m.Current() == SemesterState::Chapter2_Midterms);
        CHECK(last == "✓ 進入第二章 期中考");
    }
    SUBCASE("returnTo = Ch3（前往運動會）") {
        std::string last;
        auto sub = SubscribeToLatest(last);
        SemesterStateMachine m;
        Player p{Vec2{0, 0}};
        nccu::DialogState d;
        m.Transition(SemesterState::Interlude_Market);
        m.SetInterludeReturnTo(SemesterState::Chapter3_SportsDay);
        p.SetFlag(nccu::kFlagLeaveInterlude);
        nccu::CheckChapterGates(EventBus::Instance(), p, m, d);
        CHECK(m.Current() == SemesterState::Chapter3_SportsDay);
        CHECK(last == "✓ 進入第三章 運動會");
    }
    SUBCASE("returnTo = Ch4（前往期末考）") {
        std::string last;
        auto sub = SubscribeToLatest(last);
        SemesterStateMachine m;
        Player p{Vec2{0, 0}};
        nccu::DialogState d;
        m.Transition(SemesterState::Interlude_Market);
        m.SetInterludeReturnTo(SemesterState::Chapter4_Finals);
        p.SetFlag(nccu::kFlagLeaveInterlude);
        nccu::CheckChapterGates(EventBus::Instance(), p, m, d);
        CHECK(m.Current() == SemesterState::Chapter4_Finals);
        CHECK(last == "✓ 進入第四章 期末考");
    }
}

// Ch4 經結局閘門轉到 Ending A/B/C 時，都會發布「抵達結局」提示。
TEST_CASE("EndingGate：Ch4 -> Ending A/B/C 會發布「抵達結局」提示") {
    SUBCASE("Ending A 路徑") {
        std::string last;
        auto sub = SubscribeToLatest(last);
        SemesterStateMachine m;
        nccu::DialogState d;
        Player p{Vec2{0, 0}};
        m.Transition(SemesterState::Chapter4_Finals);
        p.AddKarma(81 - p.GetKarma());            // > 80
        p.SetFlag(nccu::kFlagHasTrueUmbrella);
        p.SetFlag(nccu::kFlagConsoledTA);
        nccu::CheckEndingGates(EventBus::Instance(), p, m, d);
        CHECK(m.Current() == SemesterState::Ending_A);
        CHECK(last == "✓ 抵達結局");
    }
    SUBCASE("Ending B 路徑（詛咒傘）") {
        std::string last;
        auto sub = SubscribeToLatest(last);
        SemesterStateMachine m;
        nccu::DialogState d;
        Player p{Vec2{0, 0}};
        m.Transition(SemesterState::Chapter4_Finals);
        p.SetFlag(nccu::kFlagTookCursedUmbrella);
        nccu::CheckEndingGates(EventBus::Instance(), p, m, d);
        CHECK(m.Current() == SemesterState::Ending_B);
        CHECK(last == "✓ 抵達結局");
    }
    SUBCASE("Ending C 路徑（醜傘）") {
        std::string last;
        auto sub = SubscribeToLatest(last);
        SemesterStateMachine m;
        nccu::DialogState d;
        Player p{Vec2{0, 0}};
        m.Transition(SemesterState::Chapter4_Finals);
        p.SetFlag(nccu::kFlagBoughtUglyUmbrella);
        nccu::CheckEndingGates(EventBus::Instance(), p, m, d);
        CHECK(m.Current() == SemesterState::Ending_C);
        CHECK(last == "✓ 抵達結局");
    }
}

// 領取 TrueUmbrella 時，章節清關提示走 HUD 上方欄、雨傘撿取台詞走下方欄，兩欄同時存活、互不覆蓋、互不洩漏。
TEST_CASE("TrueUmbrella::BeClaimed：章節提示走上方欄，撿取台詞走下方欄") {
    // 背景：先前兩個發布都寫進同一個單槽的 HUD 頻道——雨傘自己的 ShowMessage，
    // 以及 UmbrellaClaimed 訂閱者發出的章節清關 ShowMessage。後發布的那個才是
    // 玩家看到的，導致清關訊息蓋掉雨傘撿取台詞（或反之）。現在的做法是：
    // 章節清關發布在 HudSlot::Top，雨傘撿取台詞留在 HudSlot::Bottom，兩欄同時
    // 存活、兩行都看得到。本案走過實際接線（TrueUmbrella → UmbrellaClaimed
    // 訂閱者 → 章節閘門 → HUD 訂閱者），並斷言每個槽各帶其預期文字——這是防止
    // 兩個頻道被重新合併為一的回歸網。
    EventBus::Instance().Clear();
    nccu::World w("", /*loadSprites=*/false);
    std::string name;
    nccu::WireStateTransitionSubscribers(EventBus::Instance(),
                                          w.Semester(), name);
    nccu::WireHudMessageSubscriber(EventBus::Instance(), w);

    // 前置條件：學期在 Chapter 1（事件接線的同層 if 只會從 Chapter1_AddDrop 觸發）。
    REQUIRE(w.Semester().Current() == SemesterState::Chapter1_AddDrop);

    TrueUmbrella umb{Vec2{0, 0}};
    Player player{Vec2{0, 0}};
    umb.BeClaimed(&player);

    // 雙頻道的後置條件：上方欄帶章節清關提示，下方欄帶雨傘撿取文字，兩者互不覆蓋——
    // 若回歸到兩槽合併，會表現為其中一個頻道少了一行。
    CHECK(w.HudMessage(nccu::HudSlot::Top) == "✓ 章節清關 — 進入幕間市集");
    CHECK(w.HudMessage(nccu::HudSlot::Bottom).find("TrueUmbrella")
          != std::string::npos);
    CHECK(w.Semester().Current() == SemesterState::Interlude_Market);
    // 跨頻道不洩漏：每個槽剛好只有一樣東西。
    CHECK(w.HudMessage(nccu::HudSlot::Top).find("TrueUmbrella")
          == std::string::npos);
    CHECK(w.HudMessage(nccu::HudSlot::Bottom).find("章節清關")
          == std::string::npos);

    EventBus::Instance().Clear();
}

// 端到端：接上 GameController 風格的接線後，轉場的 ShowMessage 會抵達 World 的上方 HUD 槽（View 讀取的位置）。
TEST_CASE("HudMessage 訂閱者端到端收到轉場提示") {
    // 綜合檢查：接上 GameController 風格的接線後，轉場發出的 ShowMessage 會抵達
    // World 的上方 HUD 槽——也就是 View 讀取的那一面。章節提示路由到 HudSlot::Top，
    // 以便在同一幀也有下方槽的 ShowMessage（抵達提示、karma、撿取）時仍能存活。
    EventBus::Instance().Clear();
    World w("", /*loadSprites=*/false);
    nccu::WireHudMessageSubscriber(EventBus::Instance(), w);
    SemesterStateMachine m;
    Player p{Vec2{0, 0}};
    nccu::DialogState d;

    m.Transition(SemesterState::Chapter2_Midterms);
    p.SetFlag(nccu::kFlagCh2Cleared);
    nccu::CheckChapterGates(EventBus::Instance(), p, m, d);

    CHECK(w.HudMessage(nccu::HudSlot::Top) == "✓ 章節清關 — 進入幕間市集");
    // 下方槽維持空白——此路徑沒有任何下方槽的發布者觸發。
    CHECK(w.HudMessage(nccu::HudSlot::Bottom).empty());

    EventBus::Instance().Clear();
}

// 每次章節／幕間／結局轉場，都會在狀態機移動的「同一幀」把名冊更新進可見的 npcs[]。
// 先前名冊抽換是在下一幀 Update() 的開頭執行，導致轉場當幀的狀態快照出現
// semester=新章節但 npcs[]=舊章節的落差（七個主線轉場都會發生）。改由
// SceneRouter::SettleRoster 在 Update 結尾呼叫，便關閉了這個落差窗口。本測試直接
// 用 SceneRouter 釘住此契約（不需要 View／Harness），日後若退回 1 幀（或 2 幀）落差便會在此失敗。
TEST_CASE("名冊在 Transition() 同一幀跟隨 FSM 更新") {
    nccu::World w("", /*loadSprites=*/false);
    nccu::SceneRouter r{w.Semester().Current()};

    // 計算 npcId 屬於某章名冊的 NPC，模擬狀態快照中的 npcs[] 欄位（只取非空 NpcId）。
    auto npcIds = [&w]() {
        std::vector<std::string> out;
        for (const auto& o : w.Objects()) {
            if (!o || !o->IsActive()) continue;
            const std::string id{o->NpcId()};
            if (!id.empty()) out.push_back(id);
        }
        return out;
    };

    // ----- 第 N-1 幀：狀態機在 Ch1，名冊是 Ch1（5 個原型）。-----
    auto ids = npcIds();
    CHECK(std::find(ids.begin(), ids.end(), "victim") != ids.end());

    // ----- 第 N 幀：轉場在幀中觸發；SettleRoster 在 Update 結尾執行，
    // 呼叫位置與 GameController 的 Update 結尾相同。-----
    w.Semester().Transition(SemesterState::Chapter2_Midterms);
    r.SettleRoster(w);

    // 在這一幀最終的 View::Draw／狀態快照中，名冊必須已是新章節的；
    // 修正前因設計失誤而是前一章節的，正是此問題。
    ids = npcIds();
    CHECK(std::find(ids.begin(), ids.end(), "librarian") != ids.end());
    // Ch1 與 Ch2 共用 victim/suit_senior/bookworm/ta/shop_auntie；
    // librarian 只在 Ch2 出現（章節專屬的標記）。

    // 每次轉場只呼叫一次的契約：對同一狀態機狀態再呼叫一次 SettleRoster 為無操作（以其自身游標冪等）。
    const std::size_t countAfter = ids.size();
    r.SettleRoster(w);
    ids = npcIds();
    CHECK(ids.size() == countAfter);
}

// 走完完整七段主線，每一步都呼叫 SettleRoster，確認每次轉場都在同一幀呈現目的狀態的名冊、不殘留前一章 NPC。
TEST_CASE("每次轉場都關閉其 npcs[] 落差（完整主幹）") {
    // 走完七段主線——Ch1 → 幕間 → Ch2 → 幕間 → Ch3 → 幕間 → Ch4 → Ending_A——
    // 每一步都依正式版 Update 結尾的方式呼叫 SettleRoster。每一步都必須在同一幀
    // 呈現目的狀態的名冊。
    nccu::World w("", /*loadSprites=*/false);
    nccu::SceneRouter r{w.Semester().Current()};

    const SemesterState path[] = {
        SemesterState::Interlude_Market,
        SemesterState::Chapter2_Midterms,
        SemesterState::Interlude_Market,
        SemesterState::Chapter3_SportsDay,
        SemesterState::Interlude_Market,
        SemesterState::Chapter4_Finals,
        SemesterState::Ending_A,
    };

    for (SemesterState s : path) {
        w.Semester().Transition(s);
        r.SettleRoster(w);

        // 每次轉場都在這一幀落定其目的名冊。
        const auto& expected = nccu::ChapterNpcSpawns(s);
        std::size_t hits = 0;
        for (const auto& o : w.Objects()) {
            if (!o || !o->IsActive()) continue;
            const std::string id{o->NpcId()};
            if (id.empty()) continue;
            for (const auto& sp : expected)
                if (sp.npcId == id) { ++hits; break; }
        }
        CHECK_MESSAGE(hits == expected.size(),
                      "transition to " << static_cast<int>(s)
                      << " left npcs[] missing roster entries");

        // 不殘留前一章的 NPC：每個觀察到的 NPC 都必須在新章節的生成表中。
        for (const auto& o : w.Objects()) {
            if (!o || !o->IsActive()) continue;
            if (o->IsVendor()) continue;   // 攤販來自 ChapterVendors（非 NPC 名冊）；其 npcId 不在此表
            const std::string id{o->NpcId()};
            if (id.empty()) continue;
            bool inExpected = false;
            for (const auto& sp : expected)
                if (sp.npcId == id) { inExpected = true; break; }
            CHECK_MESSAGE(inExpected,
                          "transition to " << static_cast<int>(s)
                          << " leaked stale NPC: " << id);
        }
    }
}
