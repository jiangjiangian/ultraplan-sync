// Test isolation for the global EventBus singleton.
//
// EventBus has no per-handler unsubscribe — only a global Clear(). Many
// test cases Subscribe a lambda that captures stack/`this` and rely on
// the *next* test's Clear() to remove it. A test that Publish()es before
// its own Clear() (e.g. the CashPickup factory cases, Player::ApplyRain
// respawn) then invokes a handler over a destroyed capture — observed as
// SIGSEGV / "double free detected" and aborting the whole run.
//
// Production is unaffected: only GameController subscribes there, and its
// dtor Clears. This listener makes the *suite* hermetic by clearing the
// bus at every test/subcase boundary, so a handler can never outlive the
// scope that registered it. (A proper RAII unsubscribe on EventBus is a
// separate engine-hardening item — see .claude/BUGLEDGER.md.)

#include "doctest/doctest.h"
#include "controller/EventBus.h"

namespace {

struct EventBusIsolation : doctest::IReporter {
    explicit EventBusIsolation(const doctest::ContextOptions&) {}

    static void Reset() { EventBus::Instance().Clear(); }

    void test_case_start(const doctest::TestCaseData&) override   { Reset(); }
    void test_case_reenter(const doctest::TestCaseData&) override { Reset(); }
    void test_case_end(const doctest::CurrentTestCaseStats&) override { Reset(); }
    void subcase_start(const doctest::SubcaseSignature&) override  { Reset(); }
    void subcase_end() override                                   { Reset(); }

    // Unused reporting hooks — no-ops.
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
