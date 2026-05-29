#include "doctest/doctest.h"
#include "game/state/ChapterToast.h"
#include "engine/events/EventBus.h"
#include "game/controller/EventWiring.h"
#include "game/state/InterludeExit.h"
#include "game/world/World.h"
#include "engine/math/Vec2.h"
#include <string>
#include <vector>

using nccu::SemesterState;
using nccu::World;
using nccu::engine::math::Vec2;

/**
 * @file test_interlude_exit_feedback.cpp
 * @brief 驗證幕間出口的玩家回饋：進入市集時的提示、首次跨入南側帶狀區時
 *        「準備離開市集」的提示，以及這些訊息經事件匯流排送達 HUD。
 */

// 幕間出口原本只是個無聲的位置觸發；本檔測試新增的兩種回饋：
//   (a) 抵達市集時發出的入口提示（從 GameController 重置位置後送出）。
//   (b) 首次跨入南側帶狀區那一幀發出、每次造訪僅一次的離開提示。
// 鎖存（latch）由 GameController 管理；輔助函式 MaybeAnnounceInterludeExit
// 封裝了整個生命週期，使測試不需完整的 Input/Renderer 堆疊。

namespace {

// 把一個位於呼叫端堆疊上、生命週期穩定的 std::vector<std::string> 接到匯流排。
// Subscription token 可移動；lambda 捕捉的是呼叫端堆疊上的參考，故整個測試
// 期間都有效。
[[nodiscard]] EventBus::Subscription
SubscribeToAll(std::vector<std::string>& texts) {
    return EventBus::Instance().ScopedSubscribe(
        EventType::ShowMessage,
        [&texts](const Event& e) { texts.push_back(e.text); });
}

bool Contains(const std::vector<std::string>& v, const char* needle) {
    for (const auto& s : v) if (s == needle) return true;
    return false;
}

std::size_t Count(const std::vector<std::string>& v, const char* needle) {
    std::size_t n = 0;
    for (const auto& s : v) if (s == needle) ++n;
    return n;
}

} // namespace

// 固定南側帶狀區的判定，避免日後調整區域常數時悄悄讓整套測試失效。
TEST_CASE("InInterludeExitZone: south band detection still pins") {
    // y=1910 是跨入帶狀區的第一幀；入口點（kInterludeEntry, y=1500）不在
    // 區內，因此玩家抵達時不會自動觸發。
    CHECK(nccu::InInterludeExitZone(Vec2{500.0f, 1910.0f}));
    CHECK_FALSE(nccu::InInterludeExitZone(nccu::kInterludeEntry));
}

// 首次進入帶狀區會發佈一次提示，之後重複呼叫不再發佈（具冪等性）。
TEST_CASE("MaybeAnnounceInterludeExit: publishes once, then idempotent") {
    EventBus::Instance().Clear();
    std::vector<std::string> texts;
    auto sub = SubscribeToAll(texts);
    bool latched = false;

    // 首次進入區域：發佈。
    CHECK(nccu::MaybeAnnounceInterludeExit(EventBus::Instance(), latched));
    CHECK(latched);
    CHECK(Contains(texts, nccu::kInterludeExitPrep));
    CHECK(Count(texts, nccu::kInterludeExitPrep) == 1);

    // 玩家在離開幕間前於邊界來回 / 重新進入：不再發佈第二次。這就是「不洗版」
    // 保證 — 每個鎖存週期只觸發一次。
    CHECK_FALSE(nccu::MaybeAnnounceInterludeExit(EventBus::Instance(), latched));
    CHECK_FALSE(nccu::MaybeAnnounceInterludeExit(EventBus::Instance(), latched));
    CHECK(Count(texts, nccu::kInterludeExitPrep) == 1);

    EventBus::Instance().Clear();
}

// 鎖存生命週期：每次進入幕間時重置，跨入帶狀區時再次觸發。
TEST_CASE("Interlude-visit latch lifecycle: reset on entry, fire on cross") {
    EventBus::Instance().Clear();
    std::vector<std::string> texts;
    auto sub = SubscribeToAll(texts);
    bool latched = false;

    // First visit: cross the band once.
    CHECK(nccu::MaybeAnnounceInterludeExit(EventBus::Instance(), latched));
    CHECK(Count(texts, nccu::kInterludeExitPrep) == 1);

    // Player leaves the Interlude, then comes back: GameController resets
    // the latch in the Interlude-arrival branch. Mimic that reset and
    // verify the next zone entry fires again exactly once.
    latched = false;                                 // GC's reset
    CHECK(nccu::MaybeAnnounceInterludeExit(EventBus::Instance(), latched));
    CHECK(Count(texts, nccu::kInterludeExitPrep) == 2);
    CHECK_FALSE(nccu::MaybeAnnounceInterludeExit(EventBus::Instance(), latched));
    CHECK(Count(texts, nccu::kInterludeExitPrep) == 2);

    EventBus::Instance().Clear();
}

TEST_CASE("Interlude arrival hint reaches the HUD subscriber") {
    // Full path: a ShowMessage publish containing the arrival hint lands
    // on World::HudMessage(), which the View reads. The hint string is
    // the contract — GameController's Interlude-arrival branch publishes
    // exactly this text right after RespawnChapterRoster.
    EventBus::Instance().Clear();
    World w("", /*loadSprites=*/false);
    nccu::WireHudMessageSubscriber(EventBus::Instance(), w);

    EventBus::Instance().Publish(
        Event{EventType::ShowMessage, nccu::kInterludeArrivalHint});
    CHECK(w.HudMessage() == nccu::kInterludeArrivalHint);

    EventBus::Instance().Clear();
}
