#include "EventBus.h"

EventBus& EventBus::Instance() {
    static EventBus instance;
    return instance;
}

EventBus& EventBus::Subscribe(EventType type, Handler handler) {
    std::unique_lock lock(mutex_);
    handlers_[type].push_back(std::move(handler));
    return *this;
}

void EventBus::Publish(const Event& event) const {
    // Hold the shared lock only while copying the per-type handler list,
    // then drop it before dispatch. This (a) lets a handler call Subscribe
    // / Clear without deadlocking on the shared mutex, and (b) keeps the
    // snapshot stable even if writers mutate handlers_ mid-dispatch —
    // synchronous bus + recursive publish on the live vector would be UB.
    std::vector<Handler> snapshot;
    {
        std::shared_lock lock(mutex_);
        auto it = handlers_.find(event.type);
        if (it == handlers_.end()) return;
        snapshot = it->second;
    }
    for (const auto& h : snapshot) h(event);
}

EventBus& EventBus::Clear() {
    std::unique_lock lock(mutex_);
    handlers_.clear();
    return *this;
}
