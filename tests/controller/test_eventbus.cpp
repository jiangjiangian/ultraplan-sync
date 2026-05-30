#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "doctest/doctest.h"
#include "engine/events/EventBus.h"

/**
 * @file test_eventbus.cpp
 * @brief 驗證全域 EventBus：事件正確派送給訂閱者、不會送錯型別，且在派送途中
 *        新增訂閱／Clear 也不會破壞正在進行的迭代（重入安全）。
 *
 * 本檔同時定義 doctest 的 main（DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN）。
 */

// 事件應派送給對應型別的訂閱者。
TEST_CASE("EventBus 將事件派送給訂閱者") {
    EventBus::Instance().Clear();
    int hits = 0;
    std::string captured;
    EventBus::Instance().Subscribe(EventType::ShowMessage,
        [&](const Event& e) { hits++; captured = e.text; });

    EventBus::Instance().Publish(Event{ EventType::ShowMessage, "hello" });

    CHECK(hits == 1);
    CHECK(captured == "hello");
}

// 不應派送給型別不符的訂閱者。
TEST_CASE("EventBus 不會派送給型別不符的訂閱者") {
    EventBus::Instance().Clear();
    int hits = 0;
    EventBus::Instance().Subscribe(EventType::KarmaChanged,
        [&](const Event&) { hits++; });
    EventBus::Instance().Publish(Event{ EventType::ShowMessage, "" });
    CHECK(hits == 0);
}

// 重入安全：handler 在 Publish 期間變動同一型別的訂閱清單，不得破壞正在進行的
// 迭代。修正方式是在派送前先對 handler 清單做快照。
TEST_CASE("EventBus 容許在 handler 內呼叫 Subscribe 與 Clear") {
    EventBus::Instance().Clear();
    int outer_hits = 0;
    int reentrant_hits = 0;

    EventBus::Instance().Subscribe(EventType::ShowMessage,
        [&](const Event&) {
            outer_hits++;
            // 在派送途中變動同一型別的 handler 清單
            EventBus::Instance().Subscribe(EventType::ShowMessage,
                [&](const Event&) { reentrant_hits++; });
            EventBus::Instance().Clear();
        });

    EventBus::Instance().Publish(Event{ EventType::ShowMessage, "" });

    CHECK(outer_hits == 1);          // 外層 handler 恰好執行一次
    CHECK(reentrant_hits == 0);      // 本次派送不得執行新加入的 handler
}
