#include "controller/EventBus.h"

#include <mutex>     // std::unique_lock — <shared_mutex> no longer pulls this
                     // in transitively under newer libstdc++ (GCC 13+).
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
    // Hold the shared lock only while copying the per-type handler list,
    // then drop it before dispatch. This (a) lets a handler call Subscribe
    // / Clear / unsubscribe (Subscription dtor) without deadlocking on the
    // shared mutex, and (b) keeps the snapshot stable even if writers
    // mutate handlers_ mid-dispatch — synchronous bus + recursive publish
    // on the live vector would be UB. A Subscription destroyed mid-
    // dispatch therefore still fires this round but never again.
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

// --- EventBus::Subscription (RAII unsubscribe, H1) ---------------------

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
    // Take ownership away first so a re-entrant destruction (or a second
    // Reset) is a no-op — exactly-once unsubscribe, no double removal.
    if (EventBus* bus = std::exchange(bus_, nullptr)) {
        bus->Unsubscribe(std::exchange(id_, 0));
    }
}
