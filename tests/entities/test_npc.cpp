#include "doctest/doctest.h"
#include "entities/NPC.h"
#include "controller/EventBus.h"

#include <string>
#include <vector>

TEST_CASE("NPC defaults: starts at line 0, has the expected line count") {
    NPC n(nccu::gfx::Vec2{100, 100},
          std::vector<std::string>{"line 0", "line 1", "line 2"});
    CHECK(n.CurrentLineIndex() == 0);
    CHECK(n.DialogLineCount() == 3);
    CHECK(n.CurrentLineText() == "line 0");
    CHECK_FALSE(n.IsQuestGiver());
}

TEST_CASE("NPC: Interact publishes the current line then advances index") {
    EventBus::Instance().Clear();
    int hits = 0;
    std::string captured;
    EventBus::Instance().Subscribe(EventType::ShowMessage,
        [&](const Event& e) { hits++; captured = e.text; });

    NPC n(nccu::gfx::Vec2{0, 0},
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

TEST_CASE("NPC: Interact wraps after the last line") {
    EventBus::Instance().Clear();
    NPC n(nccu::gfx::Vec2{0, 0}, std::vector<std::string>{"a", "b"});
    n.Interact(nullptr); // publishes "a", index -> 1
    n.Interact(nullptr); // publishes "b", index -> 0
    CHECK(n.CurrentLineIndex() == 0);
}

TEST_CASE("NPC: SetDialogLines replaces dialog and resets index") {
    EventBus::Instance().Clear();
    NPC n(nccu::gfx::Vec2{0, 0},
          std::vector<std::string>{"a", "b", "c"});
    n.Interact(nullptr); // index -> 1
    CHECK(n.CurrentLineIndex() == 1);

    n.SetDialogLines(std::vector<std::string>{"new1", "new2"});
    CHECK(n.CurrentLineIndex() == 0);
    CHECK(n.DialogLineCount() == 2);
    CHECK(n.CurrentLineText() == "new1");
}

TEST_CASE("NPC: Interact with no dialog lines is a safe no-op") {
    EventBus::Instance().Clear();
    int hits = 0;
    EventBus::Instance().Subscribe(EventType::ShowMessage,
        [&](const Event&) { hits++; });

    NPC n(nccu::gfx::Vec2{0, 0}, std::vector<std::string>{});
    n.Interact(nullptr);
    CHECK(hits == 0);
    CHECK(n.CurrentLineIndex() == 0);
}
