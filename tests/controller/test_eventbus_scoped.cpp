/**
 * @file test_eventbus_scoped.cpp
 * @brief 驗證 EventBus 的 RAII 作用域取消訂閱（ScopedSubscribe / Subscription）。
 *
 * EventBus::ScopedSubscribe(...) 回傳一個可移動、不可複製的 Subscription，其解構會
 * 「精確」移除自己那個 handler。本檔證明：
 *   (a) Subscription 離開作用域即取消訂閱——之後的 Publish 不再呼叫它；
 *   (b) 當 handler 由作用域 Subscription 持有時，可避免懸空捕捉造成的 use-after-free
 *      （捕捉物與訂閱同生共死，匯流排絕不會對已銷毀的捕捉物呼叫 handler）；
 *   (c) 移動語意會轉移所有權，不會發生重複取消訂閱。
 *
 * eventbus_isolation listener 會在每個測試／subcase 邊界 Clear 匯流排，故以下每個
 * 案例都從乾淨的匯流排開始。
 */

#include "doctest/doctest.h"
#include "engine/events/EventBus.h"

#include <memory>
#include <string>

namespace {

Event Msg(std::string text) {
    return Event{EventType::ShowMessage, std::move(text)};
}

} // namespace

// Subscription 離開作用域後即取消訂閱，之後不再被派送。
TEST_CASE("Subscription out of scope unsubscribes (no later delivery)") {
    int hits = 0;

    {
        auto sub = EventBus::Instance().ScopedSubscribe(
            EventType::ShowMessage, [&](const Event&) { ++hits; });
        CHECK(sub.Active());

        EventBus::Instance().Publish(Msg("in-scope"));
        CHECK(hits == 1);   // token 存活期間有被派送
    }   // sub 解構 -> 恰好移除此 handler

    EventBus::Instance().Publish(Msg("after-scope"));
    CHECK(hits == 1);       // 不再被呼叫——證明已取消訂閱
}

// 作用域 Subscription 可避免懸空捕捉造成的 use-after-free。
TEST_CASE("Scoped Subscription prevents the B1/B2 dangling-capture UAF") {
    // 重現懸空捕捉的陷阱：handler 以參考捕捉呼叫端擁有的狀態，而該狀態之後被銷毀。
    // 在舊的「raw Subscribe + 手動 Clear」模型下，若忘了在捕捉物銷毀前 Clear()，下一次
    // Publish 就會 use-after-free。將 handler 交給作用域 Subscription，可使訂閱與被捕捉
    // 狀態同步銷毀，匯流排便絕不會對已釋放的記憶體呼叫 handler。
    int observed = 0;

    {
        // 用堆積配置的「呼叫端狀態」，這樣若取消訂閱失敗，sanitizer 能偵測到真正的
        // UAF（而非僅是離開作用域）。
        auto captured = std::make_unique<int>(0);

        EventBus::Subscription sub = EventBus::Instance().ScopedSubscribe(
            EventType::ShowMessage,
            [ptr = captured.get(), &observed](const Event&) {
                *ptr += 1;          // 解參考被捕捉的狀態
                observed = *ptr;
            });

        EventBus::Instance().Publish(Msg("alive"));
        CHECK(observed == 1);       // 安全：捕捉物與 sub 皆存活

        // 先銷毀訂閱（RAII：它持有捕捉了 `ptr` 的 handler），再釋放被捕捉狀態。此處
        // 的順序對應 RAII handle 自動保證的安全拆解順序。
        sub.Reset();
        CHECK_FALSE(sub.Active());
        captured.reset();           // 釋放底層記憶體
    }

    // 被捕捉狀態消失後再 Publish，絕不得碰觸它。
    EventBus::Instance().Publish(Msg("dangling"));
    CHECK(observed == 1);           // handler 不再執行 -> 無 UAF
}

// 移動 Subscription 會轉移所有權，不會重複取消訂閱。
TEST_CASE("Subscription move transfers ownership, no double-unsubscribe") {
    int hits = 0;

    SUBCASE("move-construct") {
        EventBus::Subscription a = EventBus::Instance().ScopedSubscribe(
            EventType::ShowMessage, [&](const Event&) { ++hits; });

        EventBus::Subscription b(std::move(a));
        CHECK_FALSE(a.Active());     // 被移走者不再擁有任何東西
        CHECK(b.Active());

        EventBus::Instance().Publish(Msg("x"));
        CHECK(hits == 1);            // 恰好一個存活的 handler

        // a 的解構必須是 no-op（不得重複取消訂閱，也不得移除 b 的 handler）。b 仍持有
        // 存活的訂閱。
    }

    SUBCASE("move-assign over a live subscription") {
        int otherHits = 0;
        EventBus::Subscription a = EventBus::Instance().ScopedSubscribe(
            EventType::ShowMessage, [&](const Event&) { ++hits; });
        EventBus::Subscription b = EventBus::Instance().ScopedSubscribe(
            EventType::ShowMessage, [&](const Event&) { ++otherHits; });

        // 移動指派必須先 Reset b 原本的 handler（otherHits 不再增加），再接管 a 的，
        // 最後恰好剩一個存活的 handler（hits）。
        b = std::move(a);
        CHECK_FALSE(a.Active());
        CHECK(b.Active());

        EventBus::Instance().Publish(Msg("y"));
        CHECK(hits == 1);            // a 的 handler，現由 b 持有
        CHECK(otherHits == 0);       // b 原本的 handler 已被取消訂閱
    }

    SUBCASE("destroying both copies removes the handler exactly once") {
        {
            EventBus::Subscription a = EventBus::Instance().ScopedSubscribe(
                EventType::ShowMessage, [&](const Event&) { ++hits; });
            EventBus::Subscription b(std::move(a));
            EventBus::Instance().Publish(Msg("z1"));
            CHECK(hits == 1);
        }   // a（no-op）與 b（真正移除）都在此解構
        EventBus::Instance().Publish(Msg("z2"));
        CHECK(hits == 1);            // 唯一擁有者銷毀後便不再被派送
    }
}

// ScopedSubscribe 與 raw Subscribe / Clear 可並存。
TEST_CASE("ScopedSubscribe coexists with raw Subscribe / Clear") {
    int rawHits    = 0;
    int scopedHits = 0;

    EventBus::Instance().Subscribe(EventType::ShowMessage,
        [&](const Event&) { ++rawHits; });

    {
        auto sub = EventBus::Instance().ScopedSubscribe(
            EventType::ShowMessage, [&](const Event&) { ++scopedHits; });
        EventBus::Instance().Publish(Msg("a"));
        CHECK(rawHits == 1);
        CHECK(scopedHits == 1);
    }   // 作用域 handler 被移除；raw handler 不受影響

    EventBus::Instance().Publish(Msg("b"));
    CHECK(rawHits == 2);             // raw 訂閱者仍存活
    CHECK(scopedHits == 1);          // 作用域那個已消失

    // Clear() 仍會清掉全部（向後相容），且一個其 handler 已被 Clear() 移除的
    // Subscription 解構時無害（id 已不存在時 Unsubscribe 為 no-op）。
    auto sub = EventBus::Instance().ScopedSubscribe(
        EventType::ShowMessage, [&](const Event&) { ++scopedHits; });
    EventBus::Instance().Clear();
    EventBus::Instance().Publish(Msg("c"));
    CHECK(rawHits == 2);
    CHECK(scopedHits == 1);
}   // sub 在此解構：Clear 後 Unsubscribe(id) 是安全的 no-op
