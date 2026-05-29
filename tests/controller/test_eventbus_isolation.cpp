/**
 * @file test_eventbus_isolation.cpp
 * @brief 註冊一個 doctest reporter，在每個 test case／subcase 邊界都 Clear 全域
 *        EventBus，使整個測試套件對 EventBus 保持隔離（hermetic）。
 *
 * EventBus 沒有逐 handler 的取消訂閱，只有全域 Clear()。許多測試會 Subscribe 一個
 * 捕捉堆疊變數／`this` 的 lambda，並仰賴「下一個」測試的 Clear() 來移除它。若某個
 * 測試在自己 Clear() 之前就 Publish()，便可能對已銷毀的捕捉物呼叫 handler——表現為
 * 記憶體錯誤而中止整個執行。正式程式不受影響：只有 GameController 在其中訂閱，且其
 * 解構會 Clear。本 listener 在每個測試／subcase 邊界清空匯流排，使 handler 永遠不會
 * 比註冊它的作用域活得更久。
 */

#include "doctest/doctest.h"
#include "engine/events/EventBus.h"

namespace {

struct EventBusIsolation : doctest::IReporter {
    explicit EventBusIsolation(const doctest::ContextOptions&) {}

    static void Reset() { EventBus::Instance().Clear(); }

    void test_case_start(const doctest::TestCaseData&) override   { Reset(); }
    void test_case_reenter(const doctest::TestCaseData&) override { Reset(); }
    void test_case_end(const doctest::CurrentTestCaseStats&) override { Reset(); }
    void subcase_start(const doctest::SubcaseSignature&) override  { Reset(); }
    void subcase_end() override                                   { Reset(); }

    // 未使用的回報鉤子——皆為 no-op。
    void report_query(const doctest::QueryData&) override {}
    void test_run_start() override {}
    void test_run_end(const doctest::TestRunStats&) override {}
    void test_case_exception(const doctest::TestCaseException&) override {}
    void log_assert(const doctest::AssertData&) override {}
    void log_message(const doctest::MessageData&) override {}
    void test_case_skipped(const doctest::TestCaseData&) override {}
};

} // namespace

REGISTER_LISTENER("eventbus_isolation", 1, EventBusIsolation);
