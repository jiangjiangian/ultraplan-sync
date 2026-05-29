#include "engine/events/EventBus.h"

#include <mutex>     // std::unique_lock——較新的 libstdc++（GCC 13+）下，
                     // <shared_mutex> 已不再間接帶入此標頭。
#include <utility>   // std::exchange

EventBus& EventBus::Instance() {
    static EventBus instance;
    return instance;
}

EventBus& EventBus::Subscribe(EventType type, Handler handler) {
    std::unique_lock lock(mutex_);
    handlers_[type].push_back(Slot{nextId_++, std::move(handler)});
    return *this;
}

EventBus::Subscription EventBus::ScopedSubscribe(EventType type,
                                                 Handler   handler) {
    std::unique_lock lock(mutex_);
    const std::uint64_t id = nextId_++;
    handlers_[type].push_back(Slot{id, std::move(handler)});
    return Subscription{this, id};
}

void EventBus::Unsubscribe(std::uint64_t id) noexcept {
    std::unique_lock lock(mutex_);
    for (auto& [type, slots] : handlers_) {
        for (auto it = slots.begin(); it != slots.end(); ++it) {
            if (it->id == id) {
                slots.erase(it);
                return;
            }
        }
    }
}

void EventBus::Publish(const Event& event) const {
    // 只在複製該事件型別的 handler 清單期間持有 shared lock，派送前即釋放。
    // 如此 (a) 讓 handler 內可呼叫 Subscribe／Clear／取消訂閱（Subscription
    // 解構）而不會在 shared mutex 上死鎖，(b) 即使有寫入者在派送途中改動
    // handlers_，快照仍保持穩定——同步匯流排若直接對活向量遞迴 Publish 將是
    // 未定義行為。因此派送途中被解構的 Subscription 本輪仍會觸發，但之後不再。
    std::vector<Handler> snapshot;
    {
        std::shared_lock lock(mutex_);
        auto it = handlers_.find(event.type);
        if (it == handlers_.end()) return;
        snapshot.reserve(it->second.size());
        for (const auto& slot : it->second) snapshot.push_back(slot.handler);
    }
    for (const auto& h : snapshot) h(event);
}

EventBus& EventBus::Clear() {
    std::unique_lock lock(mutex_);
    handlers_.clear();
    return *this;
}

// --- EventBus::Subscription（以 RAII 自動取消訂閱）---------------------

EventBus::Subscription::Subscription(Subscription&& other) noexcept
    : bus_(std::exchange(other.bus_, nullptr)),
      id_(std::exchange(other.id_, 0)) {}

EventBus::Subscription&
EventBus::Subscription::operator=(Subscription&& other) noexcept {
    if (this != &other) {
        Reset();
        bus_ = std::exchange(other.bus_, nullptr);
        id_  = std::exchange(other.id_, 0);
    }
    return *this;
}

EventBus::Subscription::~Subscription() { Reset(); }

void EventBus::Subscription::Reset() noexcept {
    // 先取走所有權，使重入式解構（或第二次 Reset）成為 no-op——
    // 確保恰好取消訂閱一次，不會重複移除。
    if (EventBus* bus = std::exchange(bus_, nullptr)) {
        bus->Unsubscribe(std::exchange(id_, 0));
    }
}
