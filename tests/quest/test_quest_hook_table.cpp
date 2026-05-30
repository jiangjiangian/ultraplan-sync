/**
 * @file test_quest_hook_table.cpp
 * @brief 驗證 E 互動的任務 hook 已改為註冊表，且表的內容、順序與自我守門行為正確。
 */
#include "doctest/doctest.h"
#include "engine/events/EventBus.h"
#include "game/quest/QuestHookTable.h"
#include "game/entities/Player.h"
#include "engine/math/Vec2.h"

#include <string_view>
#include <vector>

// E 互動的任務 hook 改以註冊表 (QuestHookTable) 管理，取代原本散落在
// GameController::Update 內的多個 TryXxx 呼叫。以下案例釘住兩項契約：
//   1. 表的內容與順序必須與原始的內嵌呼叫序列完全一致（順序具關鍵性——
//      後面的 hook 可能讀到前面 hook 設下的旗標）。
//   2. RunInteractHooks 會走訪整張表，且每個 hook 都自我守門；因此對任何
//      不符合的 (npcId, state) 執行整張表都不應改動任何狀態。

using nccu::InteractQuestHooks;
using nccu::QuestHook;
using nccu::RunInteractHooks;
using nccu::SemesterState;

// 表的註冊順序必須逐一對應原始內嵌呼叫的先後（順序錯置會破壞跨 hook 的旗標依賴）。
TEST_CASE("InteractQuestHooks：註冊順序與原始內嵌呼叫序列一致") {
    const std::vector<QuestHook>& hooks = InteractQuestHooks();
    REQUIRE(hooks.size() == 10);
    CHECK(hooks[0].name == std::string_view("TryReturnVictimUmbrella"));
    CHECK(hooks[1].name == std::string_view("TryReturnTaForm"));
    CHECK(hooks[2].name == std::string_view("TryRescueBookworm"));
    CHECK(hooks[3].name == std::string_view("TryMeetLibrarian"));
    CHECK(hooks[4].name == std::string_view("TryLendLibrarianUmbrella"));
    CHECK(hooks[5].name == std::string_view("TryReturnLibrarianUmbrella"));
    CHECK(hooks[6].name == std::string_view("TryApplyCh2Ripple"));
    CHECK(hooks[7].name == std::string_view("TryAdvanceCh3Trade"));
    CHECK(hooks[8].name == std::string_view("TryApplyCh3Ripple"));
    CHECK(hooks[9].name == std::string_view("TryApplyCh4Ripple"));
}

// 每筆項目都必須帶有可呼叫的函式物件，避免表中出現空 hook。
TEST_CASE("InteractQuestHooks：每筆項目都帶有可呼叫的函式物件") {
    for (const QuestHook& h : InteractQuestHooks()) {
        CHECK(static_cast<bool>(h.fn));
    }
}

// 不符合的 (npcId, state) 不得改動任何狀態：驗證每個 hook 都確實自我守門。
TEST_CASE("RunInteractHooks：不符合的 (npcId, state) 不改動任何狀態") {
    // 在 Ch1 用一個未知的 npcId，沒有任何 hook 的守門條件會成立，
    // 整張表等同無操作——karma / money / 旗標都不應改變。
    Player p{nccu::engine::math::Vec2{0, 0}};
    const int    karma0 = p.GetKarma();
    const int    money0 = p.GetMoney();
    const bool   umb0   = p.HasUmbrella();
    RunInteractHooks(EventBus::Instance(), p, "no_such_npc", SemesterState::Chapter1_AddDrop,
                     SemesterState::Chapter2_Midterms);
    CHECK(p.GetKarma()    == karma0);
    CHECK(p.GetMoney()    == money0);
    CHECK(p.HasUmbrella() == umb0);
}

// 表是穩定的單例：重複取用都回傳同一份 vector，順序不會在每幀之間漂移。
TEST_CASE("RunInteractHooks：表是穩定的單例（同一份實例）") {
    // 首次使用時建立一次，之後的呼叫都回傳同一個 vector。
    CHECK(&InteractQuestHooks() == &InteractQuestHooks());
}
