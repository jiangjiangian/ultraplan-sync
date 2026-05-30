#include "doctest/doctest.h"
#include "game/entities/NPC.h"
#include "engine/events/EventBus.h"

#include <string>
#include <vector>

/**
 * @file test_npc.cpp
 * @brief 驗證 NPC 對話基本行為：初始行索引與行數、Interact 發出當前台詞並推進索引、
 *        末行後回繞、SetDialogLines 重設、以及無台詞時的安全空操作。
 */

// NPC 預設從第 0 行開始，且行數正確。
TEST_CASE("NPC 預設值：從第 0 行開始，且行數正確") {
    NPC n(nccu::engine::math::Vec2{100, 100},
          std::vector<std::string>{"line 0", "line 1", "line 2"});
    CHECK(n.CurrentLineIndex() == 0);
    CHECK(n.DialogLineCount() == 3);
    CHECK(n.CurrentLineText() == "line 0");
    CHECK_FALSE(n.IsQuestGiver());
}

// Interact 會發出目前台詞，接著把行索引往後推進。
TEST_CASE("NPC：Interact 發出當前台詞，接著推進行索引") {
    EventBus::Instance().Clear();
    int hits = 0;
    std::string captured;
    EventBus::Instance().Subscribe(EventType::ShowMessage,
        [&](const Event& e) { hits++; captured = e.text; });

    NPC n(nccu::engine::math::Vec2{0, 0},
          std::vector<std::string>{"hello", "world", "again"});
    n.Interact(nullptr);
    CHECK(hits == 1);
    CHECK(captured == "hello");
    CHECK(n.CurrentLineIndex() == 1);

    n.Interact(nullptr);
    CHECK(hits == 2);
    CHECK(captured == "world");
    CHECK(n.CurrentLineIndex() == 2);
}

// 講完最後一行後，索引回繞到開頭。
TEST_CASE("NPC：講完最後一行後 Interact 回繞到開頭") {
    EventBus::Instance().Clear();
    NPC n(nccu::engine::math::Vec2{0, 0}, std::vector<std::string>{"a", "b"});
    n.Interact(nullptr); // 發出 "a"，索引 → 1
    n.Interact(nullptr); // 發出 "b"，索引 → 0
    CHECK(n.CurrentLineIndex() == 0);
}

// SetDialogLines 會替換對話內容並重設索引。
TEST_CASE("NPC：SetDialogLines 替換對話內容並重設索引") {
    EventBus::Instance().Clear();
    NPC n(nccu::engine::math::Vec2{0, 0},
          std::vector<std::string>{"a", "b", "c"});
    n.Interact(nullptr); // 索引 → 1
    CHECK(n.CurrentLineIndex() == 1);

    n.SetDialogLines(std::vector<std::string>{"new1", "new2"});
    CHECK(n.CurrentLineIndex() == 0);
    CHECK(n.DialogLineCount() == 2);
    CHECK(n.CurrentLineText() == "new1");
}

// 沒有任何台詞時，Interact 是安全的空操作。
TEST_CASE("NPC：無任何台詞時 Interact 是安全的空操作") {
    EventBus::Instance().Clear();
    int hits = 0;
    EventBus::Instance().Subscribe(EventType::ShowMessage,
        [&](const Event&) { hits++; });

    NPC n(nccu::engine::math::Vec2{0, 0}, std::vector<std::string>{});
    n.Interact(nullptr);
    CHECK(hits == 0);
    CHECK(n.CurrentLineIndex() == 0);
}
